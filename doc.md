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

# 4.CPU模拟

## 4.1 硬件概述

- **型号**：Ricoh 2A03（北美 NES）或 Ricoh 2A07（欧洲 PAL NES）
- 这两款芯片均基于 **MOS 6502** 架构，但有以下差异：
  - **去掉了 BCD（十进制模式）指令支持**；
  - **集成了 APU（音频处理单元）**；
  - 时钟频率略有差异：
    - NTSC（2A03）：约 1.79 MHz
    - PAL（2A07）：约 1.66 MHz

NES 的 CPU 是一颗 **8 位处理器**，但可以寻址 **16 位地址空间（64KB）**。

2A03 的主要寄存器如下：

| 名称   | 位宽  | 含义                                                |
| ------ | ----- | --------------------------------------------------- |
| **A**  | 8 位  | 累加器（Accumulator），用于算术和逻辑操作           |
| **X**  | 8 位  | 索引寄存器 X，用于偏移寻址                          |
| **Y**  | 8 位  | 索引寄存器 Y，用于偏移寻址                          |
| **PC** | 16 位 | 程序计数器（Program Counter），存储下一条指令的地址 |
| **SP** | 8 位  | 栈指针（Stack Pointer），指向 $0100–$01FF 的栈区    |
| **P**  | 8 位  | 状态寄存器（Processor Status）                      |

状态寄存器的各位含义如下：

| 位   | 名称              | 标志 | 说明                         |
| ---- | ----------------- | ---- | ---------------------------- |
| 7    | Negative          | N    | 运算结果为负                 |
| 6    | Overflow          | V    | 溢出                         |
| 5    | (unused)          | —    | 保持为 1                     |
| 4    | Break             | B    | 表示 BRK 指令执行            |
| 3    | Decimal           | D    | 十进制模式（在 2A03 中无效） |
| 2    | Interrupt Disable | I    | 禁止中断                     |
| 1    | Zero              | Z    | 运算结果为 0                 |
| 0    | Carry             | C    | 进位/借位                    |

## 4.2 CPU总线模型

NES 的 CPU 是一个独立的逻辑单元，本身**不直接知道外部设备的存在**。
 CPU 对外的唯一接口是三条总线：

1. **地址总线（Address Bus, 16 位）**
   - 输出信号，CPU 用它指定访问的内存或外设地址（$0000–$FFFF）。
2. **数据总线（Data Bus, 8 位）**
   - 双向信号线，用于数据读写。
3. **控制信号线**
   - 如 `R/W`（读/写控制）、`IRQ`、`NMI`、`RESET` 等。

> 在模拟中，CPU 本身只负责：
>
> - 发出 “我要访问地址 $XXXX”；
> - 读取或写入一个字节；
> - 外部由“总线逻辑”决定这个地址映射到哪（RAM、PPU 寄存器、APU 寄存器或 PRG ROM）。

在模拟器实现中，通常会把“总线”抽象为一组接口，例如：

```c
uint8_t bus_read(uint16_t addr); 			// 模拟cpu通过总线从外部单元读区数据
void bus_write(uint16_t addr, uint8_t data); // 模拟cpu通过总线向外部单元写数据
```

这样即可让 CPU 模拟逻辑自然地反映硬件结构。CPU 在执行指令时不直接访问内存，而是通过 `bus_read()` / `bus_write()` 来与外部通信。

## 4.3 指令执行基本流程

### 4.3.1 6502 的三阶段执行模型

一条指令通常分为三个阶段：

1. **取指（Fetch）**
   - 从程序计数器（PC）指向的地址取出操作码（opcode）。
   - PC 自动递增。
2. **寻址（Decode / Addressing）**
   - 根据操作码确定该指令的寻址方式（立即数、零页、绝对、间接、索引等）。
   - 从操作数中计算出有效地址（Effective Address）。
3. **执行（Execute）**
   - 对操作数执行指令对应的功能（算术、逻辑、跳转、读写等）。
   - 更新寄存器、标志位、PC 等状态。

------

### 4.3.2  举例：`LDA $C000` 的执行过程

假设内存 $C000 存放了一个字节 0x42。

1. **取指**
   - PC 指向 $8000，CPU 从总线读取指令码：`LDA Absolute` → 操作码 `$AD`
   - PC 递增 1
2. **寻址**
   - CPU 再连续读取两个字节作为地址低高位：`00 C0` → 地址 $C000
   - PC 递增 2
3. **执行**
   - CPU 通过总线读取 $C000 的内容（0x42）到 A 寄存器
   - 更新 Zero/Negative 标志
   - 指令结束，等待下一条取指

## 4.4 寻址模式详解（12 种）

### 1) IMP — Implied（隐含寻址）

- **含义**：操作数隐含在指令或寄存器中，不需额外字节。

- **机器码字节数**：0（只有 opcode）

- **如何执行**：指令本身决定作用对象（如 `CLC`，`INX`，`BRK` 的行为不同）。

- **示例**：`INX`（使 X 寄存器加 1）

- **实现提示（伪码）**：

  ```cpp
  // 已经取到 opcode，直接调用对应操作函数
  CPU::INX() { X = (X + 1) & 0xFF; setZN(X); }
  ```

- **备注**：没有内存有效地址计算。

------

### 2) IMM — Immediate（立即数）

- **含义**：操作数是紧随 opcode 的一个字节，直接作为数据使用。

- **机器码字节数**：1（操作码后跟 1 字节）

