#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdbool.h>
#include <dirent.h>
#include <iostream>
#include <string>
#include <vector>

#include <EGL/egl.h>
#include <gbm.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <unistd.h>
#include <fcntl.h>
struct gbm_device *gbm_device;
struct gbm_surface *gbm_surface;
int g_device_fd;
drmModeConnectorPtr connector = NULL;
drmModeCrtcPtr crtc = NULL;
drmModeEncoderPtr encoder = NULL;
drmModeResPtr res = NULL;
SDL_Joystick *g_joystick=NULL;

int ngageDev=0;
int v3Dev=1;
std::string v3Dir;
std::string ngageDir;

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480
#define MENU_ITEMS 4
#define MENU_ITEM_WIDTH 250
#define MENU_ITEM_HEIGHT 50
#define VISIBLE_ITEMS 12  // 可见文件数量
#define VISIBLE_LINES 12

#define M_A 1
#define M_B 0
#define M_X 3
#define M_Y 2
#define M_start 7
#define M_select 6
#define M_L 4
#define M_L2 20
#define M_R 5
#define M_R2 21
#define M_UP 13
#define M_DOWN 14
#define M_LEFT 15
#define M_RIGHT 16
#define M_QUIT1 8
#define M_QUIT2 9

extern std::vector<std::string> getDeviceFirmwareCodes();
extern std::vector<std::string> getDevices();
extern std::vector<std::string> getPackages();
extern std::vector<std::string> getApps(std::string);
extern int installApp(std::string path);
extern void emu_Emulator_setDirectory(//设置当前工作路径，就是设置pwd
    const char *cstr);

extern bool emu_Emulator_startNative(uint32_t rotate=0);
extern std::vector<std::string> getPackages();
extern void emu_Emulator_uninstallPackage(int uid, int ext_index);
extern void emu_Emulator_setCurrentDevice(int32_t id, bool is_temp);

const char* menu_items[MENU_ITEMS] = {
    "安装游戏",
    //"安装设备",
    "扫描游戏",
    "刷新固件",
    "扫描n-gage1游戏"
};

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
TTF_Font* font = NULL;
int selected=0;

// 文件列表结构
typedef struct {
    char** files;
    int count;
    int selected;
    int startIndex;  // 起始显示索引
} FileList;

std::string get_last_substring_exact(const std::string& str, const std::string& delimiter) {
    size_t pos = str.rfind(delimiter);
    if (pos == std::string::npos) {
        return str;
    }
    return str.substr(pos + delimiter.length());
}

std::string get_first_substring_exact(const std::string& str, const std::string& delimiter) {
    size_t pos = str.rfind(delimiter);
    if (pos == std::string::npos) {
        return str;
    }
    return str.substr(0, pos);
}


int getExtIndex(std::vector<std::string>& ps, std::string& uid)
{
    for (int i = 0; i < ps.size(); i+=3)
    {
        if(uid==ps[i])
        {
            return atoi(ps[i+1].c_str());
        }
    }

    return -1;
}

// 初始化 SDL 和 TTF
bool initialize() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0) {
        printf("SDL 初始化失败: %s\n", SDL_GetError());
        return false;
    }

    if (SDL_NumJoysticks() >= 1)
	{
		g_joystick = SDL_JoystickOpen(0);
		if (g_joystick == NULL)
		{
			std::cout<<"Unable to open joystick."<<std::endl;
			exit(0);
		}
	}

    if (TTF_Init() == -1) {
        printf("TTF 初始化失败: %s\n", TTF_GetError());
        return false;
    }

    window = SDL_CreateWindow("SDL2 菜单程序",
                              SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED,
                              WINDOW_WIDTH,
                              WINDOW_HEIGHT,
                              SDL_WINDOW_SHOWN);
    if (!window) {
        printf("窗口创建失败: %s\n", SDL_GetError());
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        printf("渲染器创建失败: %s\n", SDL_GetError());
        return false;
    }

    font = TTF_OpenFont("MiSans-Normal.ttf", 24); // 确保字体文件存在或替换为系统可用字体
    if (!font) {
        printf("字体加载失败: %s\n", TTF_GetError());
        // 尝试使用默认字体
        font = TTF_OpenFont(NULL, 24);
        if (!font) {
            printf("默认字体加载失败: %s\n", TTF_GetError());
            return false;
        }
    }

    return true;
}

