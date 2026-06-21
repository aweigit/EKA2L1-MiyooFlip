/*
 * Copyright (c) 2021 EKA2L1 Team.
 *
 * This file is part of EKA2L1 project.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "context_egl.h"
#include <common/log.h>

#include <fcntl.h>
#include <unistd.h>
#include <gbm.h>

#include <xf86drm.h>
#include <xf86drmMode.h>


extern struct gbm_device *gbm_device;
extern struct gbm_surface *gbm_surface;
extern int g_device_fd;
extern drmModeCrtcPtr crtc;
extern drmModeConnectorPtr connector;

// int drm_fd;
// uint32_t my_fb;
// uint32_t last_fb;

// 全局或上下文结构体中的状态变量
static struct gbm_bo *s_previous_bo = NULL;
static int s_page_flip_pending = 0;

/**
 * DRM Page Flip 事件回调
 * @param fd        DRM 设备文件描述符
 * @param frame     触发事件的帧序号 (vblank count)
 * @param sec       事件发生时的秒级时间戳
 * @param usec      事件发生时的微秒级时间戳
 * @param user_data drmModePageFlip 调用时传入的自定义指针
 */
static void page_flip_handler(int fd, unsigned int frame,
                              unsigned int sec, unsigned int usec,
                              void *user_data)
{
    // 1.在buffer上屏之后，此时 GPU 已经不再读取它，可以安全归还给 GBM 缓冲池。
    if (s_previous_bo != NULL) {
        gbm_surface_release_buffer(gbm_surface, s_previous_bo);
        s_previous_bo = NULL;
    }

    // 2. 清除 pending 标志，允许主循环提交新帧
    s_page_flip_pending = 0;

    // (可选) 计算精确帧间隔用于性能监控
    // double ms = sec * 1000.0 + usec / 1000.0;
}

drmEventContext evctx = {
    .version = DRM_EVENT_CONTEXT_VERSION,
    .page_flip_handler = page_flip_handler,
};

// 销毁回调：当 GBM BO 被释放时，自动清理关联的 FB
static void fb_destroy_callback(struct gbm_bo *bo, void *data)
{
    uint32_t fb_id = (uint32_t)(uintptr_t)data;

    if (fb_id > 0) {
        //fprintf(stdout,  "drmModeRmFB\n");
        drmModeRmFB(g_device_fd, fb_id);
    }
}

/**
 * 获取或创建与 GBM BO 关联的 DRM Framebuffer
 * @param bo       GBM Buffer Object
 * @return         DRM FB ID，失败返回 0
 */