- **有效地址**：没有“地址”，而是立即值 `value = bus.read(PC++)`

- **示例**：`LDA #$42` （把 0x42 载入 A）

- **伪码**：

  ```cpp
  uint8_t operand = bus.read(PC++);
  A = operand;
  setZN(A);
  ```

- **备注**：没有页面穿越惩罚。

------

### 3) ZP0 — Zero Page（零页）

- **含义**：操作数是一个 8 位地址（位于 $0000–$00FF），访问速度较快（6502 专门优化）。

- **机器码字节数**：1（后跟 1 字节的零页地址）

- **有效地址计算**：

  1. `addr = bus.read(PC++)`  （0x00–0xFF）
  2. 实际访问地址为 `0x00XX`（高字节 0）

- **示例**：`LDA $10` （从 $0010 读）

- **伪码**：

  ```cpp
  uint8_t zp = bus.read(PC++);
  uint16_t addr = zp; // implicit 0x00 high byte
  A = bus.read(addr);
  setZN(A);
  ```

- **备注**：如果实现使用数组（size 256）对零页直接访问会更简单。

------

### 4) ZPX — Zero Page, X（零页,X）

- **含义**：在零页地址上加 X（结果在零页范围内，按 8 位溢出，即 0xFF + 1 -> 0x00）。

- **字节数**：1

- **有效地址计算**：

  1. `zp = bus.read(PC++)`
  2. `addr = (zp + X) & 0xFF`（仅低 8 位，零页环绕）
  3. 访问 `0x00XX`

- **示例**：`STA $80,X`（把 A 存到 $0080 + X）

- **伪码**：

  ```cpp
  uint8_t zp = bus.read(PC++);
  uint16_t addr = (uint8_t)(zp + X); // wrap in zero page
  bus.write(addr, A);
  ```

- **注意**：**零页环绕**是重要行为 —— 加法只保留低 8 位。

------

### 5) ZPY — Zero Page, Y（零页,Y）

- **含义**：同 ZPX，但用 Y 寄存器做偏移。
- **字节数**：1
- **有效地址计算**：
  - `addr = (bus.read(PC++) + Y) & 0xFF`
- **示例**：`LDA $20,Y`
- **伪码** 与 ZPX 类似，只替换 X 为 Y。

------

### 6) REL — Relative（相对，主要用于分支）

- **含义**：用于分支指令（`BEQ`, `BNE`, `BCS`, `BCC` 等），操作数是带符号的 8 位偏移（-128..+127），相对于分支指令后的下一个地址（PC 已指向下一字节）。

- **字节数**：1（带符号偏移）

- **地址计算**：

  1. `offset = (int8_t)bus.read(PC++)` （把 8 位看成有符号）
  2. 如果条件满足：`target = PC + offset`（`PC` 此处为指向偏移字节后的位置）

- **周期与惩罚**：

  - 如果分支**不成立**：只消耗基础周期（通常 2）。
  - 如果**成立**：+1 个周期；
  - 如果分支目标与 `PC` 在不同页（高字节不同），再 +1 个周期（即总共可能多 +2）。

- **示例**：`BEQ label`（当 Z=1 时分支）

- **伪码**：

  ```cpp
  int8_t rel = (int8_t)bus.read(PC++);
  if (condition) {
      uint16_t oldPC = PC;
      PC += rel;
      cycles += 1;
      if ((oldPC & 0xFF00) != (PC & 0xFF00)) cycles += 1;
  }
  ```

------

### 7) ABS — Absolute（绝对）

- **含义**：操作数是 16 位绝对地址（两字节：低字节先，高字节后）。

- **字节数**：2

- **有效地址计算**：

  1. `lo = bus.read(PC++)`
  2. `hi = bus.read(PC++)`
  3. `addr = (hi << 8) | lo`

- **示例**：`LDA $C000`

- **伪码**：

  ```cpp
  uint16_t lo = bus.read(PC++);
  uint16_t hi = bus.read(PC++);
  uint16_t addr = (hi << 8) | lo;
  A = bus.read(addr);
  setZN(A);
  ```

- **备注**：没有零页环绕或特殊 bug。

------

### 8) ABX — Absolute, X（绝对,X）

- **含义**：从绝对地址加上 X 寄存器得到最终地址：`addr = base + X`。

- **字节数**：2

- **有效地址计算**：

  1. 读取 16 位基地址 `base`（同 ABS）
  2. `addr = base + X`

- **周期惩罚**：

  - 如果 `base` 与 `addr` 跨页（即 `(base & 0xFF00) != (addr & 0xFF00)`），某些指令会额外消耗 1 个周期（常见于读内存的指令，如 LDA）。对于写入指令（STA）并不会有额外周期（因为 STA 在计算地址时通常已经完成写入步骤，但实现细节视表格而定）。

- **示例**：`LDA $2000,X`

- **伪码**：

  ```cpp
  uint16_t base = bus.read(PC++) | (bus.read(PC++) << 8);
  uint16_t addr = base + X;
  // if page crossed, cycles++
  A = bus.read(addr);
  setZN(A);
  ```

------

### 9) ABY — Absolute, Y（绝对,Y）

- **含义**：与 ABX 类似，但用 Y 寄存器偏移。
- **字节数**：2
- **行为**：与 ABX 完全对应（也同样会在读类指令时因为跨页而 +1 周期）。
- **示例**：`LDA $4000,Y`

