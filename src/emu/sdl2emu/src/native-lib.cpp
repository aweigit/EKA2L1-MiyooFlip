/*
 * Copyright (c) 2020 EKA2L1 Team
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

#include<iostream>
#include <string>

#include <state.h>
#include <thread.h>
#include <common/fileutils.h>
#include <common/path.h>
#include <drivers/audio/audio.h>
#include <drivers/camera/camera_collection.h>
#include <drivers/graphics/graphics.h>

#include <filesystem>

#if EKA2L1_ARCH(ARM)
#include <cpu/12l1r/tests/test_entry.h>
#endif

// #define CATCH_CONFIG_RUNNER
// #define CATCH_CONFIG_ANDROID_LOGWRITE

//#include <catch2/catch.hpp>

std::unique_ptr<eka2l1::sdl2::emulator> state;
bool inited = false;

std::vector<std::string> getDeviceFirmwareCodes() {//获取设备固件代码列表
    std::vector<std::string> info = state->launcher->get_device_firwmare_codes();

    for (size_t i = 0; i < info.size(); ++i)
    {
        std::cout<<"firwmare_code "<<i<<" "<<info[i]<<std::endl;
    }

    return info;
}

std::vector<std::string> getDevices(//获取设备列表，手机列表
    ) {
    std::vector<std::string> info = state->launcher->get_devices();

    for (size_t i = 0; i < info.size(); ++i)
    {
        std::cout<<"device "<<i<<" "<<info[i]<<std::endl;
    }

    return info;
}

std::vector<std::string> getPackages(//获取包列表，这个包不只是app，还有系统组件
   ) {
    std::vector<std::string> info = state->launcher->get_packages();
    
    return info;
}

int installApp(std::string path) {//安装app，参数是app文件路径

    return state->launcher->install_app(path);
}

void emu_Emulator_setDirectory(//设置当前工作路径，就是设置pwd
    const char *cstr) {
    std::string cpath = std::string(cstr);
    const auto executable_directory = eka2l1::file_directory(cpath);
    eka2l1::common::set_current_directory(executable_directory);
}

int emu_Emulator_installDevice(std::string rpkg_path,//安装设备，参数为固件包路径(ROM文件和RPKG文件)
    std::string rom_path, bool install_rpkg) {

    return state->launcher->install_device(rpkg_path, rom_path, install_rpkg);
}

bool emu_Emulator_startNative(uint32_t rotate)//生成emu，入口点
    {
    state = std::make_unique<eka2l1::sdl2::emulator>();

    return emulator_entry(*state, rotate);
}

void emu_Emulator_rotate(uint32_t rotate)//旋转画面
    {
    state->winserv->get_screens()->ui_rotation = rotate;
    //state->rotate=rotate;
}

bool emu_Emulator_startNative_install(const char*rom, const char*pkg)//生成emu，入口点
    {
    state = std::make_unique<eka2l1::sdl2::emulator>();

    return emulator_entry(*state, rom, pkg);
}

void emu_Emulator_setCurrentDevice(int32_t id, bool is_temp) {//通过设备id 设置当前设备
    state->launcher->set_current_device(id, is_temp);
}

static void wout(const char *filename, const char *text, int r)
{
  FILE *file = fopen(filename, "a");
    if (!file) {
        perror("文件打开失败");
        return;
    }
 
    // 2. 写入字符串
    // 方法1：使用fprintf（支持格式化）
    if (fprintf(file, "%s %d\n", text, r) < 0) {
        perror("写入失败");
        fclose(file);
        return;
    }
 
    // 方法2：使用fputs（更简单）
    // if (fputs(text, file) == EOF) {
    //     perror("写入失败");
    // }
 
    // 3. 关闭文件
    if (fclose(file) != 0) {
        perror("文件关闭失败");
        return;
    }

}

namespace fs = std::filesystem; // 简化命名空间
void create_file_with_directories(const fs::path& file_path, const std::string &content) {
    try {
        // 1. 获取文件所在目录
        fs::path dir = file_path.parent_path();
        
        // 2. 如果目录不存在，递归创建所有父目录
        if (!fs::exists(dir)) {
            fs::create_directories(dir);
            std::cout << "Created directories: " << dir << std::endl;
        }
        
        // 3. 创建文件（如果已存在则覆盖）
        std::ofstream file(file_path);
        if (file.is_open()) {
            file << content << std::endl;
            std::cout << "File created: " << file_path << std::endl;
        } else {
            throw std::runtime_error("Failed to create file");
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

std::vector<std::string> getApps(std::string file_path  //获取string类型app列表返回给java
    ) {
    std::vector<std::string> info = state->launcher->get_apps();
    std::string gamelist="<?xml version=\"1.0\"?>\n<gameList>\n";

    std::vector<std::string> apps;

    for (size_t i = 0; i < info.size(); i+=2)
    {
        try {
            std::replace(info[i+1].begin(), info[i+1].end(), ' ', '-');//将游戏名里的空格替换掉
            apps.push_back(info[i+1]+" uid:"+info[i]);
            std::cout<<"app:"<<i/2<<" uid:"<<info[i]<<" name:"<<info[i+1]<<std::endl;

            std::string icon_path=state->launcher->get_app_icon_path(atoi(info[i].c_str()), file_path);//获取svg图标，不一定有

            std::string game=fmt::format("<game>\n<uid>{}</uid>\n<name>{}</name>\n<image>{}</image>\n</game>\n",
                info[i],info[i+1],icon_path);
            
            gamelist+=game;
            
            create_file_with_directories(file_path+info[i+1]+".st", info[i]);
        }
        catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
        
    }
    gamelist+="</gameList>\n";

    std::ofstream file("gamelist.xml");
    
    // 检查文件是否成功打开
    if (!file.is_open()) {
        std::cerr << "Error: Unable to create file!" << std::endl;
    }
    
    // 写入内容
    file << gamelist;
    file.close();

    return apps;
}

static void redraw_screens_immediately() {//立即重绘屏幕
    state->graphics_driver->wait_for(&state->present_status);

    eka2l1::drivers::graphics_command_builder builder;
    state->launcher->draw(builder, state->winserv ? state->winserv->get_screens() : nullptr,
                          state->window->window_fb_size().x,
                          state->window->window_fb_size().y);

    state->present_status = -100;
    builder.present(&state->present_status);

    eka2l1::drivers::command_list retrieved = builder.retrieve_command_list();
    state->graphics_driver->submit_command_list(retrieved);
}

void emu_Emulator_launchApp(int uid) {//通过app id 启动app
    // Launch the real app...
    state->launcher->launch_app(uid);
}

void emu_Emulator_surfaceChanged(void* surface,//根据surface生成native window，并开始窗口线程
    int width, int height) {
    state->window->surface_changed(surface, width, height);
    if (!inited) {
        init_threads(*state);
        inited = true;
    } else {
        start_threads(*state);
    }
}

void surfaceRedrawNeeded() {//重绘窗口
    redraw_screens_immediately();
}

void emu_Emulator_surfaceDestroyed() {//销毁surface
    pause_threads(*state);
    state->window->surface_changed(nullptr, 0, 0);
}

void emu_Emulator_pressKey(int key,//向emu发送按键
    int keyState) {
    press_key(*state, key, keyState);//0 press 1 release
}

void emu_Emulator_touchScreen(int x, int y,//向emu发送触摸事件
    int z, int action, int pointer_id) {
    touch_screen(*state, x, y, z, action, pointer_id);
}

int emu_Emulator_installApp(std::string path) {//安装app，参数是app文件路径

    return state->launcher->install_app(path);
}

void emu_Emulator_getDevices()//获取设备列表，手机列表，返回给java
 {
    std::vector<std::string> info = state->launcher->get_devices();

    for (size_t i = 0; i < info.size(); ++i)
    {
        std::cout<<"device: "<<i<<" "<<info[i]<<std::endl;
    }
}

void emu_Emulator_getDeviceFirmwareCodes() {//获取设备固件代码列表
    std::vector<std::string> info = state->launcher->get_device_firwmare_codes();

    for (size_t i = 0; i < info.size(); ++i)
    {
        std::cout<<"firwmare_code "<<i<<" "<<info[i]<<std::endl;
    }
}

void emu_Emulator_setDeviceName(int id, const char* new_name) {//设置指定设备id的名称
    state->launcher->set_device_name(id, new_name);
}

void emu_Emulator_rescanDevices() {//重新扫描设备，会把当前设备设置为0
    state = std::make_unique<eka2l1::sdl2::emulator>();
    emulator_entry_rescan_device(*state);
}


int emu_Emulator_getCurrentDevice() {//获取当前设备id
    return state->launcher->get_current_device();
}

bool emu_Emulator_doesRomNeedRPKG(const char *rom_path) {//判断设备rom是否需要rpkg
    const bool result = state->launcher->does_rom_need_rpkg(rom_path);
    return result;
}

void emu_Emulator_uninstallPackage(int uid, int ext_index) {//卸载包，包括卸载app
    state->launcher->uninstall_package(uid, ext_index);
}

void emu_Emulator_mountSdCard(std::string path) {//通过指定路径，加载SD卡游戏，当前device必须是N-gage1，此时app列表就只有一个sd卡游戏
    state->launcher->mount_sd_card(path);
}

void emu_Emulator_loadConfig() {//加载配置
    state->launcher->load_config();
}

void emu_Emulator_setLanguage(int language_id) {//设置语言
    state->launcher->set_language(language_id);
}

void emu_Emulator_setRtosLevel(int level) {
    state->launcher->set_rtos_level(level);
}

void emu_Emulator_updateAppSetting(int uid) {//通过app id更新app设置
    state->launcher->update_app_setting(uid);
}

// extern "C" JNIEXPORT jobjectArray JNICALL
// Java_com_github_eka2l1_emu_Emulator_getAppIcon(JNIEnv *env, jclass clazz, jlong uid) {//通过app id获取app图标
//     jobjectArray jicons = state->launcher->get_app_icon(env, uid);
//     return jicons;
// }

// extern "C" JNIEXPORT jobjectArray JNICALL
// Java_com_github_eka2l1_emu_Emulator_getLanguageIds(//获取语言id列表返回给java
//     JNIEnv *env,
//     jclass clazz) {
//     std::vector<std::string> language_ids = state->launcher->get_language_ids();
//     jobjectArray jlanguage_ids = env->NewObjectArray(static_cast<size_t>(language_ids.size()),
//         env->FindClass("java/lang/String"),
//         nullptr);
//     for (size_t i = 0; i < language_ids.size(); ++i)
//         env->SetObjectArrayElement(jlanguage_ids, i, env->NewStringUTF(language_ids[i].c_str()));
//     return jlanguage_ids;
// }

// extern "C" JNIEXPORT jobjectArray JNICALL
// Java_com_github_eka2l1_emu_Emulator_getLanguageNames(//获取语言名称列表返回给java
//     JNIEnv *env,
//     jclass clazz) {
//     std::vector<std::string> language_names = state->launcher->get_language_names();
//     jobjectArray jlanguage_names = env->NewObjectArray(static_cast<size_t>(language_names.size()),
//         env->FindClass("java/lang/String"),
//         nullptr);
//     for (size_t i = 0; i < language_names.size(); ++i)
//         env->SetObjectArrayElement(jlanguage_names, i, env->NewStringUTF(language_names[i].c_str()));
//     return jlanguage_names;
// }

// extern "C"
// JNIEXPORT void JNICALL
// Java_com_github_eka2l1_emu_Emulator_setScreenParams(JNIEnv *env, jclass clazz,//设置屏幕参数
//                                                     int32_t background_color, int32_t scale_ratio,
//                                                     int32_t scale_type, int32_t gravity,
//                                                     jstring bg_img_path, jfloat bg_img_opacity,
//                                                     jboolean bg_img_keep_aspect) {
//     const char *cstr = env->GetStringUTFChars(bg_img_path, nullptr);
//     std::string cpath = std::string(cstr);
//     env->ReleaseStringUTFChars(bg_img_path, cstr);

//     state->launcher->set_screen_params(background_color, scale_ratio, scale_type, gravity, cpath, bg_img_opacity, bg_img_keep_aspect);
// }

void emu_Emulator_setScreenParams(//设置屏幕参数
    int32_t background_color, int32_t scale_ratio,
    int32_t scale_type, int32_t gravity) {
    state->launcher->set_screen_params(background_color, scale_ratio, scale_type, gravity, "", 1.0f, true);
}

void emu_Emulator_submitInput(std::string text) {//把字符输入传给emu
    state->launcher->on_finished_text_input(text, false);
}

// extern "C"
// JNIEXPORT void JNICALL
// Java_com_github_eka2l1_emu_EmulatorCamera_onCaptureImageDelivered(JNIEnv *env, jclass clazz,
//                                                                   int32_t index, jbyteArray raw_data,
//                                                                   int32_t error_code) {
//     eka2l1::drivers::camera::collection_android *collection = reinterpret_cast
//             <eka2l1::drivers::camera::collection_android *>(eka2l1::drivers::camera::get_collection());

//     if (collection) {
//         jboolean is_data_copy = false;
//         size_t data_size = env->GetArrayLength(raw_data);
//         jbyte *data = env->GetByteArrayElements(raw_data, &is_data_copy);
//         collection->handle_image_capture_delivered(index, data, static_cast<int>(data_size), error_code);
//         env->ReleaseByteArrayElements(raw_data, data, 0);
//     }
// }
// extern "C"
// JNIEXPORT void JNICALL
// Java_com_github_eka2l1_emu_EmulatorCamera_onFrameViewfinderDelivered(JNIEnv *env, jclass clazz,
//                                                                      int32_t index,
//                                                                      jbyteArray raw_data,
//                                                                      int32_t error_code) {
//     eka2l1::drivers::camera::collection_android *collection = reinterpret_cast
//             <eka2l1::drivers::camera::collection_android *>(eka2l1::drivers::camera::get_collection());

//     if (collection) {
//         jboolean is_data_copy = false;
//         size_t data_size = env->GetArrayLength(raw_data);
//         jbyte *data = env->GetByteArrayElements(raw_data, &is_data_copy);
//         collection->handle_frame_viewfinder_delivered(index, data, static_cast<int>(data_size), error_code);
//         env->ReleaseByteArrayElements(raw_data, data, 0);
//     }
// }
// extern "C"
// JNIEXPORT jboolean JNICALL
// Java_com_github_eka2l1_emu_EmulatorCamera_doesCameraAllowNewFrame(JNIEnv *env, jclass clazz,
//                                                                   int32_t index) {
//     eka2l1::drivers::camera::collection_android *collection = reinterpret_cast
//             <eka2l1::drivers::camera::collection_android *>(eka2l1::drivers::camera::get_collection());

//     return collection->reserved_wants_new_frame(index);
// }

int emu_Emulator_installNGageGame(std::string path) {//通过ngage游戏文件夹安装ng1游戏，当前device必须是n-gage1
    return state->launcher->install_ngage_game(path);
}

void emu_Emulator_submitQuestionDialogResponse(int value) {//提交问题对话框的答案给emu
    state->launcher->on_question_dialog_finished(value);
}

// extern "C"
// JNIEXPORT jobjectArray JNICALL
// Java_com_github_eka2l1_emu_Emulator_getSuccessInstalledLicenseGames(JNIEnv *env, jclass clazz) {//获取成功安装证书的游戏列表
//     return retrieve_jni_string_array_from_vector(env, state->launcher->get_success_installed_license_games());
// }

// extern "C"
// JNIEXPORT jobjectArray JNICALL
// Java_com_github_eka2l1_emu_Emulator_getFailedInstalledLicenseGames(JNIEnv *env, jclass clazz) {//
//     return retrieve_jni_string_array_from_vector(env, state->launcher->get_failed_installed_license_games());
// }

bool emu_Emulator_installNG2Licenses(std::string content) {
    return state->launcher->install_ng2_game_licenses(content);
}

void emu_Emulator_setCurrentMMCID(std::string new_mmcid) {
    state->launcher->set_current_mmc_id(new_mmcid);
}

bool emu_Emulator_saveScreenshotTo(std::string file_path) {
    return state->launcher->save_screenshot_to(file_path);
}

/**
 * 去除字符串首尾的空格
 */
