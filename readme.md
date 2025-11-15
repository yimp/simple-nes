# simple-nes -- Minimal NES Emulator for Toturial

## 简介

这是一个**轻量简洁的 NES 模拟器**项目，与[《从0开始的FC模拟器》](https://space.bilibili.com/45386643)系列视频同步推进。本仓库旨在为对 NES 模拟器原理感兴趣的开发者，提供一份直观易懂、易于跟随的代码库 —— 核心聚焦 “清晰性” 与 “核心功能实现”，不堆砌冗余复杂特性。

## 快速构建

项目引用了部分第三方库，需要递归克隆。

```bash
git clone --recursive https://github.com/yimp/simple-nes.git
```

然后使用cmake配置出你所用的构建系统的构建脚本。
```bash
cd simple-nes
mkdir build
cd build
cmake -G "XXX" ..
```

构建

```bash
cmake --build .
```

## 文档

模拟器开发过程中的详细解析、实现指南及补充资料，均整理在 [doc.md](https://github.com/yimp/simple-nes/blob/main/doc/doc.md) 文件中，该文件与本 README 位于同一根目录下，可直接查看。

## 关于视频系列

系列视频会以 “分步讲解” 的形式，从基础概念到实际编码，带你掌握 NES 模拟器的核心开发流程。无论是想了解复古主机原理，还是想提升底层编程能力，都可以通过视频 + 代码的组合形式，更高效地理解知识点。

