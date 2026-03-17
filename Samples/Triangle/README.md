# 三角形渲染示例

自研 3D 游戏引擎的第一个渲染示例，演示完整的 D3D12 渲染管线。

---

## 效果预览

```
┌─────────────────────────────────────────┐
│   自研引擎 - 三角形示例                  │
├─────────────────────────────────────────┤
│                                         │
│              ● (红色)                   │
│             / \                         │
│            /   \                        │
│           /     \                       │
│          /       \                      │
│         /         \                     │
│        /           \                    │
│       ●─────────────●                   │
│    (蓝色)   (绿色)                      │
│                                         │
│   深蓝色背景                            │
│                                         │
└─────────────────────────────────────────┘
```

---

## 系统要求

- **操作系统**: Windows 10/11 (64-bit)
- **显卡**: 支持 DirectX 12 的 GPU
- **编译器**: Visual Studio 2022 或 CMake 3.20+
- **Windows SDK**: 10.0.19041.0 或更高

---

## 编译

### 使用 CMake

```bash
cd /path/to/engine
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64 -DENGINE_BUILD_SAMPLES=ON
cmake --build . --config Release
```

### 使用 Visual Studio

1. 打开 `engine/build/Engine.sln`
2. 选择 `Release` 配置
3. 右键 `Sample_Triangle` 项目 → 生成

---

## 运行

```bash
# 从 build 目录运行
./bin/Release/Sample_Triangle.exe

# 或从 Samples 目录运行
cd Samples/Triangle
./Triangle.exe
```

---

## 控制键

| 按键 | 功能 |
|------|------|
| ESC  | 退出应用 |

---

## 技术实现

### 核心模块

| 模块 | 说明 |
|------|------|
| D3D12Device | 设备初始化、命令队列 |
| D3D12SwapChain | 交换链、后备缓冲 |
| D3D12Buffer | 顶点缓冲区 |
| D3D12FrameResourceManager | 三缓冲帧资源管理 |
| D3D12PipelineState | 管线状态对象 |
| D3D12RootSignature | 根签名 |

### 渲染流程

```
初始化
  ├── 创建窗口
  ├── 初始化 D3D12 设备
  ├── 创建交换链
  ├── 创建帧资源管理器
  ├── 创建顶点缓冲区
  ├── 编译着色器
  ├── 创建根签名
  └── 创建管线状态

渲染循环
  ├── BeginFrame()
  │     └── 等待 GPU、重置分配器
  ├── 录制命令
  │     ├── 设置视口/裁剪
  │     ├── 资源屏障 (Present → RenderTarget)
  │     ├── 清除渲染目标
  │     ├── 设置管线状态
  │     ├── 绑定顶点缓冲
  │     ├── DrawInstanced(3)
  │     └── 资源屏障 (RenderTarget → Present)
  ├── 提交命令
  ├── Present()
  └── EndFrame()
        └── 信号围栏、切换帧
```

### 着色器

**顶点着色器** (shaders.hlsl):
```hlsl
VSOutput VSMain(VSInput input) {
    VSOutput output;
    output.position = float4(input.position, 1.0);
    output.color = input.color;
    return output;
}
```

**像素着色器**:
```hlsl
float4 PSMain(VSOutput input) : SV_TARGET {
    return float4(input.color, 1.0);
}
```

---

## 调试

### PIX for Windows

1. 安装 PIX (Windows Store)
2. 启动 PIX → Launch Windows Store app
3. 选择 Sample_Triangle.exe
4. 捕获帧进行分析

### Visual Studio Graphics Diagnostics

1. 调试 → 图形 → 启动图形调试
2. 按 Print Screen 捕获帧
3. 分析管线状态

---

## 已知问题

1. **窗口大小调整** - 尚未完整实现
2. **多显示器支持** - 未测试
3. **VSync 控制** - 当前固定开启

---

## 后续优化

- [ ] 添加 UI 显示 FPS
- [ ] 支持窗口大小调整
- [ ] 添加更多图元示例
- [ ] 集成 ImGui

---

*自研引擎三角形示例 - 龙景 2026-03-17* 🐉