uint32_t get_or_create_fb(struct gbm_bo *bo)
{
    // ⭐ 第一步：检查 BO 是否已有关联的 FB
    uint32_t fb_id = (uint32_t)(uintptr_t)gbm_bo_get_user_data(bo);
    if (fb_id != 0) {
        //fprintf(stdout, "命中缓存\n");
        return fb_id;
    }

    // ⭐ 第二步：首次遇到此 BO，创建新的 FB
    if(drmModeAddFB(g_device_fd, gbm_bo_get_width(bo),
                gbm_bo_get_height(bo), 24,
                gbm_bo_get_bpp(bo),
                gbm_bo_get_stride(bo),
                gbm_bo_get_handle(bo).u32,
                &fb_id)!=0)
    {
        fprintf(stderr,  "drmModeAddFB failed\n");
    }

    // ⭐ 第三步：将 FB ID 绑定到 BO，并注册销毁回调
    // 当最终回收此 BO 时，fb_destroy_callback 会自动被调用，移除对应的 FB
    gbm_bo_set_user_data(bo, (void *)(uintptr_t)fb_id, fb_destroy_callback);

    return fb_id;
}
int firstFrame=1;
void display_output_flip()
{
    if(firstFrame)
    {
        struct gbm_bo *bo = gbm_surface_lock_front_buffer(gbm_surface);
        uint32_t fb = get_or_create_fb(bo);

        if(drmModeSetCrtc(g_device_fd, crtc->crtc_id, fb, 0, 0,
                    &connector->connector_id, 1, &crtc->mode)!=0)
        {
            fprintf(stderr,  "first drmModeSetCrtc failed\n");
        }

        s_page_flip_pending = 1;
        s_previous_bo = bo;
        int ret = drmModePageFlip(g_device_fd, crtc->crtc_id, fb,
                              DRM_MODE_PAGE_FLIP_EVENT, NULL);
        if (ret != 0) {
            // 翻页失败（如队列满），立即释放避免泄漏
            fprintf(stderr, "PageFlip failed1:%d\n", errno);
            s_previous_bo = NULL;
            s_page_flip_pending = 0;
            gbm_surface_release_buffer(gbm_surface, bo);
        }
        
        firstFrame = 0;
        return;
    }

    // ⭐ 关键：如果上一帧还没上屏，不要提交新帧
    if (s_page_flip_pending) {
        drmHandleEvent(g_device_fd, &evctx);
    }

    // ---- 获取新渲染的 buffer ----
    struct gbm_bo *bo = gbm_surface_lock_front_buffer(gbm_surface);
    if (!bo) {
        fprintf(stderr, "lock_front_buffer failed\n");
        return;
    }

    // ---- 复用 FB (首次为 bo 创建，后续从 user_data 取回) ----
    uint32_t fb_id = get_or_create_fb(bo);

    s_previous_bo = bo;
    s_page_flip_pending = 1;
    // ---- 异步翻页 ----
    int ret = drmModePageFlip(g_device_fd, crtc->crtc_id, fb_id,
                              DRM_MODE_PAGE_FLIP_EVENT, NULL);
    if (ret != 0) {
        // 翻页失败（如队列满），立即释放避免泄漏
        fprintf(stderr, "PageFlip failed:%d\n", errno);
        s_previous_bo = NULL;
        s_page_flip_pending = 0;
        gbm_surface_release_buffer(gbm_surface, bo);
    }
}

namespace eka2l1::drivers::graphics {
    static constexpr std::array<EGLint, 13> egl_attribs{ EGL_SURFACE_TYPE,
                                                         EGL_WINDOW_BIT,
                                                         EGL_BLUE_SIZE,
                                                         8,
                                                         EGL_GREEN_SIZE,
                                                         8,
                                                         EGL_RED_SIZE,
                                                         8,
                                                         EGL_DEPTH_SIZE,
                                                         0,
                                                         EGL_STENCIL_SIZE,
                                                         0,
                                                         EGL_NONE };
    static constexpr std::array<EGLint, 15> egl_attribs_es{ EGL_SURFACE_TYPE,
                                                         EGL_WINDOW_BIT,
                                                         EGL_RENDERABLE_TYPE,
                                                         EGL_OPENGL_ES2_BIT,//EGL_OPENGL_ES3_BIT_KHR,
                                                         EGL_BLUE_SIZE,
                                                         8,
                                                         EGL_GREEN_SIZE,
                                                         8,
                                                         EGL_RED_SIZE,
                                                         8,
                                                         EGL_DEPTH_SIZE,
                                                         0,
                                                         EGL_STENCIL_SIZE,
                                                         0,
                                                         EGL_NONE };

    static constexpr std::array<EGLint, 3> egl_context_attribs_es{ EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };

    // void gl_context_egl::display_output()
    // {
    //     //锁定 GBM 表面的前端缓冲区，从而允许应用程序访问和读取当前显示在屏幕上的像素数据。这在需要从屏幕缓冲区中读取内容（例如截图或调试）时非常有用。
    //     struct gbm_bo *bo = gbm_surface_lock_front_buffer(gbm_surface);
    //     if(bo==nullptr)
    //     {
    //         LOG_CRITICAL(DRIVER_GRAPHICS, "gbm_surface_lock_front_buffer failed");
    //     }