// 清理资源
void cleanup() {
    if (font) TTF_CloseFont(font);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    if(g_joystick)
	{
		SDL_JoystickClose(g_joystick);
	}

    gbm_surface_destroy(gbm_surface);
	drmModeFreeCrtc(crtc);
	drmModeFreeEncoder(encoder);
    drmModeFreeConnector(connector);
	drmModeFreeResources(res);
    gbm_device_destroy(gbm_device);
    close(g_device_fd);


    TTF_Quit();
    SDL_Quit();
}

// 渲染菜单
void render_menu(int selected_index) {
    // 清屏
    SDL_SetRenderDrawColor(renderer, 190, 190, 190, 255);
    SDL_RenderClear(renderer);

    // 计算菜单位置（居中）
    int start_y = (WINDOW_HEIGHT - (MENU_ITEMS * MENU_ITEM_HEIGHT)) / 2;

    
    
    // 渲染每个菜单项
    for (int i = 0; i < MENU_ITEMS; i++) {
        // 绘制菜单项背景（可选）
        SDL_Rect bg_rect = {
            (WINDOW_WIDTH - MENU_ITEM_WIDTH) / 2,
            start_y + i * MENU_ITEM_HEIGHT,
            MENU_ITEM_WIDTH,
            MENU_ITEM_HEIGHT
        };
        SDL_SetRenderDrawColor(renderer, 
                            (i == selected_index) ? 220 : 211, 
                            (i == selected_index) ? 220 : 211, 
                            (i == selected_index) ? 255 : 211, 
                            255);
        SDL_RenderFillRect(renderer, &bg_rect);


        SDL_Color color = (i == selected_index) ? 
                          (SDL_Color){0, 0, 255, 255} :  // 选中项为蓝色
                          (SDL_Color){0, 0, 0, 255};     // 非选中项为黑色
        
        SDL_Surface* surface = TTF_RenderUTF8_Blended(font, menu_items[i], color);
        if (surface) {
            SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
            if (texture) {
                int text_width = surface->w;
                int text_height = surface->h;
                int x = (WINDOW_WIDTH - text_width) / 2;
                int y = start_y + i * MENU_ITEM_HEIGHT + (MENU_ITEM_HEIGHT - text_height) / 2;
                
                SDL_Rect dst_rect = {x, y, text_width, text_height};
                SDL_RenderCopy(renderer, texture, NULL, &dst_rect);
                
                SDL_DestroyTexture(texture);
            }
            SDL_FreeSurface(surface);
        }
        
    }
    
    SDL_RenderPresent(renderer);
}


// 渲染文件列表
void render_file_list(FileList* fileList) {
    SDL_SetRenderDrawColor(renderer, 190, 190, 190, 255);
    SDL_RenderClear(renderer);
 
    int start_y = 50;
    int item_height = 30;
 
    // 计算当前显示的文件范围
    int endIndex = fileList->startIndex + VISIBLE_ITEMS;
    if (endIndex > fileList->count) {
        endIndex = fileList->count;
    }
 
    for (int i = fileList->startIndex; i < endIndex; i++) {
        SDL_Color color = (i == fileList->selected) ? 
                          (SDL_Color){0, 0, 255, 255} :  // 选中项为蓝色
                          (SDL_Color){0, 0, 0, 255};     // 非选中项为黑色
        
        SDL_Surface* surface = TTF_RenderUTF8_Blended(font, fileList->files[i], color);
        if (surface) {
            SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
            if (texture) {
                int text_width = surface->w;
                int text_height = surface->h;
                int x = (WINDOW_WIDTH - text_width) / 2;
                int y = start_y + (i - fileList->startIndex) * item_height + (item_height - text_height) / 2;
                
                SDL_Rect dst_rect = {x, y, text_width, text_height};
                SDL_RenderCopy(renderer, texture, NULL, &dst_rect);
                
                SDL_DestroyTexture(texture);
            }
            SDL_FreeSurface(surface);
        }
    }
 
    SDL_RenderPresent(renderer);
}