### 10) IND — Indirect（间接，主要用于 JMP (ind)）

- **含义**：操作数给出一个指针（16 位），CPU 从该指针位置读取真实的跳转地址（JMP (addr)）。

- **字节数**：2

- **地址计算**：

  1. 读取 `ptr_lo = bus.read(PC++)`、`ptr_hi = bus.read(PC++)` ⇒ `ptr = (ptr_hi << 8) | ptr_lo`
  2. **标准想法**：`target = read16(ptr)`（读 ptr 与 ptr+1 的两个字节）

- **6502 特殊 bug（必须在 NES 模拟器中复现）**：

  - 如果 `ptr_lo == 0xFF`（即指针位于页面末尾，例如 `$02FF`），硬件会把高字节从同一页面的地址 `$0200` 读取（而不是 `$0300`）。换言之，高字节读取会发生**页面边界回绕**，即：
    - `low = bus.read(ptr)`
    - `high = bus.read(ptr & 0xFF00)` （而不是 `ptr + 1`）
  - 这个 bug 只影响 `JMP (addr)` 的间接寻址，模拟器常被要求**精确模拟**这个行为以确保兼容性（部分游戏利用这个 bug）。

- **示例**：`JMP ($C000)`（跳转到存储在 $C000/$C001 的地址）

- **伪码（含 bug 复现）**：

  ```cpp
  uint16_t ptr = bus.read(PC++) | (bus.read(PC++) << 8);
  uint8_t lo = bus.read(ptr);
  uint8_t hi;
  if ((ptr & 0x00FF) == 0x00FF) {
      // 6502 bug: high byte fetched from beginning of same page
      hi = bus.read(ptr & 0xFF00);
  } else {
      hi = bus.read(ptr + 1);
  }
  uint16_t target = (hi << 8) | lo;
  PC = target;
  ```

------

### 11) IZX — Indexed Indirect (Zero Page, X)（零页索引间接，常写为 (zp,X)）

- **含义**：先将零页地址与 X 相加（零页内环绕），结果是指向 16 位地址指针的零页位置；再从该指针读取低/高字节得到最终地址，最后对该地址进行内存访问。

- **机器码字节数**：1（零页地址）

- **有效地址计算（step-by-step）**：

  1. `zp = bus.read(PC++)`
  2. `ptr = (zp + X) & 0xFF`  （**零页环绕**）
  3. `lo = bus.read(ptr)`
  4. `hi = bus.read((ptr + 1) & 0xFF)`  （读取高字节也在零页内环绕）
  5. `addr = (hi << 8) | lo`

- **示例**：`LDA ($20,X)`：先在零页地址 `$20 + X` 处读指针，再从指针地址读取数据到 A。

- **伪码**：

  ```cpp
  uint8_t zp = bus.read(PC++);
  uint8_t ptr = (uint8_t)(zp + X); // wrap in zero page
  uint16_t lo = bus.read(ptr);
  uint16_t hi = bus.read((uint8_t)(ptr + 1));
  uint16_t addr = (hi << 8) | lo;
  A = bus.read(addr);
  setZN(A);
  ```

- **备注**：页面穿越惩罚 **不适用**，因为最后的 `addr` 是完整地址，任何由基址计算产生的跨页并不产生额外周期（这是 IZX 的常见特点）。

------

### 12) IZY — Indirect Indexed (Zero Page, Y)（零页间接索引，常写为 (zp),Y）

- **含义**：先从零页指针读取 16 位基址（低/高字节），然后把 Y 加到该基址上得到最终地址（这一步可能跨页）。

- **机器码字节数**：1

- **有效地址计算**：

  1. `zp = bus.read(PC++)`
  2. `lo = bus.read(zp)` （零页读）
  3. `hi = bus.read((zp + 1) & 0xFF)` （零页环绕读高字节）
  4. `base = (hi << 8) | lo`
  5. `addr = base + Y`

- **周期惩罚**：

  - 如果 `base` 与 `addr` 跨页（`(base & 0xFF00) != (addr & 0xFF00)`），某些指令会多花 1 个周期（例如 `LDA ($20),Y` 在跨页时 +1）。

- **示例**：`LDA ($30),Y`

- **伪码**：

  ```cpp
  uint8_t zp = bus.read(PC++);
  uint16_t lo = bus.read(zp);
  uint16_t hi = bus.read((uint8_t)(zp + 1));
  uint16_t base = (hi << 8) | lo;
  uint16_t addr = base + Y;
  // if page crossed, cycles++
  A = bus.read(addr);
  setZN(A);
  ```

------

### 汇总（简短表格）

| 模式 | 字节数 | 基本描述             | 零页环绕          | 页穿越惩罚                  | 典型用途          |
| ---- | ------ | -------------------- | ----------------- | --------------------------- | ----------------- |
| IMP  | 0      | 隐含（寄存器/内部）  | —                 | —                           | 控制/堆栈/标志类  |
| IMM  | 1      | 立即数               | —                 | —                           | 立刻用值（LDA #） |
| ZP0  | 1      | 零页直接             | —                 | —                           | 快速内存访问      |
| ZPX  | 1      | 零页 + X（8 位环绕） | ✅                 | —                           | 表/数组索引       |
| ZPY  | 1      | 零页 + Y（8 位环绕） | ✅                 | —                           | 同上              |
| REL  | 1      | 分支，相对（带符号） | —                 | ✅（分支成立 + 页跨越再 +1） | BEQ/BNE 等        |
| ABS  | 2      | 16 位地址            | —                 | —                           | 直接读写内存      |
| ABX  | 2      | ABS + X              | —                 | ✅（读类指令）               | 访问数组          |
| ABY  | 2      | ABS + Y              | —                 | ✅（读类指令）               | 访问数组          |
| IND  | 2      | JMP (ptr) — 间接     | 某种边界 bug      | —（但有特殊 bug）           | JMP 指针跳转      |
| IZX  | 1      | (zp,X)：零页索引间接 | ✅（指针读取环绕） | —                           | 间接寻址          |
| IZY  | 1      | (zp),Y：零页间接 + Y | ✅（指针读取环绕） | ✅（基址+Y 跨页）            | 间接寻址          |