    //     //在 DRM 驱动程序中注册一个新的帧缓冲区，并将其与指定的 GBM 缓冲区(bo)关联
    //     if(drmModeAddFB(drm_fd, gbm_bo_get_width(bo),
    //                 gbm_bo_get_height(bo), 24,
    //                 gbm_bo_get_bpp(bo),
    //                 gbm_bo_get_stride(bo),
    //                 gbm_bo_get_handle(bo).u32,
    //                 &my_fb)!=0)
    //     {
    //         LOG_CRITICAL(DRIVER_GRAPHICS, "drmModeAddFB failed");
    //     }

    //     //LOG_INFO(DRIVER_GRAPHICS, "my_fb: {}",my_fb);

    //     // show my_fb
    //     if(drmModeSetCrtc(drm_fd, crtc->crtc_id, my_fb, 0, 0,
    //                 &connector->connector_id, 1, &crtc->mode)!=0)
    //     {
    //         LOG_CRITICAL(DRIVER_GRAPHICS, "drmModeSetCrtc failed");
    //     }

    //     //删除之前的缓冲区
    //     if(last_fb>0)
    //     {
    //         drmModeRmFB(drm_fd, last_fb);
    //         last_fb=my_fb;
    //     }
        
    //     // hold on for a moment
    //     // sleep(2);

    //     // restore previous fb
    //     // assert(!drmModeSetCrtc(drm_fd, crtc->crtc_id, fb->fb_id, 0, 0,
    //     //             &connector->connector_id, 1, &crtc->mode));

    //     gbm_surface_release_buffer(gbm_surface, bo);

        
    // }

    EGLConfig gl_context_egl::get_config()
    {
        EGLint egl_config_attribs[] = {
            EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
            EGL_RED_SIZE,   8,  
            EGL_GREEN_SIZE, 8,
            EGL_BLUE_SIZE,  8,  
            EGL_ALPHA_SIZE, 8,
            EGL_NATIVE_VISUAL_ID, GBM_FORMAT_ARGB8888,
            EGL_NONE
        };

        EGLint num_configs = 0;
        EGLConfig configs = {0};
        if(eglChooseConfig(egl_display, egl_config_attribs, &configs, 1, &num_configs) != EGL_TRUE)
        {
            LOG_CRITICAL(DRIVER_GRAPHICS, "get_config eglChooseConfig() failed");
        }
        return configs;
    }


    bool gl_context_egl::is_headless() const {
        return !render_window;
    }

    bool gl_context_egl::create_context(EGLContext shared, std::pair<int, int> *target_version) {
        printf("create_context\n");
        if (m_opengl_mode == mode::opengl_es) {
            if (egl_context = eglCreateContext(egl_display, egl_config, shared, egl_context_attribs_es.data());
                    egl_context == EGL_NO_CONTEXT) {
                LOG_CRITICAL(DRIVER_GRAPHICS, "eglCreateContext() failed");
                return false;
            }
        } else {
            std::array<EGLint, 5> egl_context_attrib_template{ EGL_CONTEXT_MAJOR_VERSION, 0,
                EGL_CONTEXT_MINOR_VERSION, 0, EGL_NONE };
            
            if (target_version != nullptr) {
                egl_context_attrib_template[1] = target_version->first;
                egl_context_attrib_template[3] = target_version->second;

                if (egl_context = eglCreateContext(egl_display, egl_config, shared, egl_context_attrib_template.data());
                        egl_context == EGL_NO_CONTEXT) {
                    LOG_CRITICAL(DRIVER_GRAPHICS, "eglCreateContext() failed");
                    return false;
                }

                context_version = *target_version;
            } else {
                for (auto version: s_desktop_opengl_versions) {
                    egl_context_attrib_template[1] = version.first;
                    egl_context_attrib_template[3] = version.second;

                    if (egl_context = eglCreateContext(egl_display, egl_config, shared, egl_context_attrib_template.data());
                            egl_context != EGL_NO_CONTEXT) {
                        context_version = version;
                        break;
                    }
                }

                if (egl_context == EGL_NO_CONTEXT) {
                    LOG_CRITICAL(DRIVER_GRAPHICS, "eglCreateContext() failed");
                    return false;
                }
            }
        }
        printf("create_context2\n");
        return true;
    }