// 渲染多行文本
void render_multiline_text(const std::vector<std::string> lines, int startLine) {
    SDL_SetRenderDrawColor(renderer, 190, 190, 190, 255);
    SDL_RenderClear(renderer);
 
    

    int y = 50;
    int line_height = 30;

    int lineCount=lines.size();
    int endLine = startLine + VISIBLE_LINES;
    if (endLine > lineCount) {
        endLine = lineCount;
    }
 
    
    for (int i = startLine; i < endLine; i++) {
        SDL_Color color = (i == selected) ? 
                          (SDL_Color){0, 0, 255, 255} :  // 选中项为蓝色
                          (SDL_Color){0, 0, 0, 255}; 

        SDL_Surface* surface = TTF_RenderUTF8_Blended(font, lines[i].c_str(), color);
        if (surface) {
            SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
            if (texture) {
                int text_width = surface->w;
                int text_height = surface->h;
                int x = (WINDOW_WIDTH - text_width) / 2;
                
                SDL_Rect dst_rect = {x, y, text_width, text_height};
                SDL_RenderCopy(renderer, texture, NULL, &dst_rect);
                
                SDL_DestroyTexture(texture);
            }
            SDL_FreeSurface(surface);
        }
        y += line_height;
    }
 
    SDL_RenderPresent(renderer);
}

// 渲染单行文本
void render_multiline_text(const std::string line) {
    int y = (WINDOW_HEIGHT - (MENU_ITEMS * MENU_ITEM_HEIGHT)) / 2;

    SDL_Color color =(SDL_Color){0, 0, 255, 255};

    // 绘制菜单项背景（可选）
    SDL_Rect bg_rect = {
        (WINDOW_WIDTH - MENU_ITEM_WIDTH) / 2,
        y,
        MENU_ITEM_WIDTH,
        MENU_ITEM_HEIGHT*MENU_ITEMS
    };
    SDL_SetRenderDrawColor(renderer, 180, 180, 180, 255);
    SDL_RenderFillRect(renderer, &bg_rect);
                        

    SDL_Surface* surface = TTF_RenderUTF8_Blended(font, line.c_str(), color);
    if (surface) {
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        if (texture) {
            int text_width = surface->w;
            int text_height = surface->h;
            int x = (WINDOW_WIDTH - text_width) / 2;
            
            SDL_Rect dst_rect = {x, y+(MENU_ITEM_HEIGHT*MENU_ITEMS-text_height)/2, text_width, text_height};
            SDL_RenderCopy(renderer, texture, NULL, &dst_rect);
            
            SDL_DestroyTexture(texture);
        }
        SDL_FreeSurface(surface);
    }
    
 
    SDL_RenderPresent(renderer);
}
 
// 获取当前目录下的文件列表
FileList get_file_list() {
    FileList fileList = {NULL, 0, 0};
    DIR *dir;
    struct dirent *ent;
    std::string d("./app/");
    if ((dir = opendir(d.c_str())) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            if (ent->d_type == DT_REG) { // 只处理普通文件
                fileList.files = (char**)realloc(fileList.files, (fileList.count + 1) * sizeof(char*));
                fileList.files[fileList.count] = strdup(ent->d_name);
                fileList.count++;
            }
        }
        closedir(dir);
    }
    return fileList;
}
 
// 释放文件列表
void free_file_list(FileList* fileList) {
    for (int i = 0; i < fileList->count; i++) {
        free(fileList->files[i]);
    }
    free(fileList->files);
}

bool issure(const std::string& line, int uid, int extindex)
{
    bool running=true;
    while (running) 
    {
        SDL_Event event;
        while (SDL_PollEvent(&event)) 
        {
            switch (event.type)
            {
                case SDL_QUIT: 
                    running = false;
                    continue;

                case SDL_JOYBUTTONDOWN:
                {
                    int key = event.jbutton.button;
                    switch (key) {
                        case M_A:
                            emu_Emulator_uninstallPackage(uid, extindex);
                            return true;
                        case M_B:
                        case M_QUIT1:
                        case M_QUIT2:
                            running = false;
                            break;
                    }
                }
            }
        }
        render_multiline_text(std::string("卸载:"+get_first_substring_exact(line, "uid:")+"?是(A), 取消(B)"));
        SDL_Delay(16); // 约60FPS
    }

    return false;
}
 
