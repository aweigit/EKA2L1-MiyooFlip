
#include <string>

#include <state.h>
#include <thread.h>
#include <common/fileutils.h>
#include <common/path.h>
#include <drivers/audio/audio.h>
#include <drivers/camera/camera_collection.h>
#include <drivers/graphics/graphics.h>
#include <common/log.h>

#if EKA2L1_ARCH(ARM)
#include <cpu/12l1r/tests/test_entry.h>
#endif

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <iostream>
#include <unistd.h>

#include <EGL/egl.h>

#include <fcntl.h>
#include "cJSON.h"

// #define CATCH_CONFIG_RUNNER
// #define CATCH_CONFIG_ANDROID_LOGWRITE

// #include <catch2/catch.hpp>
SDL_Joystick *g_joystick=NULL;
int source_width=640,source_height=480;
int display_width=1024,display_height=768;
bool capturing = true;
int rotate=0;

extern void emu_Emulator_setDirectory(//设置当前工作路径，就是设置pwd
    const char *cstr);

extern bool emu_Emulator_startNative(uint32_t rotate=0);
extern bool emu_Emulator_startNative_install(const char*rom, const char*pkg);//生成emu，入口点

extern int emu_Emulator_installApp(std::string path);
extern void emu_Emulator_surfaceChanged(void* surface,//根据surface生成native window，并开始窗口线程
    int width, int height);
extern void emu_Emulator_launchApp(int uid);
extern void surfaceRedrawNeeded() ;
extern void emu_Emulator_pressKey(int key, int keystat);
extern void emu_Emulator_rescanDevices();

extern void emu_Emulator_getDeviceFirmwareCodes();
extern void emu_Emulator_getDevices();
extern void emu_Emulator_setScreenParams(//设置屏幕参数
    int background_color, int scale_ratio,
    int scale_type, int gravity);
extern int emu_Emulator_installDevice(std::string rpkg_path,//安装设备，参数为固件包路径(ROM文件和RPKG文件)
	std::string rom_path, bool install_rpkg);
extern void emu_Emulator_setCurrentDevice(int32_t id, bool is_temp);
extern void emu_Emulator_rotate(uint32_t rotate);

extern std::vector<std::string> getApps(std::string);
extern bool readResolutionAdvanced(const std::string& filename, int& width, int& height);

#define SDL_AXIS_TRIGGERLEFT 100
#define SDL_AXIS_TRIGGERRIGHT 101

//为sdl键值起个别名
int KEY_left=SDLK_q;
int KEY_right=SDLK_w;
int KEY_ok=SDLK_RETURN;
int KEY_star=SDLK_e;
int KEY_pound=SDLK_r;
int KEY_1=SDLK_1;
int KEY_3=SDLK_3;
int KEY_7=SDLK_7;
int KEY_9=SDLK_9;
int KEY_0=SDLK_0;
int KEY_4=SDLK_4;
int KEY_6=SDLK_6;

//默认映射
/*
{
	"左键":"X",
    "右键":"B",
    "OK":"A",
	"*":"SELECT",
	"#":"START",
	"0":"Y",
	"1":"L",
	"3":"R",
	"7":"L2",
	"9":"R2",
	"4":"L3",
	"6":"R3"
}

*/

//这个地方会根据配置变化
int BUTTON_Y=KEY_left;
int BUTTON_A=KEY_right;
int BUTTON_X=KEY_ok;
int BUTTON_BACK=KEY_star;
int BUTTON_START=KEY_pound;
int BUTTON_LEFTSHOULDER=KEY_1;
int BUTTON_RIGHTSHOULDER=KEY_3;
int TRIGGERLEFT=KEY_7;
int TRIGGERRIGHT=KEY_9;
int BUTTON_B=KEY_0;

int BUTTON_LEFTSTICK=KEY_4;
int BUTTON_RIGHTSTICK=KEY_6;