    void gl_context_egl::init_gl() {
        LOG_INFO(DRIVER_GRAPHICS, "init_gl1");
        LOG_INFO(DRIVER_GRAPHICS, "gbm_device:{}", (void*)gbm_device);
        LOG_INFO(DRIVER_GRAPHICS, "gbm_surface:{}", (void*)gbm_surface);

        PFNEGLGETPLATFORMDISPLAYEXTPROC get_platform_display = NULL;
        get_platform_display = (PFNEGLGETPLATFORMDISPLAYEXTPROC)eglGetProcAddress("eglGetPlatformDisplayEXT");
        egl_display = get_platform_display(EGL_PLATFORM_GBM_KHR, gbm_device, NULL);


        if (/*egl_display = eglGetDisplay(wsi.display_connection ? reinterpret_cast<EGLNativeDisplayType>(wsi.display_connection) : EGL_DEFAULT_DISPLAY);*/ egl_display == EGL_NO_DISPLAY) {
            LOG_CRITICAL(DRIVER_GRAPHICS, "eglGetDisplay() failed");
            return;
        }
        printf("init_gl1.1\n");
        EGLint majorVersion;
	    EGLint minorVersion;
        if (eglInitialize(egl_display, &majorVersion, &minorVersion) != EGL_TRUE) {
            LOG_CRITICAL(DRIVER_GRAPHICS, "eglInitialize() failed");
            return;
        }

        LOG_INFO(DRIVER_GRAPHICS, "majorVersion:{} minorVersion:{}", majorVersion, minorVersion);


        printf("init_gl1.2\n");

        if (eglBindAPI((m_opengl_mode == mode::opengl_es) ? EGL_OPENGL_ES_API : EGL_OPENGL_API) != EGL_TRUE) {
            LOG_CRITICAL(DRIVER_GRAPHICS, "Can't bind target graphics API!");
            return;
        }

        LOG_INFO(DRIVER_GRAPHICS, "init_gl1.3");

        egl_config = get_config();

        printf("init_gl2\n");

        if (!create_context(nullptr, nullptr)) {
            return;
        }

        printf("init_gl3\n");

        if (m_opengl_mode == mode::opengl_es && eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context) != EGL_TRUE) {
            LOG_CRITICAL(DRIVER_GRAPHICS, "eglMakeCurrent() failed");
            return;
        }