// 执行选中的菜单项操作
void execute_selected_item(int selected_index) {
    switch (selected_index) {
        case 0: 
        {
            FileList fileList = get_file_list();
            bool running = true;
            while (running) 
            {
                SDL_Event event;
                while (SDL_PollEvent(&event)) 
                {
                    switch (event.type)
			        {
                        case SDL_QUIT: 
                            running = false;
                            continue;
                        case SDL_JOYHATMOTION:
                            if ( event.jhat.value == SDL_HAT_UP )
                            {
                                event.type=SDL_JOYBUTTONDOWN;
                                event.jbutton.button=M_UP;
                                event.jbutton.state = SDL_PRESSED;
                                
                            }
                            
                            else if ( event.jhat.value == SDL_HAT_DOWN )
                            {
                                
                                event.type=SDL_JOYBUTTONDOWN;
                                event.jbutton.button=M_DOWN;
                                event.jbutton.state = SDL_PRESSED;
                            }
                            
                            else if ( event.jhat.value == SDL_HAT_LEFT )
                            {
                                
                                event.type=SDL_JOYBUTTONDOWN;
                                event.jbutton.button=M_LEFT;
                                event.jbutton.state = SDL_PRESSED;
                            }
                            
                            else if ( event.jhat.value == SDL_HAT_RIGHT )
                            {
                                
                                event.type=SDL_JOYBUTTONDOWN;
                                event.jbutton.button=M_RIGHT;
                                event.jbutton.state = SDL_PRESSED;
                            }
                            
                            else
                            {
                                break;
                                
                            }

                        case SDL_JOYBUTTONDOWN:
                        {
                            int key = event.jbutton.button;
                            switch (key) {
                                case M_UP:
                                    if (fileList.selected > fileList.startIndex) {
                                        fileList.selected--;
                                    } else if (fileList.startIndex > 0) {
                                        fileList.startIndex--;
                                    }
                                    break;
                                case M_DOWN:
                                    if (fileList.selected < fileList.count - 1) {
                                        fileList.selected++;
                                        if (fileList.selected - fileList.startIndex >= VISIBLE_ITEMS) {
                                            fileList.startIndex++;
                                        }
                                    }
                                    break;
                                case M_A:
                                {
                                    printf("选中文件: %s\n", fileList.files[fileList.selected]);
                                    render_multiline_text(std::string("正在安装:")+fileList.files[fileList.selected]+"...");
                                    installApp(std::string("./app/")+fileList.files[fileList.selected]);
                                    //安装完立即刷新.st启动文件
                                    std::string cmd=std::string("./native 3 ")+std::to_string(v3Dev)+" "+v3Dir;
                                    std::string cmd2=std::string("./native 3 ")+std::to_string(ngageDev)+" "+ngageDir;


                                    if (system(cmd.c_str()) == -1 || system(cmd2.c_str()) == -1 ) {
                                        // 处理系统调用失败
                                        render_multiline_text("安装失败");
                                    }
                                    else
                                    {
                                        render_multiline_text("安装完成!");
                                    }
                                        
                                    SDL_Delay(1500);
                                }
                                    break;
                                case M_B:
                                case M_QUIT1:
                                case M_QUIT2:
                                    running = false;
                                    break;
                            }
                        }
                    }
                }
                render_file_list(&fileList);
                SDL_Delay(16); // 约60FPS
            }
            free_file_list(&fileList);
            break;
        }
        case 1:
        {   
            emu_Emulator_setCurrentDevice(v3Dev, true);
            selected=0;
            std::vector<std::string> info=getDeviceFirmwareCodes();
            std::vector<std::string> info1=getDevices();

            std::vector<std::string> dev_phone;
            for(int i=0; i<info1.size(); i++)
            {
                dev_phone.push_back(info1[i]+" : "+info[i]);
            }
            dev_phone.insert(dev_phone.begin(), "------手机型号 : 固件代码------");

            std::vector<std::string> info3=getApps(v3Dir.c_str());
            std::vector<std::string> ps=getPackages();
            info3.insert(info3.begin(), std::string("---------已安装软件(")+std::to_string(info3.size())+")---------");
            info3.push_back("------------------------");

            //info.insert(info.end(), info1.begin(), info1.end());
            //info.insert(info.end(), info2.begin(), info2.end());
            dev_phone.insert(dev_phone.end(), info3.begin(), info3.end());
            int lineCount=dev_phone.size();
            bool running = true;
            int startLine = 0;
            while (running)
            {
                SDL_Event event;
                while (SDL_PollEvent(&event)) 
                {
                    switch (event.type)
                    {
                        case SDL_QUIT: 
                            running = false;
                            continue;
                        case SDL_JOYHATMOTION:
                            if ( event.jhat.value == SDL_HAT_UP )
                            {
                                event.type=SDL_JOYBUTTONDOWN;
                                event.jbutton.button=M_UP;
                                event.jbutton.state = SDL_PRESSED;
                                
                            }
                            
                            else if ( event.jhat.value == SDL_HAT_DOWN )
                            {
                                
                                event.type=SDL_JOYBUTTONDOWN;
                                event.jbutton.button=M_DOWN;
                                event.jbutton.state = SDL_PRESSED;
                            }
                            
                            else if ( event.jhat.value == SDL_HAT_LEFT )
                            {
                                
                                event.type=SDL_JOYBUTTONDOWN;
                                event.jbutton.button=M_LEFT;
                                event.jbutton.state = SDL_PRESSED;
                            }
                            
                            else if ( event.jhat.value == SDL_HAT_RIGHT )
                            {
                                
                                event.type=SDL_JOYBUTTONDOWN;
                                event.jbutton.button=M_RIGHT;
                                event.jbutton.state = SDL_PRESSED;
                            }
                            
                            else
                            {
                                break;
                                
                            }

                        case SDL_JOYBUTTONDOWN:
                        {
                            int key = event.jbutton.button;
                            if (key == M_B) {
                                running = false;
                            } else if(key == M_A){
                                std::string uid=get_last_substring_exact(dev_phone[selected], "uid:");
                                if(uid!=dev_phone[selected])//卸载
                                {
                                    if(issure(dev_phone[selected], atoi(uid.c_str()), getExtIndex(ps, uid)))//卸载成功，依赖
                                    {
                                        running = false;
                                    }
                                }
                            }else if (key == M_UP) {
                                if (selected > startLine) {
                                    selected--;
                                }else if (startLine > 0) {
                                    startLine--;
                                }
                            } else if (key == M_DOWN) {
                                if (selected < lineCount - 1) {
                                    selected++;
                                    if (selected - startLine >= VISIBLE_LINES) {
                                        startLine++;
                                    }
                                }
                            }
                        }
                    }
                }
                render_multiline_text(dev_phone, startLine);
                SDL_Delay(16); // 约60FPS
            }
            break;
        }
            
        case 2:
        {
            printf("执行: 刷新固件\n");
            render_multiline_text("正在刷新固件...");
            std::string cmd=std::string("./native 5");
            if (system(cmd.c_str()) == -1) {
                // 处理系统调用失败
                render_multiline_text("刷新失败");
            }
            else
                render_multiline_text("刷新完成!");
            SDL_Delay(1500);
            break;
        }
            
        case 3:
        {
            printf("执行: 扫描ngage游戏\n");
            render_multiline_text("正在扫描ngage游戏...");
            std::string cmd=std::string("./native 3 ")+std::to_string(ngageDev)+" "+ngageDir;
            if (system(cmd.c_str()) == -1) {
                // 处理系统调用失败
                render_multiline_text("扫描失败");
            }
            else
                render_multiline_text("扫描完成!");
           
            SDL_Delay(1500);
            break;
        }
            
    }

}

