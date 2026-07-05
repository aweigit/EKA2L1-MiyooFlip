# EKA2L1-MiyooFlip

[English](#english)

---

### 中文

本项目是 [EKA2L1](https://github.com/EKA2L1/EKA2L1) 在 Linux 平台下的移植版本，支持aarch64 开源掌机TrimuiBrick、MiyooFlip、x64 Ubuntu 。

### 支持平台

| 平台 | 架构 | 依赖库 |
|------|------|----------|
| Ubuntu 20+ | x86_64 | EGL/GLESv2 |
| Trimui Brick | aarch64 | EGL/GLESv2 |
| Miyoo Flip | aarch64 | Mali/DRM |

### 依赖

- CMake >= 3.8
- 支持 C++20 的GCC
- SDL2
- SDL2_ttf (menu 程序需要)
- EGL
- GLESv2

### 构建说明

#### 1. 预编译 FFmpeg

FFmpeg 必须在 CMake 构建前预编译：

**x64 Linux:**
```bash
cd src/external/ffmpeg
./linux_x64-build.sh
```

**aarch64 Linux:**
```bash
cd src/external/ffmpeg
./linux_aarch64-build.sh
```

#### 2. CMake 构建

##### 为 x64 Ubuntu 20 构建
```bash
mkdir build && cd build
cmake .. -DLINUX_SDL2=True \
         -DCUSTOM_LINK_PATH=/usr/lib/x86_64-linux-gnu \
         -DCI=OFF \
         -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_C_COMPILER=gcc \
         -DCMAKE_CXX_COMPILER=g++
make
```

##### 为 Trimui Brick 构建
```bash
mkdir build && cd build
cmake .. -DLINUX_SDL2=True \
         -DTRIMUI_BRICK=True \
         -DCUSTOM_LINK_PATH=/usr/lib/lib-trimui \
         -DCI=OFF \
         -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_C_COMPILER=aarch64-none-linux-gnu-gcc \
         -DCMAKE_CXX_COMPILER=aarch64-none-linux-gnu-g++
make
```

##### 为 Miyoo Flip 构建
```bash
mkdir build && cd build
cmake .. -DLINUX_SDL2=True \
         -DMIYOO_FLIP=True \
         -DCUSTOM_LINK_PATH=/usr/lib/lib-flip \
         -DCI=OFF \
         -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_C_COMPILER=aarch64-none-linux-gnu-gcc \
         -DCMAKE_CXX_COMPILER=aarch64-none-linux-gnu-g++
make
```

#### CMake 参数说明

| 参数 | 说明 |
|------|------|
| `LINUX_SDL2` | 启用 SDL2 前端 (必须为 True) |
| `CUSTOM_LINK_PATH` | 链接库搜索路径(必须指定) |
| `TRIMUI_BRICK` | 启用 Trimui Brick 平台支持 |
| `MIYOO_FLIP` | 启用 Miyoo Flip 平台支持 |

### 构建产物

构建完成后，在 `build/` 目录下生成以下可执行文件：

| 文件 | 说明 |
|------|------|
| `native` | 模拟器主程序 |
| `menu` | 塞班模拟器设置程序 |

### 使用方法（以Miyoo Flip为例）

#### 目录结构

模拟器数据目录结构如下：
```
Emu/NOKIA/
├── compat/
├── patch/
├── resources/
├── scripts/
├── data/           # 固件和系统数据
│   ├── devices.yml # 固件配置文件
│   ├── drives/     # 虚拟磁盘 (c:, d:, e:, z:)
│   └── roms/       # 固件 ROM 文件
├── config.yml      # 模拟器配置文件
├── app/            # 待安装的游戏文件 (SIS/SISX)
├── sis/            # 游戏启动文件 (.st，仅包含APP ID)
├── native          # 模拟器主程序
├── menu            # 模拟器设置程序
├── size.cfg        # 屏幕分辨率
└── keymap.cfg      # 按键映射配置
```

#### native 程序 (模拟器主程序)

**路径:** `Emu/NOKIA/native`

##### 安装固件
```bash
./native 0 <ROM文件> <RPKG文件>
# 示例：安装 E6 固件
./native 0 ./E6.ROM ./E6.RPKG
```

##### 安装游戏 (SIS/SISX)
```bash
./native 1 <游戏文件>
# 示例：安装七夜
./native 1 ./7days.sisx
```

##### 通过 APP ID 启动游戏
```bash
./native 2 <APP ID (十进制)>
# 示例：启动七夜 (APP ID: 3845442303)
./native 2 3845442303
```

##### 生成/更新 .st 文件
```bash
./native 3 [固件序号] [.st文件保存路径/]
# 示例：使用 Nokia 5320 固件 (序号1)
./native 3 1 ./sis/
```

##### 通过 .st 文件启动游戏
```bash
./native 4 [.st文件路径] [画面旋转角度: 0/90/180/270] [固件序号]
# 示例：启动七夜，旋转90度，使用固件序号2
./native 4 ./7Days.st 90 2
```

##### 刷新固件
```bash
./native 5
```

#### menu 程序 (塞班模拟器设置)

**路径:** `Emu/NOKIA/menu`

```bash
./menu [S60v3固件序号] [N-Gage1固件序号] [S60v3游戏.st路径/] [N-Gage1/S60v1/v2游戏.st路径/]
# 示例
./menu 1 0 ./sis/ ./sis/ng/
```

##### 功能菜单
- **安装游戏** - 从 `Emu/NOKIA/app` 安装 SIS/SISX 游戏
- **扫描游戏** - 扫描并生成 S60v3 游戏的 .st 文件
- **刷新固件** - 重新生成 `devices.yml` 配置
- **扫描 N-Gage1 游戏** - 扫描 N-Gage1/S60v1/v2 游戏

### 游戏安装

#### N-Gage 2 游戏
将游戏文件放入 `Emu/NOKIA/data/drives/c/data/n-gage` 目录，文件名不能包含中文。在掌机上打开 Games (使用 Nokia 5320 固件) 自动安装。

#### N-Gage 1 游戏
将游戏文件夹拷贝到 `Emu/NOKIA/data/drives/e/system/apps` 目录。**注意:** `drives/e/` 下所有文件名必须为小写。

#### SIS/SISX 游戏
1. 将游戏文件放入 `Emu/NOKIA/app` 目录 (文件名不能含空格)
2. 运行 `./menu` 选择"安装游戏"安装

### 渲染纹理降采样参数

```bash
#config.yml文件(0.5-1.0，越大越清晰，但是内存占用更高)
render-scale: 0.5
```

### 按键映射

#### 默认映射

| 掌机按键 | 映射功能 |
|---------|----------|
| Y | 左键 |
| A | 右键 |
| X | OK 键 (确认) |
| B | 数字 0 |
| Select | * 键 |
| Start | # 键 |
| L1/R1 | 数字 1/3 |
| L2/R2 | 数字 7/9 |
| Menu | 强制退出游戏 |

#### 组合键

| 组合 | 功能 |
|------|------|
| Select + Start | 切换 OK 键为数字 5 |
| Select + B | 旋转画面 |

#### 平台特殊映射

**Miyoo Flip:**
- 左摇杆可作为方向键 (映射为 2/4/6/8)
- 左摇杆点击把方向键切换为2,4,6,8
- 右摇杆点击作为 OK 键

**Trimui Brick:**
- L3、R3默认映射为 4/6 (因无摇杆)

按键映射可通过 `keymap.cfg` 文件自定义。

### 添加新固件

1. 在电脑版 EKA2L1 安装新固件
2. 将对应文件夹拷贝到掌机内存卡
3. 运行 `./native 5` 或通过 menu 程序"刷新固件"
4. **注意:** 刷新固件后，固件序号可能变化，需相应修改启动脚本参数

### 注意事项

1. **文件名必须小写:** `data/drives/` 目录下所有文件及子文件夹名称必须为小写
2. **固件序号:** `devices.yml` 中固件顺序决定序号 (从0开始)，添加新固件后序号可能变化

### 许可证

本项目基于 GNU General Public License v3 (GPLv3) 许可证开源。详见 [LICENSE](LICENSE) 文件。

### 致谢

感谢 EKA2L1 原项目团队的杰出工作。

---

[中文](#中文)

## English

This is a port of [EKA2L1](https://github.com/EKA2L1/EKA2L1) for the Linux platform, supporting aarch64 handhelds (Trimui Brick, Miyoo Flip) and x64 Ubuntu.

### Supported Platforms

| Platform | Architecture | Dependencies |
|----------|-------------|--------------|
| Ubuntu 20+ | x86_64 | EGL/GLESv2 |
| Trimui Brick | aarch64 | EGL/GLESv2 |
| Miyoo Flip | aarch64 | Mali/DRM |

### Dependencies

- CMake >= 3.8
- GCC with C++20 support
- SDL2
- SDL2_ttf (required for menu program)
- EGL
- GLESv2

### Build Instructions

#### 1. Pre-compile FFmpeg

FFmpeg must be pre-compiled before the CMake build:

**x64 Linux:**
```bash
cd src/external/ffmpeg
./linux_x64-build.sh
```

**aarch64 Linux:**
```bash
cd src/external/ffmpeg
./linux_aarch64-build.sh
```

#### 2. CMake Build

##### Build for x64 Ubuntu 20
```bash
mkdir build && cd build
cmake .. -DLINUX_SDL2=True \
         -DCUSTOM_LINK_PATH=/usr/lib/x86_64-linux-gnu \
         -DCI=OFF \
         -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_C_COMPILER=gcc \
         -DCMAKE_CXX_COMPILER=g++
make
```

##### Build for Trimui Brick
```bash
mkdir build && cd build
cmake .. -DLINUX_SDL2=True \
         -DTRIMUI_BRICK=True \
         -DCUSTOM_LINK_PATH=/usr/lib/lib-trimui \
         -DCI=OFF \
         -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_C_COMPILER=aarch64-none-linux-gnu-gcc \
         -DCMAKE_CXX_COMPILER=aarch64-none-linux-gnu-g++
make
```

##### Build for Miyoo Flip
```bash
mkdir build && cd build
cmake .. -DLINUX_SDL2=True \
         -DMIYOO_FLIP=True \
         -DCUSTOM_LINK_PATH=/usr/lib/lib-flip \
         -DCI=OFF \
         -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_C_COMPILER=aarch64-none-linux-gnu-gcc \
         -DCMAKE_CXX_COMPILER=aarch64-none-linux-gnu-g++
make
```

#### CMake Options

| Option | Description |
|--------|-------------|
| `LINUX_SDL2` | Enable SDL2 frontend (must be True) |
| `CUSTOM_LINK_PATH` | Library search path |
| `TRIMUI_BRICK` | Enable Trimui Brick platform support |
| `MIYOO_FLIP` | Enable Miyoo Flip platform support |

### Build Artifacts

After building, the following executables are generated in the `build/` directory:

| File | Description |
|------|-------------|
| `native` | Emulator main program |
| `menu` | Symbian emulator settings program |

### Usage (Miyoo Flip Example)

#### Directory Structure

The emulator data directory structure is as follows:
```
Emu/NOKIA/
├── compat/
├── patch/
├── resources/
├── scripts/
├── data/           # Firmware and system data
│   ├── devices.yml # Firmware config
│   ├── drives/     # Virtual disks (c:, d:, e:, z:)
│   └── roms/       # Firmware ROM files
├── config.yml      # Emulator config
├── app/            # Games to install (SIS/SISX)
├── sis/            # Game launch files (.st, APP ID only)
├── native          # Emulator main program
├── menu            # Emulator settings program
├── size.cfg        # Screen resolution
└── keymap.cfg      # Key mapping config
```

#### native Program (Emulator Main)

**Path:** `Emu/NOKIA/native`

##### Install Firmware
```bash
./native 0 <ROM file> <RPKG file>
# Example: Install E6 firmware
./native 0 ./E6.ROM ./E6.RPKG
```

##### Install Game (SIS/SISX)
```bash
./native 1 <game file>
# Example: Install 7Days
./native 1 ./7days.sisx
```

##### Launch Game by APP ID
```bash
./native 2 <APP ID (decimal)>
# Example: Launch 7Days (APP ID: 3845442303)
./native 2 3845442303
```

##### Generate/Update .st Files
```bash
./native 3 [firmware index] [.st save path/]
# Example: Use Nokia 5320 firmware (index 1)
./native 3 1 ./sis/
```

##### Launch Game via .st File
```bash
./native 4 [.st path] [rotation: 0/90/180/270] [firmware index]
# Example: Launch 7Days, rotate 90°, firmware index 2
./native 4 ./7Days.st 90 2
```

##### Refresh Firmware
```bash
./native 5
```

#### menu Program (Symbian Emulator Settings)

**Path:** `Emu/NOKIA/menu`

```bash
./menu [S60v3 index] [N-Gage1 index] [S60v3 .st path/] [N-Gage1/S60v1/v2 .st path/]
# Example
./menu 1 0 ./sis/ ./sis/ng/
```

##### Menu Features
- **Install Game** - Install SIS/SISX games from `Emu/NOKIA/app`
- **Scan Games** - Scan and generate .st files for S60v3 games
- **Refresh Firmware** - Regenerate `devices.yml` config
- **Scan N-Gage1 Games** - Scan N-Gage1/S60v1/v2 games

### Game Installation

#### N-Gage 2 Games
Place game files in `Emu/NOKIA/data/drives/c/data/n-gage`. Filenames must not contain Chinese characters. Open Games on the handheld (using Nokia 5320 firmware) to auto-install.

#### N-Gage 1 Games
Copy game folders to `Emu/NOKIA/data/drives/e/system/apps`. **Note:** All filenames under `drives/e/` must be lowercase.

#### SIS/SISX Games
1. Place game files in `Emu/NOKIA/app` (filenames must not contain spaces)
2. Run `./menu` and select "Install Game" to install

### Render Texture Downsampling‌

```bash
#config.yml(0.5-1.0)
render-scale: 0.5
```

### Key Mapping

#### Default Mapping

| Handheld Button | Mapped Function |
|----------------|-----------------|
| Y | Left Softkey |
| A | Right Softkey |
| X | OK Key (Confirm) |
| B | Number 0 |
| Select | * Key |
| Start | # Key |
| L1/R1 | Number 1/3 |
| L2/R2 | Number 7/9 |
| Menu | Force Exit Game |

#### Combo Keys

| Combo | Function |
|-------|----------|
| Select + Start | Toggle OK key to number 5 |
| Select + B | Rotate Screen |

#### Platform-Specific Mapping

**Miyoo Flip:**
- Left joystick works as D-pad (mapped to 2/4/6/8)
- Left joystick press toggles D-pad to 2,4,6,8
- Right joystick press acts as OK key

**Trimui Brick:**
- L3, R3 default mapped to 4/6 (no joystick available)

Key mappings can be customized via the `keymap.cfg` file.

### Adding New Firmware

1. Install new firmware in the desktop version of EKA2L1
2. Copy the corresponding folder to the handheld's memory card
3. Run `./native 5` or use the menu program "Refresh Firmware"
4. **Note:** After refreshing firmware, the firmware index may change; update launch script parameters accordingly

### Notes

1. **Filenames must be lowercase:** All files and subfolders under `data/drives/` must be lowercase
2. **Firmware index:** The order in `devices.yml` determines the index (starting from 0); indices may change after adding firmware

### License

This project is licensed under the GNU General Public License v3 (GPLv3). See the [LICENSE](LICENSE) file for details.

### Acknowledgments

Thanks to the original EKA2L1 project team for their excellent work.