std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, last - first + 1);
}

/**
 * 从文件中读取分辨率（增强版）
 * 支持格式: "640 480", "640,480", "640x480", "640*480"
 * @param filename 文件名
 * @param width 输出参数，存储宽度
 * @param height 输出参数，存储高度
 * @return 成功返回 true，失败返回 false
 */
bool readResolutionAdvanced(const std::string& filename, int& width, int& height) {
    std::ifstream file(filename);
    
    if (!file.is_open()) {
        std::cerr << "无法打开文件: " << filename << std::endl;
        return false;
    }
    
    std::string line;
    if (!std::getline(file, line)) {
        std::cerr << "读取文件失败或文件为空" << std::endl;
        return false;
    }
    
    // 去除首尾空格
    line = trim(line);
    
    if (line.empty()) {
        std::cerr << "文件内容为空" << std::endl;
        return false;
    }
    
    // 尝试多种分隔符
    char delimiters[] = {' ', ',', 'x', '*', '\t'};
    size_t pos = std::string::npos;
    
    for (char delim : delimiters) {
        pos = line.find(delim);
        if (pos != std::string::npos) break;
    }
    
    if (pos == std::string::npos) {
        std::cerr << "格式错误：找不到有效的分隔符" << std::endl;
        return false;
    }
    
    try {
        std::string wStr = trim(line.substr(0, pos));
        std::string hStr = trim(line.substr(pos + 1));
        
        width = std::stoi(wStr);
        height = std::stoi(hStr);
        
        // 基本验证
        if (width <= 0 || height <= 0) {
            std::cerr << "分辨率值必须为正数" << std::endl;
            return false;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "解析整数失败: " << e.what() << std::endl;
        return false;
    }
    
    return true;
}