static void wout(const char *filename, const char *text, int r=-2)
{
  FILE *file = fopen(filename, "a");
    if (!file) {
        perror("文件打开失败");
        return;
    }
 
    // 2. 写入字符串
    // 方法1：使用fprintf（支持格式化）
    if (fprintf(file, "%s %x\n", text, r) < 0) {
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


void setEmuKey(const char * name, int key)//再加个参数为模拟器需要的键值，如左键key_left
{
	if(strcmp(name,"X")==0)
	{
		BUTTON_X=key;
	}
	else if(strcmp(name,"B")==0)
	{
		BUTTON_B=key;
	}
	else if(strcmp(name,"A")==0)
	{
		BUTTON_A=key;
	}
	else if(strcmp(name,"SELECT")==0)
	{
		BUTTON_BACK=key;
	}
	else if(strcmp(name,"START")==0)
	{
		BUTTON_START=key;
	}
	else if(strcmp(name,"Y")==0)
	{
		BUTTON_Y=key;
	}
	else if(strcmp(name,"L")==0)
	{
		BUTTON_LEFTSHOULDER=key;
	}
	else if(strcmp(name,"R")==0)
	{
		BUTTON_RIGHTSHOULDER=key;
	}
	else if(strcmp(name,"L2")==0)
	{
		TRIGGERLEFT=key;
	}
	else if(strcmp(name,"R2")==0)
	{
		TRIGGERRIGHT=key;
	}
	else if(strcmp(name,"L3")==0)
	{
		BUTTON_LEFTSTICK=key;
	}
	else if(strcmp(name,"R3")==0)
	{
		BUTTON_RIGHTSTICK=key;
	}
	
}

void defaultKeymap()
{
    //默认映射
    BUTTON_Y=KEY_left;
    BUTTON_A=KEY_right;
    BUTTON_X=KEY_ok;
    BUTTON_BACK=KEY_star;
    BUTTON_START=KEY_pound;
    BUTTON_LEFTSHOULDER=KEY_1;
    BUTTON_RIGHTSHOULDER=KEY_3;
    TRIGGERLEFT=KEY_7;
    TRIGGERRIGHT=KEY_9;
    BUTTON_B=KEY_0;

	BUTTON_LEFTSTICK=KEY_4;
	BUTTON_RIGHTSTICK=KEY_6;
}

int loadConfig() {
    // 打开文件
    FILE *file = fopen("keymap.cfg", "r");
    if (file == NULL) {
		std::cout  << "打开keymap.cfg失败" << std::endl;
        return -1;
    }
 
    // 确定文件长度
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
 
    // 读取文件内容到字符串
    char *data = (char*)malloc(length + 1);
    fread(data, 1, length, file);
    data[length] = '\0';
    fclose(file);
 
    // 解析JSON字符串
    cJSON *json = cJSON_Parse(data);
    if (json == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            std::cout  << "解析json错误:" << error_ptr << std::endl;
        }
        cJSON_Delete(json);
        free(data);
        return -1;
    }
	
	try{
		// 使用cJSON对象
		cJSON *name = cJSON_GetObjectItem(json, "左键");
		setEmuKey(name->valuestring, KEY_left);
		
		name = cJSON_GetObjectItem(json, "右键");
		setEmuKey(name->valuestring, KEY_right);
		
		name = cJSON_GetObjectItem(json, "OK");
		setEmuKey(name->valuestring, KEY_ok);
		
		name = cJSON_GetObjectItem(json, "*");
		setEmuKey(name->valuestring, KEY_star);
		
		name = cJSON_GetObjectItem(json, "#");
		setEmuKey(name->valuestring, KEY_pound);
		
		name = cJSON_GetObjectItem(json, "0");
		setEmuKey(name->valuestring, KEY_0);
		
		name = cJSON_GetObjectItem(json, "1");
		setEmuKey(name->valuestring, KEY_1);
		
		name = cJSON_GetObjectItem(json, "3");
		setEmuKey(name->valuestring, KEY_3);
		
		name = cJSON_GetObjectItem(json, "7");
		setEmuKey(name->valuestring, KEY_7);
		
		name = cJSON_GetObjectItem(json, "9");
		setEmuKey(name->valuestring, KEY_9);

		name = cJSON_GetObjectItem(json, "4");
		setEmuKey(name->valuestring, KEY_4);

		name = cJSON_GetObjectItem(json, "6");
		setEmuKey(name->valuestring, KEY_6);
	}
	catch(std::exception& e)
	{
		
		defaultKeymap();
		
		std::cout << "解析json出错:"<<e.what() << std::endl;
		return -1;
	}
 
 
    // 清理工作
    cJSON_Delete(json);
    free(data);
 
    return 0;
}


int joy2emu(int joy)
{
	switch(joy)
	{
		case SDL_CONTROLLER_BUTTON_Y:
			return BUTTON_X;
		case SDL_CONTROLLER_BUTTON_A:
			return BUTTON_B;
		case SDL_CONTROLLER_BUTTON_X:
			return BUTTON_Y;
		case SDL_CONTROLLER_BUTTON_BACK:
			return BUTTON_BACK;
		case SDL_CONTROLLER_BUTTON_START:
			return BUTTON_START;
		case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
			return BUTTON_LEFTSHOULDER;
		case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
			return BUTTON_RIGHTSHOULDER;
		case SDL_AXIS_TRIGGERLEFT://注意这是自定义的摇杆的轴值，确认是否跟Button值重复
			return TRIGGERLEFT;
		case SDL_AXIS_TRIGGERRIGHT:
			return TRIGGERRIGHT;
		case SDL_CONTROLLER_BUTTON_B:
			return BUTTON_A;

		case SDL_CONTROLLER_BUTTON_LEFTSTICK:
			return BUTTON_LEFTSTICK;
		case SDL_CONTROLLER_BUTTON_RIGHTSTICK:
			return BUTTON_RIGHTSTICK;
	}
	
	return 0;
}

short joymouseX = 0;
short joymouseY = 0;
bool use_mouse = 0;
bool use_numpad = 0;
bool switch_ok = 0;
// mouse cursor image
unsigned char joymouseImage[374] =
{
	0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,1,2,2,1,0,0,0,0,0,0,0,0,0,
	0,0,0,0,1,2,2,1,0,0,0,0,0,0,0,0,0,
	0,0,0,0,1,2,2,1,0,0,0,0,0,0,0,0,0,
	0,0,0,0,1,2,2,1,1,1,0,0,0,0,0,0,0,
	0,0,0,0,1,2,2,1,2,2,1,1,1,0,0,0,0,
	0,0,0,0,1,2,2,1,2,2,1,2,2,1,1,0,0,
	0,0,0,0,1,2,2,1,2,2,1,2,2,1,2,1,0,
	1,1,1,0,1,2,2,1,2,2,1,2,2,1,2,2,1,
	1,2,2,1,1,2,2,2,2,2,2,2,2,1,2,2,1,
	1,2,2,2,1,2,2,2,2,2,2,2,2,2,2,2,1,
	0,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,
	0,0,1,2,2,2,2,2,2,2,2,2,2,2,2,2,1,
	0,0,1,2,2,2,2,2,2,2,2,2,2,2,2,2,1,
	0,0,0,1,2,2,2,2,2,2,2,2,2,2,2,2,1,
	0,0,0,1,2,2,2,2,2,2,2,2,2,2,2,1,0,
	0,0,0,0,1,2,2,2,2,2,2,2,2,2,2,1,0,
	0,0,0,0,1,2,2,2,2,2,2,2,2,2,2,1,0,
	0,0,0,0,0,1,2,2,2,2,2,2,2,2,1,0,0,
	0,0,0,0,0,1,2,2,2,2,2,2,2,2,1,0,0,
	0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,0,0
};

uint32_t frameDeadline = 0;

#define STICK_DEAD_ZONE 8000  // 死区阈值，可调
#define TRIGGER_PRESS_THRESHOLD   8000  // 按下阈值
#define TRIGGER_RELEASE_THRESHOLD 2000  // 释放阈值（必须 <= PRESS）
// 配置参数（可调整）
#define DEAD_ZONE      0.15f   // 摇杆死区（0.0 ～ 1.0）
#define SENSITIVITY    4.0f    // 每帧最大移动像素（越高越快）
#define USE_RADIAL_DZ  1       // 1 = 径向死区，0 = 轴向死区
static bool left_trigger_active  = false;
static bool right_trigger_active = false;

// 当前方向状态（避免重复打印/触发）
static bool up    = false;
static bool down  = false;
static bool left  = false;
static bool right = false;

// 存储当前轴值（用于帧更新或事件驱动）
static Sint16 left_x = 0;
static Sint16 left_y = 0;

//SDL_CONTROLLER_BUTTON_LEFTSTICK / RIGHTSTICK（按下摇杆）
//SDL_CONTROLLER_AXIS_TRIGGERLEFT / RIGHT L2、R2为扳机键，与摇杆相同，但是只有正值
//SDL_CONTROLLER_BUTTON_DPAD_UP 方向键

// 用于保存已连接的手柄
std::map<SDL_JoystickID, SDL_GameController*> g_gameControllers;

// 归一化单轴（仅用于轴向死区）
static float normalize_axis(Sint16 axis, float deadzone) {
    float val = axis / 32768.0f;
    if (fabsf(val) < deadzone) return 0.0f;
    float sign = (val > 0) ? 1.0f : -1.0f;
    return sign * ((fabsf(val) - deadzone) / (1.0f - deadzone));
}

void updateMouse_xy()
{
	// === 摇杆输入处理 ===
	Sint16 raw_x = SDL_GameControllerGetAxis(g_gameControllers[0], SDL_CONTROLLER_AXIS_LEFTX);
	Sint16 raw_y = SDL_GameControllerGetAxis(g_gameControllers[0], SDL_CONTROLLER_AXIS_LEFTY);

	float nx, ny;

#if USE_RADIAL_DZ
	// --- 径向死区（推荐用于 2D 移动）---
	float fx = raw_x / 32768.0f;
	float fy = raw_y / 32768.0f;
	float magnitude = sqrtf(fx * fx + fy * fy);

	if (magnitude < DEAD_ZONE) {
		nx = ny = 0.0f;
	} else {
		// 重映射有效区域到 [0, 1]
		float scale = (magnitude - DEAD_ZONE) / (1.0f - DEAD_ZONE);
		// 保持方向，缩放幅度
		if (magnitude > 0) {
			nx = (fx / magnitude) * scale;
			ny = (fy / magnitude) * scale;
		} else {
			nx = ny = 0.0f;
		}
	}
#else
	// --- 轴向死区 ---
	nx = normalize_axis(raw_x, DEAD_ZONE);
	ny = normalize_axis(raw_y, DEAD_ZONE);
#endif

	// 应用灵敏度（注意：Y 轴反向，因为屏幕坐标向下为正）
	joymouseX += nx * SENSITIVITY;
	joymouseY += ny * SENSITIVITY;  // 减号：上推摇杆 → 鼠标向上（Y 减小）

	// === 边界检测：限制在窗口内 ===
	if (joymouseX < 0) joymouseX = 0;
	if (joymouseY < 0) joymouseY = 0;
	if (joymouseX >= source_width-8) joymouseX = source_width-8;
	if (joymouseY >= source_height-8) joymouseY = source_height-8;
}

void updateMouse(int direct)
{
	switch(direct)
	{
		case SDLK_UP:
			if(joymouseY<6)
			{
				joymouseY=0;
			}
			else
			{
				joymouseY-=6;
			}
			break;
		case SDLK_DOWN:
			if(joymouseY+6>=source_height-11)
			{
				joymouseY=source_height-11;
			}
			else
			{
				joymouseY+=6;
			}
			break;
		case SDLK_LEFT:
			if(joymouseX<6)
			{
				joymouseX=0;
			}
			else
			{
				joymouseX-=6;
			}
			break;
		case SDLK_RIGHT:
			if(joymouseX+6>=source_width-8)
			{
				joymouseX=source_width-8;
			}
			else
			{
				joymouseX+=6;
			}
			break;
		
	}
}


int key2key(int k)
{
	if(switch_ok || use_numpad)
	{
		switch (k)
		{
			case SDLK_RETURN:
				return '5';
			default:
				break;
		}
	}


    switch (k)
    {
			case SDLK_o://右摇杆(R3)专用
				return 0xA7;
            case SDLK_r:
                return 0x7F;//#
            case SDLK_e:
                return '*';
            case SDLK_0:
                return '0';
            case SDLK_1:
                return '1';
            case SDLK_2:
                return '2';
            case SDLK_3:
                return '3';
            case SDLK_4:
                return '4';
            case SDLK_5:
                return '5';
            case SDLK_6:
                return '6';
            case SDLK_7:
                return '7';
            case SDLK_8:
                return '8';
            case SDLK_9:
                return '9';
            case SDLK_UP:
                return 0x10;
            case SDLK_DOWN:
                return 0x11;
            case SDLK_LEFT:
                return 0x0E;
            case SDLK_RIGHT:
                return 0x0F;
            case SDLK_RETURN:
                return 0xA7;
            case SDLK_q:
                return 0xA4;
            case SDLK_w:
                return 0xA5;
            case SDLK_m:
                return 0x01;
            case SDLK_n:
                return 10;
            // public static final int KEY_UP = 0x10;
            // public static final int KEY_DOWN = 0x11;
            // public static final int KEY_LEFT = 0x0E;
            // public static final int KEY_RIGHT = 0x0F;
            // public static final int KEY_FIRE = 0xA7;
            // public static final int KEY_SOFT_LEFT = 0xA4;
            // public static final int KEY_SOFT_RIGHT = 0xA5;
            // public static final int KEY_CLEAR = 0x01;
            // public static final int KEY_SEND = 10;
            default:
                return 0;
        
    }

    return 0;
    
}


void sendKey(int key, bool pressed)
{
	//wout("wlog.txt", "key:", key2key(key));
	emu_Emulator_pressKey(key2key(key), pressed ? 0 : 1);
}

void sendKey_Mouse(bool pressed)
{
	unsigned char bytes [5];
	bytes[0] = (char) (0x10 | pressed );

	bytes[1] = (char) (joymouseX >> 8 & 0xFF);
	bytes[2] = (char) (joymouseX & 0xFF);
	bytes[3] = (char) (joymouseY >> 8 & 0xFF);
	bytes[4] = (char) (joymouseY & 0xFF);
	
	//emu_Emulator_mouseKey
}

void sendKey_Numpad(int key , bool pressed)
{
	int newkey=0;
	switch (key)
	{
		case SDL_CONTROLLER_BUTTON_DPAD_UP:
			newkey = SDLK_2;
			break;
		case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
			newkey = SDLK_8;
			break;
		case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
			newkey = SDLK_4;
			break;
		case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
			newkey = SDLK_6;
			break;
		default:
			return;
	}

	sendKey(newkey, pressed);
}

int dpad2sdlk(int key)
{
	int newkey=0;
	switch (key)
	{
		case SDL_CONTROLLER_BUTTON_DPAD_UP:
		case SDLK_UP:
			newkey = SDLK_UP;
			break;
		case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
		case SDLK_DOWN:
			newkey = SDLK_DOWN;
			break;
		case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
		case SDLK_LEFT:
			newkey = SDLK_LEFT;
			break;
		case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
		case SDLK_RIGHT:
			newkey = SDLK_RIGHT;
			break;
		default:
			return 0;
	}
	return newkey;
}

void sendKey_Direct(int key , bool pressed)
{
	int newkey=dpad2sdlk(key);
	if(use_mouse)
	{
		updateMouse(newkey);
	}
	else
	{
		sendKey(newkey, pressed);
	}
}

void sendKey_Axis(int key , bool pressed)
{
	int newkey=dpad2sdlk(key);
	if(!use_mouse){
		sendKey(newkey, pressed);
	}
}

void update_direction_from_stick() {
    bool new_up    = (left_y < -STICK_DEAD_ZONE);
    bool new_down  = (left_y >  STICK_DEAD_ZONE);
    bool new_left  = (left_x < -STICK_DEAD_ZONE);
    bool new_right = (left_x >  STICK_DEAD_ZONE);

    // 检测“按下”（从 false → true）
    if (new_up && !up){
		printf("UP pressed\n");
		sendKey_Axis(SDLK_UP, true);
	}    
    if (new_down && !down){
		printf("DOWN pressed\n");
		sendKey_Axis(SDLK_DOWN, true);
	} 
    if (new_left && !left){
		printf("LEFT pressed\n");
		sendKey_Axis(SDLK_LEFT, true);
	} 
    if (new_right && !right)
	{
		printf("RIGHT pressed\n");
		sendKey_Axis(SDLK_RIGHT, true);
	} 

    // 检测“释放”（从 true → false）
    if (!new_up && up){
		printf("UP released\n");
		sendKey_Axis(SDLK_UP, false);
	}    
    if (!new_down && down){
		printf("DOWN released\n");
		sendKey_Axis(SDLK_DOWN, false);
	} 
    if (!new_left && left){
		printf("LEFT released\n");
		sendKey_Axis(SDLK_LEFT, false);
	} 
    if (!new_right && right){
		printf("RIGHT released\n");
		sendKey_Axis(SDLK_RIGHT, false);
	} 

    // 更新状态
    up    = new_up;
    down  = new_down;
    left  = new_left;
    right = new_right;
}

void AddController(int joystick_index) {
    if (SDL_IsGameController(joystick_index)) {
        SDL_GameController* gc = SDL_GameControllerOpen(joystick_index);
        if (gc) {
            SDL_JoystickID instance_id = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(gc));
            g_gameControllers[instance_id] = gc;
            std::cout << "手柄已连接: " << SDL_GameControllerName(gc) << " (ID: " << instance_id << ")\n";
        } else {
            std::cerr << "无法打开手柄: " << SDL_GetError() << "\n";
        }
    } else {
        std::cout << "设备 " << joystick_index << " 不是有效的 Game Controller\n";
    }
}