## 4.5 6502指令集

本节将详细说明 NES（Ricoh 2A03）CPU 支持的主要指令。
 每个类别先介绍通用行为与标志位影响，然后选取代表性指令展开说明。

------

### 1) Access

> 访问类：LDA、STA、LDX、STX、LDY、STY

**功能说明**

- `LDx`（Load）指令：将内存内容载入寄存器；
- `STx`（Store）指令：将寄存器内容存入内存。

| 指令    | 功能            | 影响标志 |
| ------- | --------------- | -------- |
| **LDA** | 内存 → 累加器 A | Z, N     |
| **LDX** | 内存 → X 寄存器 | Z, N     |
| **LDY** | 内存 → Y 寄存器 | Z, N     |
| **STA** | 累加器 A → 内存 | 无       |
| **STX** | X 寄存器 → 内存 | 无       |
| **STY** | Y 寄存器 → 内存 | 无       |

**示例：LDA**

```cpp
void CPU::LDA() {
    A = fetched;      // fetched = 由寻址模式确定的值
    setFlag(Z, A == 0);
    setFlag(N, A & 0x80);
}
```

> `LDX`、`LDY` 实现类似，只是目标寄存器不同。

### 2) Transfer

> 传送类：TAX、TXA、TAY、TYA

**功能说明**

- 这些指令在寄存器之间传送数据，不访问内存。
- 全部会影响 Z/N 标志。

| 指令    | 功能  | 影响标志 |
| ------- | ----- | -------- |
| **TAX** | A → X | Z, N     |
| **TXA** | X → A | Z, N     |
| **TAY** | A → Y | Z, N     |
| **TYA** | Y → A | Z, N     |

**示例：TAX**

```cpp
void CPU::TAX() {
    X = A;
    setFlag(Z, X == 0);
    setFlag(N, X & 0x80);
}
```

------

### 2) Arithmetic

> 算术类：ADC、SBC、INC、DEC、INX、DEX、INY、DEY

**功能说明**

- 算术类指令执行加减或自增自减操作；
- 所有算术操作都会影响 Z、N、C、V（视情况）标志。

------

**✅ ADC（Add with Carry）**

执行：`A = A + M + C`

| 标志 | 含义                        |
| ---- | --------------------------- |
| C    | 进位（结果超过 255 时设 1） |
| V    | 溢出（符号位异常变化）      |
| Z    | 结果为 0                    |
| N    | 结果为负                    |

```cpp
void CPU::ADC() {
    uint16_t sum = A + fetched + getFlag(C);
    setFlag(C, sum > 0xFF);
    setFlag(Z, (sum & 0xFF) == 0);
    setFlag(V, (~(A ^ fetched) & (A ^ sum) & 0x80));
    setFlag(N, sum & 0x80);
    A = sum & 0xFF;
}
```

------

**✅ SBC（Subtract with Carry）**

6502 的减法通过补码加法实现：
 `A = A - M - (1 - C)` ≡ `A + (~M) + C`

```cpp
void CPU::SBC() {
    uint16_t value = fetched ^ 0xFF;
    uint16_t sum = A + value + getFlag(C);
    setFlag(C, sum > 0xFF);
    setFlag(Z, (sum & 0xFF) == 0);
    setFlag(V, (sum ^ A) & (sum ^ value) & 0x80);
    setFlag(N, sum & 0x80);
    A = sum & 0xFF;
}
```

------

**✅ INC / DEC（内存自增自减）**

```cpp
void CPU::INC() {
    uint8_t v = bus.read(addr_abs) + 1;
    bus.write(addr_abs, v);
    setFlag(Z, v == 0);
    setFlag(N, v & 0x80);
}
```

`DEC` 逻辑相同但减 1。

------

**✅ INX / DEX / INY / DEY（寄存器自增自减）**

```cpp
void CPU::INX() { X++; setFlag(Z, X == 0); setFlag(N, X & 0x80); }
void CPU::DEX() { X--; setFlag(Z, X == 0); setFlag(N, X & 0x80); }
```

------

### 4) Shift

> 移位类：ASL、LSR、ROL、ROR

**功能说明**

- 对寄存器或内存内容执行逻辑移位。
- 通常影响 C、Z、N 标志。

| 指令    | 操作             | C 标志                           | 说明         |
| ------- | ---------------- | -------------------------------- | ------------ |
| **ASL** | 左移一位         | 移出的最高位                     | 乘 2         |
| **LSR** | 右移一位         | 移出的最低位                     | 除 2（逻辑） |
| **ROL** | 循环左移，含进位 | 原最高位进 Carry，Carry 入最低位 |              |
| **ROR** | 循环右移，含进位 | 原最低位进 Carry，Carry 入最高位 |              |

