# 1.程序架构设计

## 1.1 RetroArch 架构

[RetroArch](https://www.retroarch.com/) 是一个多平台模拟器框架，它的设计理念非常值得参考：

|             模块             |                   职责                    |
| :--------------------------: | :---------------------------------------: |
|   **RetroArch (Frontend)**   | 负责 UI、音视频、输入、状态保存、配置管理 |
|   **Libretro Core (Core)**   | 仅负责游戏机逻辑模拟，如 NES、SNES、GB 等 |
| **Libretro API (Interface)** |  定义标准接口，使核心可以被任何前端加载   |

在本系列教程中，整个模拟器的结构将会采取RetroArch的思路来进行设计。

 分为三个主要部分：

```
前端（Frontend）
└── 调用核心，负责音视频输出、输入事件、UI交互

核心（Core）
└── 模拟 CPU、PPU、APU、内存、总线等 NES 硬件逻辑

通用接口（Bridge）
└── 定义前端与核心之间的通信协议（帧缓冲、音频缓冲、输入状态）

```

这种设计类似于 **RetroArch + Libretro** 的思想：

- **前端**：负责平台相关的部分（窗口系统、键盘手柄、音频播放等）
- **核心**：负责平台无关的部分（NES 硬件模拟逻辑）
- **接口**：两者之间使用固定的数据结构通信，从而保证可移植性

## 1.2 核心模块划分

|               模块                |     功能     |               说明               |
| :-------------------------------: | :----------: | :------------------------------: |
|        **CPU (MOS 6502)**         |  指令集实现  |         负责执行程序代码         |
| **PPU (Picture Processing Unit)** |   图像生成   |    控制图层、调色板、精灵绘制    |
|  **APU (Audio Processing Unit)**  |   声音合成   |   生成方波、三角波、噪声等波形   |
|        **Cartridge (ROM)**        | 存储游戏数据 |  解析 iNES 文件头，加载 PRG/CHR  |
|              **Bus**              | 连接各个模块 |   模拟 NES 地址总线与内存映射    |
|            **Mapper**             | 扩展存储控制 | 处理不同游戏的地址映射带切换逻辑 |
|         **Emulator Core**         |   整体协调   | 负责时序同步、帧执行、状态保存等 |

## 1.3 前端模块划分

|    模块    |             功能              |
| :--------: | :---------------------------: |
| **Video**  | 将 PPU 输出的帧缓冲显示到窗口 |
| **Audio**  |     播放 APU 输出的音频流     |
| **Input**  |       采集手柄/键盘输入       |
| **Window** |    驱动核心执行并渲染画面     |

第三方库：

- 使用**PortAudio** 播放音频
- 使用 **GLFW** 显示窗口画面
- 使用**OpenGL**进行画面渲染

# 2.开发环境搭建

本系列教程开发环境的组合为：

> **CMake + MinGW-w64 + VSCode**

这套环境兼顾了 **跨平台性、现代性、轻量级与实用性**，是目前实现模拟器类项目的理想方案。

## 2.1 CMake

[CMake](https://cmake.org/) 是一个跨平台的构建系统配置工具，它并不直接编译代码，而是生成适合目标平台的构建脚本（如 Ninja、Makefile、Visual Studio 工程等）。

一个NES 模拟器包含多个模块（CPU / PPU / APU / 前端），还涉及到一些第三方库。使用CMake 能非常方便构建整个项目。

|        特性         |                             说明                             |
| :-----------------: | :----------------------------------------------------------: |
|     **跨平台**      |   一份 `CMakeLists.txt` 可在 Windows、Linux、macOS 上通用    |
|   **模块化管理**    | 支持 `add_subdirectory()` 管理多个子模块（如 `nes_core` / `frontend`） |
|   **可移植构建**    | 支持多种编译器（GCC / Clang / MSVC）和构建系统（Make / Ninja） |
| **与 IDE 深度集成** |           VSCode、CLion、Visual Studio 都原生支持            |
|  **自动依赖检测**   |        可轻松查找并链接外部库，如 SDL2、PortAudio 等         |

## 2.2 MinGW

[MinGW-w64](https://www.mingw-w64.org/) 是 GCC 编译器在 Windows 平台的移植版，它提供：

- GNU 工具链（gcc、g++、ld、make、gdb）
- 兼容 Win32 API
- 可生成高性能、轻量级的原生 Windows 可执行文件

对于 NES 模拟器这种“跨平台项目 + 频繁调试型代码”，MinGW-w64 的优势在于：

- 快速迭代、编译速度快；
- 二进制文件启动迅速，录制演示方便；
- 未来移植到 Linux/macOS 几乎零改动。

|            特性             |                             说明                             |
| :-------------------------: | :----------------------------------------------------------: |
|     **原生可执行文件**      | 编译结果无需依赖额外运行时（不像 MSVC 那样需安装 redistributable） |
| **与 Linux 一致的编译体验** |           同样的 CMake 配置文件可直接移植到 Linux            |
|     **支持 POSIX 标准**     |              可使用 pthread、stdint.h 等标准库               |
|  **配合 Ninja 编译速度快**  |                    支持并行构建和增量编译                    |

## 2.3 VSCode

[Visual Studio Code](https://code.visualstudio.com/) 是一个轻量级、模块化的编辑器，通过插件可以变身为功能完整的 C++ IDE。

它支持智能提示、调试、代码格式化、构建任务、终端集成等特性，非常适合做跨平台项目开发。

|          特性          |                           说明                            |
| :--------------------: | :-------------------------------------------------------: |
|    **插件生态丰富**    |  CMake Tools、C/C++、Clang-Format、LLDB/GDB、GitLens 等   |
|   **跨平台一致体验**   |           Windows、macOS、Linux 均可无差别使用            |
|      **快速启动**      |             远比 Visual Studio、CLion 更轻量              |
| **内置终端与任务系统** |      可以直接运行 CMake/Ninja 命令，无需额外切换环境      |
|   **优秀的调试支持**   | 通过 CodeLLDB 或 C++ Debugger 可直接断点调试 NES 核心代码 |

# 3.框架搭建

## 3.1 日志模块

在任何一个软件系统中，日志都是必不可少的。它帮助我们更直观的理解程序的运行途径，有时候比直接调试程序能更快的发现程序BUG。本系列将实现一个轻量级的日志器，在实现的过程中顺带讲解一些基础的c++知识。

主要组成部分：

| 模块                                    | 作用                                           |
| --------------------------------------- | ---------------------------------------------- |
| `Timestamp`                             | 负责生成时间戳字符串                           |
| `Logger`                                | 日志系统主体，支持不同级别的日志               |
| `Logger::LogEntry`                      | 记录源代码上下文（文件、行号、函数名）         |
| 一组宏定义（`LOG_INFO`, `LOG_WARN`, …） | 简化调用，自动捕获上下文信息                   |
| `logger_ctx` 命名空间                   | 存放线程 ID、进程 ID、日志文件句柄等上下文数据 |

### 实现重点

### (1) 可变参数模板与格式化输出

```c++
template<typename... Args>
std::string formatString(const char* fmt, Args&&... args)
```

- 使用 **模板参数包** (`Args...`) 支持任意数量参数；
- 内部调用 `std::snprintf` 实现类似 `printf` 的格式化；
- 对于长度不确定的字符串，使用了两种缓冲策略：
  - 优先使用 `thread_local char buf[1024]`
  - 超长时再动态分配 `std::vector<char>`

👉 **重点理解：**

- 可变参数模板的展开与转发（`std::forward`）
- 为什么要用 `thread_local`：避免多线程共享缓冲区带来的竞争
- `std::snprintf(nullptr, 0, fmt, args...)` 用于预估字符串长度的技巧

###  (2) 日志级别控制

```c++
enum {
    LOG_LEVEL_TRACE,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_FATAL,
};
```

- 日志分级控制输出粒度，方便在开发、调试、发布阶段切换。

- 内部判断：

  ```c++
  if (LOG_LEVEL_TRACE >= _level)
      log(LOG_LEVEL_TRACE, ...);
  ```

- `_level` 控制最低输出级别。
   例如：`_level = LOG_LEVEL_WARN` 时，只会输出 WARN / ERROR / FATAL。

### (3) 宏与上下文捕获

```c++
#define LOG_WARN(...) \
    __logger.warn(Logger::LogEntry{__LINE__, __FILE__, __func__}, __VA_ARGS__)
```

- 通过预定义宏 `__LINE__`, `__FILE__`, `__func__` 捕获调用点信息；
- 将这些信息打包为 `LogEntry` 结构；
- 通过 `__logger`（全局单例）直接输出日志。

👉 **重点理解：**

- 为什么使用宏？ → 自动补充上下文，避免手动传参；
- 宏参数转发 `__VA_ARGS__`；
- 最简单的单例（`extern Logger __logger`）。

### (4) 多线程安全与锁

```c++
std::mutex _mutex;
std::lock_guard<std::mutex> lock(_mutex);
```

- 每次写日志时加锁，防止多线程同时输出产生交叉。
- 使用 RAII (`std::lock_guard`) 自动上锁与释放。

👉 **重点理解：**

- 为什么日志系统必须是线程安全的；
- RAII 思想如何保证异常安全；
- 可扩展方向：异步写日志、环形缓冲。

### (5) 线程上下文信息

`logger_ctx` 命名空间保存了：

```c++
thread_local int tid;
thread_local std::string tid_string;
thread_local std::string log_ctx_string;
```

- 使用 `thread_local` 为每个线程保存独立的日志上下文。
- 日志中输出 `[pid:tid]` 格式，方便区分线程来源。

👉 **重点理解：**

- `thread_local` 的作用与生命周期；
- 线程 ID 获取：`std::this_thread::get_id()`；
- 日志上下文字符串的缓存机制。

### (6) 时间戳的跨平台实现

```c++
#ifdef _USING_MSVC
    GetSystemTimeAsFileTime(&ft);
#else
    ::gettimeofday(&tv, nullptr);
#endif
```

- Windows 与 unix 系统的差异处理；
- `gettimeofday` 获取微秒级时间；
- 时间戳格式化时用 `gmtime_r` / `gmtime_s` 区分平台线程安全版本。

👉 **重点理解：**

- 平台条件编译 (`#ifdef`)；
- `tm` 结构体与时间字符串格式化

## 3.2 窗口

### 3.2.1 什么是 GLFW

> [GLFW（Graphics Library Framework）](https://www.glfw.org/)是一个专为 OpenGL / Vulkan 程序设计的轻量级库，用于创建窗口、管理输入、处理上下文。

它的主要职责是：

- 创建和管理 **窗口**；
- 创建 **OpenGL 上下文**；
- 处理 **键盘、鼠标、手柄输入事件**。

### 3.2.2 GLFW 与 OpenGL 的关系与区别

GLFW **不负责图形渲染本身**，它只是提供图形上下文与事件系统，真正绘制图形的工作由 **OpenGL** 完成。

| 对比项 | GLFW                         | OpenGL                                 |
| ------ | ---------------------------- | -------------------------------------- |
| 功能   | 创建窗口、输入管理、事件循环 | 渲染图形（绘制三角形、纹理、着色器等） |
| 层级   | 系统层（操作系统 API 封装）  | GPU 层（显卡指令集标准）               |
| 关系   | GLFW 创建一个“OpenGL 上下文” | OpenGL 在这个上下文中绘制内容          |
| 替代品 | SDL、SFML、Win32 API、GLUT   | Vulkan、Metal、DirectX                 |

#### 概念区分

- **GLFW** → 管理“窗口”和“上下文”
- **OpenGL** → 真正执行绘制命令

一句话概括：

> GLFW 打开一张纸，OpenGL 在上面画图。

#### 运行时关系

当执行：

```c++
glfwMakeContextCurrent(window);
```

时，GLFW 调用系统 API（如 `wglMakeCurrent` 或 `glXMakeCurrent`）在当前线程绑定 OpenGL 上下文。
 之后所有的 OpenGL 指令才会生效。

### 3.2.3 关键 API

| API                        | 作用               | 备注                                               |
| -------------------------- | ------------------ | -------------------------------------------------- |
| `glfwInit()`               | 初始化 GLFW        | 必须第一个调用，否则其他函数无效                   |
| `glfwCreateWindow()`       | 创建窗口和上下文   | 第三个参数是窗口标题，后两个可用于多窗口共享上下文 |
| `glfwMakeContextCurrent()` | 激活 OpenGL 上下文 | OpenGL 的所有绘制操作都在“当前上下文”中进行        |
| `glfwWindowShouldClose()`  | 检测窗口是否被关闭 | 由用户关闭窗口或按 `ESC` 时触发                    |
| `glfwSwapBuffers()`        | 交换前后帧缓冲区   | 将绘制完成的帧显示到屏幕                           |
| `glfwPollEvents()`         | 处理输入与窗口事件 | 内部可能触发回调函数                               |
| `glClear()`                | 清除上一帧内容     | 一般每帧调用一次                                   |

### 3.2.4 窗口主循环

主循环的逻辑：

```c++
while (!glfwWindowShouldClose(window)) {
    // 渲染
    glClear(GL_COLOR_BUFFER_BIT);
    // 绘制指令
    // ...
    glfwSwapBuffers(window);
    
    // 处理窗口事件
    glfwPollEvents();
}
```

每一帧循环包含三步：

1. **渲染内容**（`glClear` + 绘制）
2. **显示帧缓冲**（`glfwSwapBuffers`）
3. **处理输入事件**（`glfwPollEvents`）

虽然程序一直在循环，但是CPU 占用率几乎为 0：

`glfwSwapBuffers()` 通常会隐式调用 **垂直同步（VSync）**。VSync 让程序等待屏幕刷新（通常取决于显示器的刷新率），从而自动“限帧”；因此主循环实际是“被动等待”，而不是全速空转；如果禁用 VSync（通过 `glfwSwapInterval(0)`），你会看到 CPU 占用率瞬间升高。

### 3.2.5 GLFW 输入系统概览

GLFW 提供两种输入处理方式：

| 模式                     | 特点                             | 示例                                   | 适用场景                 |
| ------------------------ | -------------------------------- | -------------------------------------- | ------------------------ |
| **事件回调（Callback）** | 当用户触发输入时自动调用回调函数 | `glfwSetKeyCallback()`                 | GUI 程序、工具类应用     |
| **主动轮询（Polling）**  | 每帧手动查询输入状态             | `glfwGetKey()`、`glfwGetMouseButton()` | 游戏、模拟器循环（推荐） |

在游戏或模拟器中，我们通常使用 **轮询模式（Polling）**，因为：

- 我们的主循环已经是固定帧率结构；
- NES 输入本身也是“每帧查询手柄状态”；
- 回调方式更复杂。

```c++
if (glfwGetKey(window, GLFW_KEY_A)) {
	LOG_TRACE("A pressed");
}
```

###  3.2.6 OpenGL 渲染

#### 什么是“图元”？

在 OpenGL 中，**所有可见的图像**都是由最基本的**几何单元（Primitive）**组成的，比如：

| 图元类型       | 含义     | 示例用途                     |
| -------------- | -------- | ---------------------------- |
| `GL_POINTS`    | 绘制点   | 粒子效果                     |
| `GL_LINES`     | 绘制线段 | 边框、网格                   |
| `GL_TRIANGLES` | 三角形   | 所有现代 3D 图形的基本单位   |
| `GL_QUADS`     | 四边形   | 适合 2D 画面显示（本例使用） |

OpenGL 渲染时，其实并不懂“屏幕”或“图像”，它只是：

1. 根据你提供的**顶点坐标（Vertex）**画出几何面；
2. 用**纹理坐标（TexCoord）**决定每个像素在纹理中取什么颜色。

> “NES 屏幕画面”其实就是贴在一个长方形上的一张图。 

#### OpenGL 纹理

> 相关接口：`glGenTextures` / `glBindTexture` / `glTexImage2D`

- **纹理（Texture）** 是 GPU 上存储图像数据的显存资源；
- 使用前必须：
  1. `glGenTextures`：分配 ID；
  2. `glBindTexture`：绑定为当前操作对象；
  3. `glTexImage2D`：真正创建显存并上传数据；

#### 纹理坐标与顶点坐标的关系

在固定管线中，坐标空间分两种：

| 类型                      | 范围        | 含义                         |
| ------------------------- | ----------- | ---------------------------- |
| 顶点坐标 (`glVertex2f`)   | -1.0 ~ +1.0 | 显示在屏幕上的位置（归一化） |
| 纹理坐标 (`glTexCoord2f`) | 0.0 ~ 1.0   | 纹理图像的取样位置           |

> 举例：
>
> - `(0,0)` → 纹理左下角
> - `(1,1)` → 纹理右上角

当我们调用：

```c++
glTexCoord2f(0.0f, 1.0f);
glVertex2f(-1.0f, -1.0f);
```

这表示：

> “把纹理左下角的像素贴到屏幕左下角的位置。”

#### 纹理参数与最近点采样

> 对应代码：
>
> ```c++
> glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
> glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
> ```

- NES 使用像素级渲染（没有插值），因此应使用 **最近点采样（Nearest）**；
- 这样缩放后依然保持像素风格，不会出现模糊感。

#### 随机数据生成（模拟帧缓冲）

> 对应代码：
>
> ```c++
> std::vector<short> random_data(256 * 240);
> random_data[i] = engine() & 0xFFFF;
> ```

- 模拟一个“显存帧缓冲”；
- NES 最终渲染时，这个数据会由 PPU 生成；
- 使用随机数只是为了可视化。

使用glTexSubImage2D函数将纹理数据推送到GPU。

> 对应代码：
>
> ```c++
> glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 240, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, random_data.data());
> ```

- 不需要重新创建纹理，只更新像素数据；
- 参数说明：
  - `GL_RGB`：每像素三通道；
  - `GL_UNSIGNED_SHORT_5_6_5`：每像素16位，R5G6B5 格式；

#### 视口与显示比例

> ```c++
> const float nes_aspect = 256.f / 240.f;
> ...
> if (aspect > nes_aspect) w_scale = nes_aspect / aspect;
> else h_scale = aspect / nes_aspect;
> ```

- NES 分辨率比例 ≈ **1.0667 : 1**；
- 我们要让画面始终居中显示，不被拉伸；
- 根据窗口比例计算出缩放因子。

glViewport 与坐标系统

> ```c++
> glViewport(0, 0, w, h);
> glBegin(GL_QUADS);
> ...
> ```

`glViewport` 决定 OpenGL 渲染的像素区域，一般简单为窗口长宽；

## 3.3 libretro框架搭建

> libretro预声明好一组接口，用于我们实现前端跟核心之间的交互逻辑。

### 3.3.1 认识 libretro

我们首先从官方仓库下载头文件：

🔗 https://raw.githubusercontent.com/libretro/RetroArch/refs/heads/master/libretro-common/include/libretro.h

> 💡 这个文件定义了所有 **Libretro Core（核心）** 与 **前端（Frontend）** 通信所需的接口、回调与常量。

#### 什么是 Libretro？

Libretro 是一个**模拟器插件接口标准**。
 它把“模拟器核心”（Core）与“图形前端”（Frontend）分离，使得同一核心可以在不同前端中运行。

| 角色         | 功能                 | 举例                               |
| ------------ | -------------------- | ---------------------------------- |
| **Frontend** | 管理窗口、音频、输入 | RetroArch、BizHawk、你自己的程序   |
| **Core**     | 实际模拟游戏逻辑     | FCEUmm、Snes9x、mGBA、你的自制核心 |

> 🧩 简而言之：
>  Frontend 负责“跑环境”，Core 负责“跑游戏”。

#### 关键结构与函数

在 `libretro.h` 中最重要的是一组接口函数（需要在Core中 实现）：

```c++
void retro_init(); // 用于执行一些初始化工作
void retro_run();  // 用于实现模拟逻辑，一般由前端程序每帧调用一次 
bool retro_load_game(); // 用于实现游戏加载的逻辑
void retro_reset(); // 用于实现重启游戏的逻辑
void retro_set_xxx(); // 用于注册各种回调
```

以及一组“回调函数”接口（Frontend 实现并注册给 Core）：

```c++
retro_video_refresh_t video_cb;
retro_audio_sample_t audio_cb;
retro_input_poll_t input_poll_cb;
```

> 这两组函数构成了 Core 与 Frontend 之间的“双向通信通道”。

### 3.3.2 前端使用 Libretro 的基本流程

一个前端要正确加载并运行 Core，通常需要以下步骤：

| 步骤                       | 内容                                   |
| -------------------------- | -------------------------------------- |
| 1️⃣ 加载核心动态库           | `dlopen("core.dll")` / `LoadLibrary()` |
| 2️⃣ 解析接口函数             | `dlsym()` 获取 `retro_init` 等函数指针 |
| 3️⃣ 设置回调函数             | `retro_set_video_refresh()` 等         |
| 4️⃣ 调用 `retro_load_game()` | 传入 ROM 文件路径（或者ROM数据）       |
| 5️⃣ 主循环调用 `retro_run()` | 执行每帧模拟逻辑与渲染                 |
| 6️⃣ 关闭核心                 | ``dlclose("core.dll")`                 |

> 💡 这其实就是 RetroArch 的基本框架，本项目仅通过静态链接核心来实现一个极简版本。

**实现封装类 `SimpleRetro`**

用一个 C++ 单例类封装所有 libretro 交互，方便前端使用。

核心功能：

1. **构造函数**
   - 解析命令行参数，加载 ROM 文件；
   - 初始化 libretro Core；
   - 注册回调（视频、输入、音频等）。
2. **`setVideoRefresh()`**
   - 设置前端视频回调接口；
   - 内部保存函数指针，供 Core 调用。
3. **`run()`**
   - 主循环：调用 `retro_run()`；
   - 每帧 Core 会调用 `video_refresh()` 输出画面。
4. **`video_refresh()`（静态函数）**
   - 作为 libretro 的视频回调适配器；
   - 转发到前端提供的函数，用 GLFW 显示图像。

这个类充当一个框架的作用，后面随着进度推进，会逐步扩展这个类。

### 3.3.3 在Core中实现libretro

Core 需要实现 `libretro.h` 中的所有标准函数。
 下面是一个最小可运行的示例（不包含任何真实模拟逻辑）：

```c++
#include "libretro.h"
#include <string.h>

static retro_video_refresh_t video_cb = nullptr;

void retro_init(void) {}
void retro_set_video_refresh(retro_video_refresh_t cb) { video_cb = cb; }

bool retro_load_game(const struct retro_game_info *game)
{
    // 打印游戏信息
    printf("Loaded game: %s (%zu bytes)\n", game->path, game->size);
    return true;
}
void retro_run(void)
{
    static uint32_t dummy_frame[256 * 240];
    memset(dummy_frame, 0x80, sizeof(dummy_frame)); // 灰色背景
    if (video_cb)
        video_cb(dummy_frame, 256, 240, 256 * 4);
}
void retro_reset(void) {}
```

> 💡 这段代码每帧都会输出一张灰色图像到前端，说明整个调用链已经成功建立。