void RemoveController(SDL_JoystickID instance_id) {
    auto it = g_gameControllers.find(instance_id);
    if (it != g_gameControllers.end()) {
        SDL_GameControllerClose(it->second);
        g_gameControllers.erase(it);
        std::cout << "手柄已断开 (ID: " << instance_id << ")\n";
    }
}

uint32_t getUid(const std::string &path)
{
	std::ifstream file(path);  
    if (!file.is_open()) {  
        std::cerr << "Error opening file" << std::endl;  
        return 1;  
    }  
  
    std::string line;
	uint32_t uid;
	try {
		std::getline(file, line);
		//std::cout<<"line: "<<line<<std::endl;
		uid = atoi(line.c_str());
	} catch (const std::exception& e) {  
		std::cerr << "Error converting line to integer: " << e.what() << std::endl;  
	}

	return uid;
}

void cleanup()
{
	if(g_joystick)
	{
		SDL_JoystickClose(g_joystick);
	}

	// 清理所有手柄
    for (auto& pair : g_gameControllers) {
        SDL_GameControllerClose(pair.second);
    }
    g_gameControllers.clear();

	SDL_Quit();
}

//app: 7Days, uid: 0xE534CAFF
int main(int argc, char *argv[]){

	if(argc<2)
	{
		printf("用法: ./natvie type arg1 arg2...\n");
		return 0;
	}

	readResolutionAdvanced("size.cfg", display_width, display_height);

	int type=atoi(argv[1]);
	emu_Emulator_setDirectory(".");
	
	uint32_t uid=0xE534CAFF;//默认是7夜
	switch(type)
	{
		case 0://安装固件：rom地址,pkg地址
			emu_Emulator_startNative_install(argv[2],argv[3]);
			return 0;
			
		case 1://安装app：app地址
			emu_Emulator_startNative();
			emu_Emulator_installApp(argv[2]);
			
			return 0;
		case 2://启动app：app uid
			uid=atoi(argv[2]);
			emu_Emulator_startNative();

			emu_Emulator_setScreenParams(0xFF4F4F4F, 100.0f, 1, 2);
			emu_Emulator_surfaceChanged((void *)0x1, display_width, display_height);
			emu_Emulator_launchApp(uid);
			break;
		case 3://只是查看信息，更新.st启动文件，sis游戏与ngage1游戏需要分别更新
			emu_Emulator_startNative();
			emu_Emulator_getDeviceFirmwareCodes();
			emu_Emulator_getDevices();
			emu_Emulator_setCurrentDevice(atoi(argv[2]),true);
			getApps(argv[3]);
			return 0;
		case 4://通过.st文件启动app，并且带旋转，带设备id
			{
				std::string path = argv[2];
				uid=getUid(path);
				if(argc==3)
				{
					emu_Emulator_startNative();
				}
				else if(argc==4)
				{
					emu_Emulator_startNative(atoi(argv[3]));
				}
				else if(argc==5)
				{
					emu_Emulator_startNative(atoi(argv[3]));
					emu_Emulator_setCurrentDevice(atoi(argv[4]),true);
				}

				
				
				emu_Emulator_setScreenParams(0xFF4F4F4F, 100.0f, 1, 2);
				emu_Emulator_surfaceChanged((void *)0x1, display_width, display_height);
				emu_Emulator_launchApp(uid);
			}
			break;
		case 5://扫描固件
			emu_Emulator_rescanDevices();
			return 0;
	}


	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0) 
	{
		std::cout<<"SDL无法初始化! SDL_Error: "<<SDL_GetError()<<std::endl;
		exit(0);
	}

	SDL_GameControllerEventState(SDL_ENABLE);
    // 检测启动时已连接的手柄
    int num_joysticks = SDL_NumJoysticks();
    for (int i = 0; i < num_joysticks; ++i) {
		// 获取 GUID
        SDL_JoystickGUID guid = SDL_JoystickGetDeviceGUID(i);
        
        // 转为字符串（格式：00000000000000000000000000000000）
        char guid_str[33];
        SDL_JoystickGetGUIDString(guid, guid_str, sizeof(guid_str));

        const char* name = SDL_JoystickNameForIndex(i);
        printf("Joystick %d: %s\n", i, name);
        printf("  GUID: %s\n", guid_str);
		
        AddController(i);
    }
	
	loadConfig();
    int mod=0;
	SDL_Event e;
	//SDL_EnableKeyRepeat(200, 20);
	while (capturing)
	{
		if (SDL_WaitEvent(&e))
		// while(SDL_PollEvent(&event)!= 0)
		{
			switch (e.type)
			{
				case SDL_QUIT:
					capturing = false;
					sendKey(-1, true);
					//continue;
					break;
				case SDL_CONTROLLERDEVICEADDED:
					//e.cdevice.which 在 CONTROLLERDEVICEADDED/REMOVED 事件中 就是 Instance ID（不是 device index)
					//实际上：SDL 允许将 instance ID 作为 "virtual index" 传入 SDL_GameControllerOpen
					//这是 SDL2 的特殊行为：支持用 instance ID 打开
                    std::cout << "检测到新手柄插入\n";
					//先判断是否已经打开了
					if (g_gameControllers.find(e.cdevice.which) == g_gameControllers.end()) {
						AddController(e.cdevice.which);
					}
                    
                    break;

                case SDL_CONTROLLERDEVICEREMOVED:
                    std::cout << "手柄被拔出\n";
                    RemoveController(e.cdevice.which);
                    break;
				
				case SDL_KEYDOWN:
				case SDL_KEYUP:
				{
					// printf("keycode: 0x%x name: %s state: %d\n",event.key.keysym.sym,SDL_GetKeyName(event.key.keysym.sym),event.key.state);
					// fflush(stdout);
					
					// int key = e.key.keysym.sym;
					// if(key==SDLK_UP)//上
					// {
					// 	if(rotate==1)
					// 	{
					// 		key=SDLK_RIGHT;
					// 	}
					// 	else if(rotate==2)
					// 	{
					// 		key=SDLK_LEFT;
					// 	}
						
					// 	if(use_mouse && e.key.state == SDL_PRESSED)
					// 	{
					// 		updateMouse(key);
					// 	}
						
					// }
					// else if(key==SDLK_DOWN)//下
					// {
					// 	if(rotate==1)
					// 	{
					// 		key=SDLK_LEFT;
					// 	}
					// 	else if(rotate==2)
					// 	{
					// 		key=SDLK_RIGHT;
					// 	}
					// 	if(use_mouse && e.key.state == SDL_PRESSED)
					// 	{
					// 		updateMouse(key);
					// 	}
					// }
					// else if(key==SDLK_LEFT) //左
					// {
					// 	if(rotate==1)
					// 	{
					// 		key=SDLK_UP;
					// 	}
					// 	else if(rotate==2)
					// 	{
					// 		key=SDLK_DOWN;
					// 	}
					// 	if(use_mouse && e.key.state == SDL_PRESSED)
					// 	{
					// 		updateMouse(key);
					// 	}
					// }
					// else if(key==SDLK_RIGHT) //右
					// {
					// 	if(rotate==1)
					// 	{
					// 		key=SDLK_DOWN;
					// 	}
					// 	else if(rotate==2)
					// 	{
					// 		key=SDLK_UP;
					// 	}
					// 	if(use_mouse && e.key.state == SDL_PRESSED)
					// 	{
					// 		updateMouse(key);
					// 	}
					// }
					
					// if(!use_mouse)
					// {
					// 	sendKey(key, e.key.state == SDL_PRESSED);
					// }
				}
				break;

				case SDL_CONTROLLERAXISMOTION: {
                    Sint16 value = e.caxis.value;
					const char* axis_name = "";
					
					switch (e.caxis.axis) {
						case SDL_CONTROLLER_AXIS_LEFTX://左摇杆用于控制方向
							left_x = value;
							update_direction_from_stick();
							axis_name = "Left Stick X";
							break;
						case SDL_CONTROLLER_AXIS_LEFTY:
							left_y = value;
							update_direction_from_stick();
							axis_name = "Left Stick Y";
							break;
						case SDL_CONTROLLER_AXIS_RIGHTX:
							axis_name = "Right Stick X";
							break;
						case SDL_CONTROLLER_AXIS_RIGHTY:
							axis_name = "Right Stick Y";
							break;
						case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
						{
							if (!left_trigger_active && value > TRIGGER_PRESS_THRESHOLD) {
								left_trigger_active = true;
								printf("Left Trigger PRESSED (value=%d)\n", value);
								// 在这里执行“按下”逻辑
								sendKey(joy2emu(SDL_AXIS_TRIGGERLEFT),true);
							}
							else if (left_trigger_active && value < TRIGGER_RELEASE_THRESHOLD) {
								left_trigger_active = false;
								printf("Left Trigger RELEASED (value=%d)\n", value);
								// 在这里执行“释放”逻辑
								sendKey(joy2emu(SDL_AXIS_TRIGGERLEFT),false);
							}
						
							axis_name = "Left Trigger";
							break;
						}
						case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
						{
							if (!right_trigger_active && value > TRIGGER_PRESS_THRESHOLD) {
								right_trigger_active = true;
								printf("Right Trigger PRESSED (value=%d)\n", value);
								sendKey(joy2emu(SDL_AXIS_TRIGGERRIGHT),true);
							}
							else if (right_trigger_active && value < TRIGGER_RELEASE_THRESHOLD) {
								right_trigger_active = false;
								printf("Right Trigger RELEASED (value=%d)\n", value);
								sendKey(joy2emu(SDL_AXIS_TRIGGERRIGHT),false);
							}
						
							axis_name = "Right Trigger";
							break;
						}
						default:
							axis_name = "Unknown Axis";
					}
					
					//printf("axis_name: %s value:%d\n", axis_name, e.caxis.value);
					
					break;
                }
				

                case SDL_CONTROLLERBUTTONDOWN:
                case SDL_CONTROLLERBUTTONUP: {
					const char* button_name = "";
                    SDL_GameControllerButton button = static_cast<SDL_GameControllerButton>(e.cbutton.button);
                    const char* state = (e.cbutton.state == SDL_PRESSED) ? " 按下 " : " 释放 ";
                    
					int key=0;
					switch (e.cbutton.button) {
						case SDL_CONTROLLER_BUTTON_A:
							button_name="SDL_CONTROLLER_BUTTON_A";
							break;
						case SDL_CONTROLLER_BUTTON_B:
							button_name="SDL_CONTROLLER_BUTTON_B";
							break;
						case SDL_CONTROLLER_BUTTON_X:
							button_name="SDL_CONTROLLER_BUTTON_X";
							break;
						case SDL_CONTROLLER_BUTTON_Y://在鼠标模式下模拟鼠标点击
							button_name="SDL_CONTROLLER_BUTTON_Y";
							if(use_mouse){
								sendKey_Mouse(e.cbutton.state == SDL_PRESSED);
								e.cbutton.button=0;
							}
							break;
						case SDL_CONTROLLER_BUTTON_BACK://select
							button_name="SDL_CONTROLLER_BUTTON_BACK";
							break;
						case SDL_CONTROLLER_BUTTON_START://start
							button_name="SDL_CONTROLLER_BUTTON_START";
							break;
						case SDL_CONTROLLER_BUTTON_LEFTSHOULDER://L1
							button_name="SDL_CONTROLLER_BUTTON_LEFTSHOULDER";
							break;
						case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER://R1
							button_name="SDL_CONTROLLER_BUTTON_RIGHTSHOULDER";
							break;
						case SDL_CONTROLLER_BUTTON_LEFTSTICK://L3
							button_name="SDL_CONTROLLER_BUTTON_LEFTSTICK";
							//use_numpad=((e.cbutton.state == SDL_PRESSED) ? (1-use_numpad) : use_numpad) ;
							break;
						case SDL_CONTROLLER_BUTTON_RIGHTSTICK://R3
							button_name="SDL_CONTROLLER_BUTTON_RIGHTSTICK";
							//sendKey(SDLK_o, e.cbutton.state == SDL_PRESSED);
							break;
						case SDL_CONTROLLER_BUTTON_DPAD_UP:
						case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
						case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
						case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
							if(use_numpad)
							{
								sendKey_Numpad(e.cbutton.button, e.cbutton.state == SDL_PRESSED);
							}
							else{
								sendKey_Direct(e.cbutton.button, e.cbutton.state == SDL_PRESSED);
							}
							break;
						case SDL_CONTROLLER_BUTTON_GUIDE: //menu键退出
							button_name="SDL_CONTROLLER_BUTTON_GUIDE";
							capturing = false;
							sendKey(-1, e.cbutton.state == SDL_PRESSED);
							break;
						default://未识别的键也退出
							button_name = "Unknown button_name";
							capturing = false;
							sendKey(-1, e.cbutton.state == SDL_PRESSED);
							break;	
						
					}
					//std::cout << "按钮 [" << static_cast<int>(button) << "] " << button_name << state << "\n";
					//按住select键
					if(e.cbutton.button == SDL_CONTROLLER_BUTTON_BACK && e.cbutton.state == SDL_PRESSED){mod=1;}
					else if(e.cbutton.button == SDL_CONTROLLER_BUTTON_BACK && e.cbutton.state == SDL_RELEASED){mod=0;}

					key = joy2emu(e.cbutton.button);

					if(mod)
					{
						switch (e.cbutton.button) {
							case SDL_CONTROLLER_BUTTON_X://切换鼠标模式
								mod=0;
								//use_mouse=1-use_mouse;//切换鼠标
								key=0;
								break;
							case SDL_CONTROLLER_BUTTON_A://旋转
								mod=0;
								rotate=(1+rotate)%4;//连续旋转
								emu_Emulator_rotate(90*rotate);
								key=0;
								break;
							case SDL_CONTROLLER_BUTTON_START://把ok键变成5
								mod=0;
								switch_ok=((e.cbutton.state == SDL_PRESSED) ? (1-switch_ok) : switch_ok) ;
								key=0;
								break;
						}
					}

					if(key && !use_mouse)
					{
						sendKey(key, e.cbutton.state == SDL_PRESSED);
					}

					break;
                }
				
				case SDL_JOYAXISMOTION:
				case SDL_JOYHATMOTION:
				case SDL_JOYBUTTONDOWN:
    		    case SDL_JOYBUTTONUP:
					break;
				
				default:
					//printf("未知事件:event type 0x%x \n",e.type);
					//fflush(stdout);
					break;
				
				
			}//switch
		}//if
	}//while

	cleanup();

    return 0;
}