**示例：ASL**

```cpp
void CPU::ASL() {
    uint16_t res = (uint16_t)fetched << 1;
    setFlag(C, res & 0x100);
    setFlag(Z, (res & 0xFF) == 0);
    setFlag(N, res & 0x80);
    write(addr_abs, res & 0xFF);
}
```

------

### 5) Bitwise

> 按位操作类：AND、ORA、EOR、BIT

| 指令    | 操作             | 影响标志          |
| ------- | ---------------- | ----------------- |
| **AND** | `A = A & M`      | Z, N              |
| **ORA** | `A = A      | M` |                   |
| **EOR** | `A = A ^ M`      | Z, N              |
| **BIT** | 特殊测试位       | Z, N=bit7, V=bit6 |

**示例：BIT**

```cpp
void CPU::BIT() {
    uint8_t v = bus.read(addr_abs);
    setFlag(Z, (A & v) == 0);
    setFlag(N, v & 0x80);
    setFlag(V, v & 0x40);
}
```

------

### 6) Compare

> 比较类：CMP、CPX、CPY

- 比较实质是执行 `A - M`（或 `X - M`、`Y - M`）但不保存结果；
- 影响标志：C, Z, N。

```cpp
void CPU::CMP() {
    uint16_t tmp = A - fetched;
    setFlag(C, A >= fetched);
    setFlag(Z, (tmp & 0xFF) == 0);
    setFlag(N, tmp & 0x80);
}
```

`CPX` / `CPY` 同理。

------

### 7) Branch

> 分支类：BCC、BCS、BEQ、BNE、BPL、BMI、BVC、BVS

- 全部使用 **Relative 寻址**；
- 如果条件满足，修改 PC；
- 若目标地址跨页，多消耗一个周期。

| 指令    | 条件         |
| ------- | ------------ |
| **BCC** | Carry = 0    |
| **BCS** | Carry = 1    |
| **BEQ** | Zero = 1     |
| **BNE** | Zero = 0     |
| **BPL** | Negative = 0 |
| **BMI** | Negative = 1 |
| **BVC** | Overflow = 0 |
| **BVS** | Overflow = 1 |

**示例：BEQ3**

```cpp
void CPU::BEQ() {
    if (getFlag(Z)) {
        cycles++;
        uint16_t oldPC = PC;
        PC += addr_rel;
        if ((oldPC & 0xFF00) != (PC & 0xFF00)) cycles++;
    }
}
```

------

### 8) Jump

> 跳转类：JMP、JSR、RTS、BRK、RTI

**JMP**

```cpp
void CPU::JMP() { PC = addr_abs; }
```

**JSR / RTS**

- **JSR**：压入返回地址（PC-1），跳转；
- **RTS**：从栈中弹出返回地址，加 1。

```cpp
void CPU::JSR() {
    PC--;
    push((PC >> 8) & 0xFF);
    push(PC & 0xFF);
    PC = addr_abs;
}
void CPU::RTS() {
    uint16_t lo = pull();
    uint16_t hi = pull();
    PC = ((hi << 8) | lo) + 1;
}
```

**BRK / RTI**

- **BRK**：软中断，压栈状态与返回地址；
- **RTI**：从栈恢复标志和 PC。

------

### 9) Stack

> 堆栈类：PHA、PLA、PHP、PLP、TXS、TSX

| 指令    | 功能           | 影响标志 |
| ------- | -------------- | -------- |
| **PHA** | 压入 A         | 无       |
| **PLA** | 弹出到 A       | Z, N     |
| **PHP** | 压入状态寄存器 | 无       |
| **PLP** | 弹出状态寄存器 | 所有     |
| **TXS** | X → 栈指针     | 无       |
| **TSX** | 栈指针 → X     | Z, N     |

```cpp
void CPU::PHA() { push(A); }
void CPU::PLA() { A = pull(); setFlag(Z, A==0); setFlag(N, A&0x80); }
```

------

### 10) Flags

> 标志位操作：CLC、SEC、CLI、SEI、CLD、SED、CLV

| 指令    | 功能                          | 修改标志 |
| ------- | ----------------------------- | -------- |
| **CLC** | 清除进位                      | C=0      |
| **SEC** | 置进位                        | C=1      |
| **CLI** | 启用中断                      | I=0      |
| **SEI** | 禁用中断                      | I=1      |
| **CLD** | 清除十进制模式（无效于 2A03） | D=0      |
| **SED** | 设置十进制模式（无效于 2A03） | D=1      |
| **CLV** | 清除溢出                      | V=0      |

> NES CPU（2A03）不支持十进制模式，因此 `CLD/SED` 实际无作用，但应保留指令实现。

------

### 11) Others（NOP、XXX）

**NOP**

- 空操作，不改变任何状态。
- 某些非法 opcode 在 NES 游戏中被用作特殊延迟或副作用操作（可选模拟）。

```cpp
void CPU::NOP() {}
```

**XXX**

- 占位符，代表未实现或非法指令。

## 4.6  模拟实现思路

要在模拟器中实现CPU执行指令的过程，有一种常见的思路，即维护一个操作码表，当从读取到操作码后，通过查表，找到该操作码的寻址方式和具体指令。如下所示，`__operations`是一个`operation`结构体的数组，用于注册操作码和具体的寻址函数，指令函数和执行该操作码所需要的时钟周期。