        printf("init_gl4\n");
    }

    bool gl_context_egl::init_surface() {
        prepare_render_window();

        if (!render_window) {
            LOG_CRITICAL(DRIVER_GRAPHICS, "surface is nullptr");
            return false;
        }

        destroy_surface();
        create_surface();

        if (eglSurfaceAttrib(egl_display, egl_surface, EGL_SWAP_BEHAVIOR, EGL_BUFFER_DESTROYED) != EGL_TRUE) {
            LOG_CRITICAL(DRIVER_GRAPHICS, "eglSurfaceAttrib() failed");
            return false;
        }

        if (eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context) != EGL_TRUE) {
            LOG_CRITICAL(DRIVER_GRAPHICS, "eglMakeCurrent() failed");
            return false;
        }

        return true;
    }

    void gl_context_egl::update_surface(void *new_surface) {
        LOG_INFO(DRIVER_GRAPHICS, "gl_context_egl::update_surface");
        render_window = new_surface;
        LOG_INFO(DRIVER_GRAPHICS, "clear_current");
        clear_current();
        LOG_INFO(DRIVER_GRAPHICS, "destroy_surface");
        destroy_surface();
        LOG_INFO(DRIVER_GRAPHICS, "create_surface");
        create_surface();
        LOG_INFO(DRIVER_GRAPHICS, "make_current");
        make_current();
    }

    bool gl_context_egl::make_current() {
        if (!egl_surface) {
            if (!init_surface()) {
                // Bind with no surface
                return eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context);
            } else {
                return true;
            }
        }
        return eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context);
    }

    bool gl_context_egl::clear_current() {
        return eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    }

    void gl_context_egl::swap_buffers() {
        if (render_window) {
            //LOG_INFO(DRIVER_GRAPHICS, "swap_buffers");
            eglSwapBuffers(egl_display, egl_surface);
            display_output_flip();
        }
    }

    void gl_context_egl::set_swap_interval(const std::int32_t interval) {
        if (render_window) {
            eglSwapInterval(egl_display, interval);
        }
    }

    gl_context_egl::gl_context_egl(const window_system_info& wsi, bool stereo, bool core, bool is_gles)
        : egl_surface(nullptr)
        , egl_context(nullptr)
        , egl_display(nullptr)
        , render_window(nullptr)
        , wsi(wsi) {
        m_opengl_mode = is_gles ? mode::opengl_es : mode::opengl;
        printf("gl_context_egl\n");
        init_gl();
    }

    gl_context_egl::~gl_context_egl() {
        destroy_surface();

        if (egl_context) {
            if (eglGetCurrentContext() == egl_context) {
                clear_current();
            }

            if (!eglDestroyContext(egl_display, egl_context)) {
                LOG_ERROR(DRIVER_GRAPHICS, "Failed to destroy GLES context!");
            }
        }
    }

    void gl_context_egl::create_surface() {
        if (!render_window) {
            return;
        }

        egl_config=get_config();

        if (egl_surface = eglCreateWindowSurface(egl_display, egl_config, reinterpret_cast<EGLNativeWindowType>(render_window), 0);
            egl_surface == EGL_NO_SURFACE) {
            LOG_ERROR(DRIVER_GRAPHICS, "Create window surface failed with code: {}", eglGetError());
            return;
        }

        printf("create_surface\n");

        EGLint surface_width = 1;
        EGLint surface_height = 1;

        if (!eglQuerySurface(egl_display, egl_surface, EGL_WIDTH, &surface_width) ||
            !eglQuerySurface(egl_display, egl_surface, EGL_HEIGHT, &surface_height))  {
            LOG_ERROR(DRIVER_GRAPHICS, "Failed to retrieve surface width or height.");
            return;
        }

        m_backbuffer_width = static_cast<int>(surface_width);
        m_backbuffer_height = static_cast<int>(surface_height);
    }

    void gl_context_egl::destroy_surface() {
        if (!egl_surface) {
            return;
        }
        if (eglGetCurrentSurface(EGL_DRAW) == egl_surface) {
            eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        }
        if (!eglDestroySurface(egl_display, egl_surface)) {
            LOG_CRITICAL(DRIVER_GRAPHICS, "eglDestroySurface() failed");
        }
        egl_surface = EGL_NO_SURFACE;

        printf("destroy_surface\n");
    }

    bool gl_context_egl::init_gl_for_shared(gl_context_egl *parent) {
        printf("init_gl_for_shared\n");
        egl_display = parent->egl_display;
        egl_config = parent->egl_config;

        m_opengl_mode = parent->m_opengl_mode;

        if (eglBindAPI((m_opengl_mode == mode::opengl_es) ? EGL_OPENGL_ES_API : EGL_OPENGL_API) != EGL_TRUE) {
            LOG_CRITICAL(DRIVER_GRAPHICS, "Can't bind target graphics API!");
            return false;
        }

        if (!create_context(parent->egl_context, &parent->context_version)) {
            return false;
        }

        return true;
    }

    std::unique_ptr<gl_context> gl_context_egl::create_shared_context() {
        std::unique_ptr<gl_context_egl> shared_context = std::make_unique<gl_context_egl>();
        if (!shared_context->init_gl_for_shared(this)) {
            return nullptr;
        }

        return shared_context;
    }

    void gl_context_egl::update(const std::uint32_t new_width, const std::uint32_t new_height) {
        m_backbuffer_width = static_cast<int>(new_width);
        m_backbuffer_height = static_cast<int>(new_height);
    }
}