void init_gbm()
{
	gbm_surface=NULL;
	gbm_device=NULL;
	g_device_fd=0;

	g_device_fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
	std::cout << "g_device_fd:"<<g_device_fd<<std::endl;
	res = drmModeGetResources(g_device_fd);

    // connector = drmModeGetConnector(g_device_fd, res->connectors[1]);
    // encoder = drmModeGetEncoder(g_device_fd, res->encoders[2]);
    // crtc = drmModeGetCrtc(g_device_fd, res->crtcs[1]);

	for (int i = 0; i < res->count_connectors; i++) {
		connector = drmModeGetConnector(g_device_fd, res->connectors[i]);
		
		if (connector->connection == DRM_MODE_CONNECTED) {  
            printf("Connector %d (type %d, status %d) supports:\n",  
                   i, connector->connector_type, connector->connection);  
            for (int j = 0; j < connector->count_modes; j++) {  
                printf("  Mode %d: %dx%d, flag: 0x%08x\n",  
                       j, connector->modes[j].hdisplay, connector->modes[j].vdisplay,  
                       connector->modes[j].flags);  
            }  
        }  

		// find a connected connection
		if (connector->connection == DRM_MODE_CONNECTED)
			break;

		drmFree(connector);
		connector = NULL;
	}

	encoder = drmModeGetEncoder(g_device_fd, connector->encoder_id);
	crtc = drmModeGetCrtc(g_device_fd, encoder->crtc_id);

	//drmSetMaster(g_device_fd);
	gbm_device = gbm_create_device(g_device_fd);
	uint32_t surface_fmt = GBM_FORMAT_ARGB8888;//flip
	if (!gbm_device_is_format_supported(gbm_device, surface_fmt, GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING)) {
        std::cout<<"GBM surface format not supported. Trying anyway."<<std::endl;
    }
	
	gbm_surface=gbm_surface_create(gbm_device, crtc->mode.hdisplay, crtc->mode.vdisplay, surface_fmt, GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
		
	std::cout<<"gbm_surface: "<<gbm_surface<<std::endl;
}

int main(int argc, char* argv[]) {
    if (!initialize()) {
        cleanup();
        return 1;
    }

    if(argc==5)
    {
        v3Dev=atoi(argv[1]);//v3设备序号
        ngageDev=atoi(argv[2]);//ng1设备序号
        v3Dir=argv[3];//v3启动文件生成目录
        ngageDir=argv[4];//ng1启动文件生成目录
    }

    bool running = true;
    int selected_index = 0;
    SDL_Event event;

    emu_Emulator_setDirectory(".");
    init_gbm();
    emu_Emulator_startNative();//此时初始化模拟器时，需要有正常的devices.yml，不然安装游戏，扫描游戏，卸载游戏会失败

    while (running) 
    {
        // 处理事件
        while (SDL_PollEvent(&event)) 
        {

            switch (event.type)
			{
                case SDL_QUIT: 
                    running = false;
                    continue;
                case SDL_JOYHATMOTION:
					if ( event.jhat.value == SDL_HAT_UP )
					{
						event.type=SDL_JOYBUTTONDOWN;
						event.jbutton.button=M_UP;
						event.jbutton.state = SDL_PRESSED;
						
					}
					
					else if ( event.jhat.value == SDL_HAT_DOWN )
					{
						
						event.type=SDL_JOYBUTTONDOWN;
						event.jbutton.button=M_DOWN;
						event.jbutton.state = SDL_PRESSED;
					}
					
					else if ( event.jhat.value == SDL_HAT_LEFT )
					{
						
						event.type=SDL_JOYBUTTONDOWN;
						event.jbutton.button=M_LEFT;
						event.jbutton.state = SDL_PRESSED;
					}
					
					else if ( event.jhat.value == SDL_HAT_RIGHT )
					{
						
						event.type=SDL_JOYBUTTONDOWN;
						event.jbutton.button=M_RIGHT;
						event.jbutton.state = SDL_PRESSED;
					}
					
					else
					{
						break;
						
					}

				case SDL_JOYBUTTONDOWN:
                {
                    int key = event.jbutton.button;
                    switch (key) {
                        case M_UP:
                            selected_index = (selected_index - 1 + MENU_ITEMS) % MENU_ITEMS;
                            break;
                        case M_DOWN:
                            selected_index = (selected_index + 1) % MENU_ITEMS;
                            break;
                        case M_A:
                            execute_selected_item(selected_index);
                            break;
                        case M_B:
                        case M_QUIT1:
                        case M_QUIT2:
                            running = false;
                            break;
                    }
                }
            }
        }

        render_menu(selected_index);
        SDL_Delay(16); // 约60FPS
    }

    cleanup();
    return 0;
}