```c
struct operation {
    void (*addressing_func)(void);
    void (*instruction_func)(void);
    u64  instruction_code : 8;
    u64  addressing_code : 8;
    u64  cycles : 8;
    u64  __padding : 40;
};

// 注册操作码
struct operation __operations[] = { 
    OP( BRK, IMM, 7 ), OP( ORA, IZX, 6 ), OP( XXX, IMP, 2 ), OP( XXX, IMP, 8 ),
    ...
};
```

有了操作码表，我们就可以简单地用下面几行代码来模拟CPU执行指令了。

```c
void cpu_clock() {
    u8 opcode = bus_read(__pc++);
    remain_cycles = __operations[opcode].cycles;
    __operations[opcode].addressing_func();
    __operations[opcode].instruction_func();
}
```

> 操作码表参考：https://www.oxyron.de/html/opcodes02.html

## 4.7 CPU 地址空间详解

NES 主机的 CPU（Ricoh 2A03）是基于 MOS 6502 的定制版本，它采用 **16 位地址总线**，因此理论可寻址范围为 **$0000~$FFFF（64KB）**。
但 NES 实际上并没有这么多物理内存。整个地址空间被映射到多个不同的功能模块中（RAM、PPU、APU、扩展 RAM、ROM 等），如下图所示：

```
+-------------------+  $FFFF  ┐
|       PRG ROM     |         │
|  （上半区映射）    |         │  通常为 16KB 或 32KB，可被 mapper 动态切换
+-------------------+  $8000  ┘
|       WRAM        |  卡带内电池RAM（部分游戏）
+-------------------+  $6000
|       Unused      |  未使用或Mapper寄存器
+-------------------+  $4020
|   APU / I/O port  |  声音寄存器 + 控制器端口
+-------------------+  $4000
|   PPU Registers   |  映射至显卡寄存器，8字节镜像
+-------------------+  $2000
|       RAM         |  主机内置2KB RAM，重复映射
+-------------------+  $0000
```

### 4.7.1 $0000~$1FFF — 内部 RAM（CPU System RAM）

- **大小**：2KB（2048字节）
- **镜像范围**：$0000~$07FF 为真实 RAM；整个 $0000~$1FFF 为 8 次镜像。
- **原因**：CPU 地址线 A11 以上未连接到 RAM 芯片，导致寻址重复。

| 地址范围    | 功能            | 备注                     |
| ----------- | --------------- | ------------------------ |
| $0000–$07FF | 物理 RAM（2KB） | 存储栈、变量、缓冲数据等 |
| $0800–$0FFF | RAM 镜像        | 与 $0000–$07FF 同内容    |
| $1000–$17FF | RAM 镜像        | 同上                     |
| $1800–$1FFF | RAM 镜像        | 同上                     |

**实现示例：**

```
uint8_t CPU::read(uint16_t addr) {
    if (addr < 0x2000)
        return ram[addr & 0x07FF]; // 8次镜像
    ...
}
```

------

### 4.7.2 $2000~$3FFF — PPU 寄存器区域（PPU_IO）

- **寄存器数量**：8 个（$2000~$2007）
- **镜像规律**：整个 $2000~$3FFF 每 8 字节镜像一次（A3~A13 未使用）
- **主要功能**：CPU 通过这些寄存器与 PPU（Picture Processing Unit）通信

| 地址  | 名称      | 功能                                             |
| ----- | --------- | ------------------------------------------------ |
| $2000 | PPUCTRL   | 控制寄存器 #1（设置VBlank中断、背景/图像基址等） |
| $2001 | PPUMASK   | 显示控制（开启背景/精灵、调色等）                |
| $2002 | PPUSTATUS | 状态寄存器（VBlank 标志等）                      |
| $2003 | OAMADDR   | 精灵表内存地址                                   |
| $2004 | OAMDATA   | 精灵数据读写端口                                 |
| $2005 | PPUSCROLL | 背景滚动偏移                                     |
| $2006 | PPUADDR   | VRAM 地址端口                                    |
| $2007 | PPUDATA   | VRAM 数据端口                                    |

**镜像示例：**

```
if (addr >= 0x2000 && addr <= 0x3FFF)
    return ppu.readRegister(addr & 0x2007);
```

------

### 4.7.3 $4000~$401F — APU 与 I/O（APU_IO）

这一段包括声音控制、手柄输入、DMA 传输等多种功能。

| 地址        | 功能                                                     |
| ----------- | -------------------------------------------------------- |
| $4000–$400F | APU 声道控制寄存器（脉冲波、三角波、噪声等）             |
| $4010–$4013 | DMC 声音通道控制                                         |
| $4014       | OAM DMA 寄存器（将 $XX00~$XXFF 的256字节复制到 PPU OAM） |
| $4015       | APU 状态与声道开关                                       |
| $4016       | 控制器 #1 端口                                           |
| $4017       | 控制器 #2 端口 + APU 帧计数控制                          |
| $4018–$401F | 未使用或测试端口                                         |

> ⚙️ 注意：$4014 是 CPU 与 PPU 交互的重要桥梁，执行一次 DMA 会“冻结” CPU 大约 513 或 514 周期。

------

### 4.7.4 $4020~$5FFF — 未使用 / 扩展 / Mapper 寄存器区（UNUSE）

- 这一段在官方 NES 上没有统一功能；
- 有些 Mapper（卡带芯片）会在此区域实现自己的寄存器，如：
  - **MMC3** 的 IRQ 计数器控制；
  - **VRC 系列** 的银行切换寄存器；
  - **Namco 163** 的扩展声音寄存器；
- 也有部分卡带的扩展 RAM（WRAM）部分从 $4020 开始。

> 模拟器实现时，可把此区域的读写请求转发给 Mapper 模块，由 Mapper 决定行为。

------

### 4.7.5 $6000~$7FFF — WRAM（卡带电池备份 RAM）

- 通常用于 RPG 游戏保存进度（如《塞尔达传说》）
- **是否存在取决于卡带硬件**（多数 mapper 可控制启用或禁用）
- 典型容量：8KB
- 常由 Mapper 控制读写使能

```
if (addr >= 0x6000 && addr <= 0x7FFF)
    return mapper->readWRAM(addr);
```

------

### 4.7.6 $8000~$FFFF — PRG ROM（游戏程序区）

- **主程序与数据存放区**；
- 由卡带上的 ROM 芯片提供；
- 一般为 16KB 或 32KB；
- 大多数游戏通过 **Mapper** 来切换此区域的 ROM 数据块（bank switching），实现更大容量的程序存储。

常见分区方式：

| 地址范围    | 功能           | 说明                           |
| ----------- | -------------- | ------------------------------ |
| $8000–$BFFF | PRG-ROM Bank 0 | 可切换区域（多数 Mapper 控制） |
| $C000–$FFFF | PRG-ROM Bank 1 | 固定或切换区域                 |

例子：

对于没有 Mapper（即 NROM）卡带：

- 如果只有 16KB ROM，则 $C000–$FFFF 为 $8000–$BFFF 的镜像；
- 如果有 32KB ROM，则两部分独立。

### 地址空间访问总结表

| 地址范围    | 名称         | 说明                      |
| ----------- | ------------ | ------------------------- |
| $0000–$1FFF | 内部 RAM     | 2KB，8 次镜像             |
| $2000–$3FFF | PPU 寄存器   | 8 字节镜像                |
| $4000–$401F | APU / I/O    | 声音与控制器              |
| $4020–$5FFF | 扩展或未使用 | Mapper 扩展寄存器区       |
| $6000–$7FFF | WRAM         | 卡带内电池 RAM            |
| $8000–$FFFF | PRG-ROM      | 程序 ROM，Mapper 控制切换 |

------

### Mapper 的角色（ROM 区映射机制）

NES 的 ROM 并非固定连线，而是通过 **Mapper（映射芯片）** 动态控制哪些 PRG-ROM 或 CHR-ROM 区块出现在 CPU/PPU 地址空间中。

- Mapper 响应 CPU 对某些地址（通常 $8000~$FFFF 或 $4020~$5FFF）的写操作；
- 内部寄存器决定 PRG/CHR bank 的切换；
- 通过这种机制，NES 可以突破 32KB 限制。

**示例：**

```
void CPU::write(uint16_t addr, uint8_t data) {
    if (addr >= 0x8000)
        mapper->writeRegister(addr, data);
}
```

------

### 特别说明：地址空间与 DMA / 中断

- **DMA**：当写入 $4014 时，会触发 256 字节从 CPU RAM → PPU OAM 的 DMA。

- **NMI / IRQ 向量**：最后 6 个字节（$FFFA–$FFFF）存放中断向量：

  | 地址        | 向量         | 说明                 |
  | ----------- | ------------ | -------------------- |
  | $FFFA/$FFFB | NMI 向量     | 垂直回扫中断（PPU）  |
  | $FFFC/$FFFD | Reset 向量   | 上电或重置时入口地址 |
  | $FFFE/$FFFF | IRQ/BRK 向量 | APU 或 Mapper 触发   |

# 5.ROM数据布局

本章节介绍NES ROM文件的一些细节—— 主要是**PRG-ROM / CHR-ROM / Mapper 寄存器** 等内容NES 模拟器加载 ROM 文件时，需要首先解析其文件头（header），再根据其中的信息构建地址空间映射。标准 NES ROM 文件采用 **iNES 格式**（由 NESticle 模拟器最早定义）。
 文件整体结构如下：

```
+-----------------+  Header (16 bytes)
|  iNES Header    |
+-----------------+  Trainer (optional, 512 bytes)
|   Trainer Data  |
+-----------------+  PRG-ROM 数据区
|   PRG ROM Banks |
+-----------------+  CHR-ROM 数据区
|   CHR ROM Banks |
+-----------------+  Misc / Mapper / PlayChoice
|   Misc. Data    |
+-----------------+
```

------

## 5.1 Header 格式详解（共 16 字节）

| 偏移  | 长度 | 名称         | 说明                                     |
| ----- | ---- | ------------ | ---------------------------------------- |
| 0–3   | 4    | Signature    | 固定为 `"NES"` + `0x1A`                  |
| 4     | 1    | PRG-ROM 大小 | 以 16KB 为单位                           |
| 5     | 1    | CHR-ROM 大小 | 以 8KB 为单位（若为 0 表示使用 CHR-RAM） |
| 6     | 1    | Flag 6       | Mapper 低4位 + 镜像/电池/Trainer 标志    |
| 7     | 1    | Flag 7       | Mapper 高4位 + NES 2.0 标识              |
| 8     | 1    | PRG-RAM 大小 | 以 8KB 为单位（若为 0，则视为 8KB）      |
| 9     | 1    | TV 系统      | PAL/NTSC 标识（部分旧版无效）            |
| 10–15 | 6    | 保留         | NES 2.0 可能用于扩展字段                 |

### 1. Signature（魔数）

- 固定为 `"N" (0x4E)`, `"E" (0x45)`, `"S" (0x53)`, `0x1A`。
- 用于验证文件是否为 iNES 格式。

### 2. PRG-ROM 大小

- 单位：16KB；
- CPU 看到的 $8000~$FFFF 区域由这些 ROM 块映射组成；
- 例如：
  - 值 = 1 → 16KB；
  - 值 = 2 → 32KB；
  - 值 = 8 → 128KB。

### 3. CHR-ROM 大小

- 单位：8KB；
- PPU 的图像数据区域；
- 若为 0，则表示使用 **CHR-RAM**（即图案数据在运行时可写入）。

### 4. Flag 6（控制位 1）

| 位      | 含义                                            |
| ------- | ----------------------------------------------- |
| bit 0   | 镜像类型（0=水平，1=垂直）                      |
| bit 1   | 1 表示有电池供电的 WRAM（保存进度）             |
| bit 2   | 1 表示存在 Trainer（在 PRG-ROM 之前的 512字节） |
| bit 3   | 1 表示四屏镜像（忽略 bit0）                     |
| bit 4–7 | Mapper 编号低 4 位                              |

### 5. Flag 7（控制位 2）

| 位      | 含义                        |
| ------- | --------------------------- |
| bit 0–1 | 通常为 0（旧版）            |
| bit 2   | 是否支持 PlayChoice-10 硬件 |
| bit 3   | 是否支持 VS Unisystem 硬件  |
| bit 4–7 | Mapper 编号高 4 位          |

> Mapper ID = (Flag7 高4位 << 4) | (Flag6 高4位)

### 6. PRG-RAM 大小

- 单位：8KB；
- 若值为 0，通常仍假定有 8KB WRAM；
- 实际上由 Mapper 控制是否启用。

### 7. 其他字段

- 旧版中大多无效；
- NES 2.0 扩展格式中，部分字段重新定义为更精确的 ROM/RAM 大小描述。

------

## 5.2 ROM 数据区布局

解析完 header 后，文件余下部分即是游戏的实际数据：

| 区域    | 起始             | 大小     | 说明                           |
| ------- | ---------------- | -------- | ------------------------------ |
| Trainer | 可选（16字节后） | 512 字节 | 初始化时写入 $7000~$71FF       |
| PRG-ROM | 之后             | 16KB × N | CPU 代码与数据                 |
| CHR-ROM | 之后             | 8KB × N  | PPU 图案数据（角色/背景 Tile） |

### PRG-ROM 区（CPU使用）

- 对应 CPU 地址空间中的 **$8000–$FFFF**；
- 模拟器加载时，通常：
  - 若 PRG-ROM 只有 16KB：$C000–$FFFF 为其镜像；
  - 若为 32KB：直接映射两块；
  - 若 Mapper 存在，则由 Mapper 控制银行切换。

### CHR-ROM 区（PPU使用）

- 对应 PPU 地址空间的 **$0000–$1FFF**；
- 存储图形 Tile 数据（8×8 像素图块）；
- 每个 Tile 由 16 字节构成：
  - 前 8 字节：低平面（bit0）；
  - 后 8 字节：高平面（bit1）；
  - 组合后得到每像素 2bit（0~3）的调色索引。

> 若 CHR-ROM 大小为 0，则需在 PPU 内部分配 8KB CHR-RAM，并由 CPU 程序动态写入图案。

------

## 5.3 总结

### ROM 与 CPU 地址空间的映射关系

| ROM 区域          | 被映射到的 CPU/PPU 地址空间 | 控制逻辑       |
| ----------------- | --------------------------- | -------------- |
| PRG-ROM           | $8000–$FFFF                 | 由 Mapper 控制 |
| CHR-ROM / CHR-RAM | PPU $0000–$1FFF             | 由 Mapper 控制 |
| WRAM（如有）      | CPU $6000–$7FFF             | 可电池备份     |
| Trainer（如有）   | 加载时写入 CPU RAM 特定区域 | 初始化用途     |

------

### 常见 Mapper 与 ROM 区关系

| Mapper ID | 名称  | 特点                                |
| --------- | ----- | ----------------------------------- |
| 0         | NROM  | 无银行切换，固定 16KB/32KB ROM      |
| 1         | MMC1  | 可切换 PRG/CHR 银行，支持 WRAM      |
| 2         | UxROM | 切换 16KB PRG bank                  |
| 3         | CNROM | 切换 CHR bank                       |
| 4         | MMC3  | 最常见，支持 PRG+CHR 动态切换和 IRQ |
| 7         | AOROM | 32KB PRG bank 切换，单屏镜像控制    |

------

### 图示总结：ROM 文件 → 模拟器映射流程

```
ROM 文件结构 (.nes)
 ├── Header (16B)
 │    ├─ PRG-ROM大小：2 × 16KB = 32KB
 │    ├─ CHR-ROM大小：1 × 8KB = 8KB
 │    ├─ Mapper ID：4 (MMC3)
 │    └─ 镜像模式：垂直
 ├── PRG-ROM 数据区（32KB） → 映射到 CPU $8000~$FFFF
 ├── CHR-ROM 数据区（8KB） → 映射到 PPU $0000~$1FFF
 └── WRAM（卡带） → CPU $6000~$7FFF（可选）
```

