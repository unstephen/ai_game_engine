# 🤝 游戏引擎开发 - 协作指南

**欢迎加入自研 3D 游戏引擎项目！**

本指南帮助你快速上手，开始贡献代码。

---

## 📋 目录

1. [快速开始](#快速开始)
2. [开发环境配置](#开发环境配置)
3. [任务认领流程](#任务认领流程)
4. [代码规范](#代码规范)
5. [提交规范](#提交规范)
6. [沟通渠道](#沟通渠道)

---

## 🚀 快速开始

### 第一步：加入项目

1. **联系无为** - 获取 GitHub 仓库邀请
2. **加入飞书群** - "游戏引擎开发组"
3. **阅读文档** - 先看 `README.md` 和 `BUILD.md`

### 第二步：配置环境

```bash
# 1. 创建 GitHub Personal Access Token
# 访问：https://github.com/settings/tokens
# 权限：repo + workflow

# 2. 克隆仓库
git clone https://ghp_YOUR_TOKEN@github.com/unstephen/ai_game_engine.git
cd ai_game_engine

# 3. 配置 Git 身份
git config user.name "你的名字"
git config user.email "your@email.com"

# 4. 安装依赖
# Windows: 安装 Visual Studio 2022 + CMake
# Linux: sudo apt install cmake clang cppcheck
```

### 第三步：编译项目

```bash
# Windows
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release

# Linux
cmake -B build
cmake --build build
```

### 第四步：认领任务

1. 查看 `Docs/TASK_BOARD.md`
2. 选择 [待分配] 的任务
3. 创建 GitHub Issue 认领
4. 开始开发！

---

## 💻 开发环境配置

### Windows

| 软件 | 版本 | 下载链接 |
|------|------|----------|
| Visual Studio | 2022 | https://visualstudio.microsoft.com/ |
| CMake | 3.20+ | https://cmake.org/download/ |
| Git | 最新 | https://git-scm.com/ |

**安装步骤**：
1. 安装 VS 2022，勾选"使用 C++ 的桌面开发"
2. 安装 CMake，添加到 PATH
3. 安装 Git，配置用户信息

### Linux (Ubuntu/Debian)

```bash
sudo apt update
sudo apt install -y cmake clang clang-format cppcheck git
```

### macOS

```bash
brew install cmake llvm clang-format cppcheck
```

---

## 📝 任务认领流程

### 流程图

```
查看任务看板 → 选择任务 → 创建 Issue → 开发 → 提交 PR → 审查 → 合并
```

### 详细步骤

#### 1. 查看任务看板
- 位置：`Docs/TASK_BOARD.md`
- 选择标记为 `[待分配]` 的任务
- 注意任务难度和预计时间

#### 2. 创建 Issue
标题格式：`[任务认领] 任务 ID - 任务名称`

示例：
```
标题：[任务认领] RHI-001 - 虚拟纹理调研

内容：
- 任务 ID: RHI-001
- 负责人：@yourname
- 预计完成：2026-03-25
- 产出形式：调研报告 + 实现方案
```

#### 3. 开发
```bash
# 创建分支
git checkout -b feature/RHI-001-virtual-texture

# 开发...
# 提交代码
git add .
git commit -m "feat: 添加虚拟纹理调研文档"

# 推送
git push origin feature/RHI-001-virtual-texture
```

#### 4. 提交 PR
- 标题：`[RHI-001] 虚拟纹理调研报告`
- 描述：说明完成内容、测试结果
- 关联 Issue：`Closes #123`

#### 5. 代码审查
- 无为会审查代码
- 可能需要修改
- 通过后合并到 dev 分支

---

## 📐 代码规范

### C++ 代码风格

```cpp
// 类名：大驼峰
class TexturePool {
public:
    // 函数：大驼峰
    void AllocateTexture();
    
private:
    // 成员变量：m_ 前缀
    uint32_t m_width;
    uint32_t m_height;
};

// 命名空间：小写
namespace engine::rhi {

// 枚举：大驼峰
enum class ResourceType {
    Texture,
    Buffer,
};

// 变量：小写 + 下划线
uint32_t texture_count = 0;

// 常量：全大写
constexpr uint32_t MAX_TEXTURES = 1024;

} // namespace engine::rhi
```

### 注释规范

```cpp
/// @brief 函数功能说明
/// @param param1 参数说明
/// @return 返回值说明
void FunctionName(int param1);
```

### 文件格式

- 使用 UTF-8 编码
- Unix 换行符 (LF)
- 缩进：4 个空格
- 行尾无空白字符

---

## 📦 提交规范

### Commit Message 格式

```
<type>: <subject>

<body>

<footer>
```

### Type 类型

| 类型 | 说明 |
|------|------|
| `feat` | 新功能 |
| `fix` | Bug 修复 |
| `docs` | 文档更新 |
| `style` | 代码格式（不影响功能） |
| `refactor` | 重构 |
| `perf` | 性能优化 |
| `test` | 测试 |
| `chore` | 构建/工具配置 |

### 示例

```bash
# 新功能
git commit -m "feat: 实现纹理池基础版本"

# Bug 修复
git commit -m "fix: 修复 Windows 头文件包含顺序问题"

# 文档更新
git commit -m "docs: 添加协作指南"

# 重构
git commit -m "refactor: 优化内存分配器性能"
```

### 详细提交（推荐）

```bash
git commit -m "feat: 实现虚拟纹理基础版本

- 添加 VirtualTexture 类
- 实现页面调度算法
- 添加单元测试

Closes #123"
```

---

## 📞 沟通渠道

### 飞书群

- **群名**: 游戏引擎开发组
- **用途**: 日常沟通、问题讨论、进度同步
- **机器人**: 每日站会自动汇报

### GitHub

- **Issues**: 任务讨论、Bug 报告
- **PR**: 代码审查、合并请求
- **Discussions**: 技术方案讨论

### 每日站会

**时间**: 工作日 10:00 自动发送

**格式**:
```
🐉 引擎开发日报 - 2026-03-18

【你的名字】
✅ 已完成：xxx
🔄 进行中：xxx
⏸️ 阻塞：xxx（需要帮助）
```

---

## 📚 学习资源

### 必读文档

- `README.md` - 项目介绍
- `BUILD.md` - 编译指南
- `Docs/Architecture/` - 架构文档
- `Docs/TASK_BOARD.md` - 任务看板

### 推荐资料

- **GDC**: https://gdcvault.com/
- **SIGGRAPH**: https://s2026.siggraph.org/
- **Unreal Engine**: https://dev.epicgames.com/
- **LearnOpenGL**: https://learnopengl.com/

### 技术博客

- The Real Phenix
- Jeremie Selber
- Colin Barré-Brisebois

---

## ❓ 常见问题

### Q: 如何开始第一个任务？
A: 从 `⭐` 或 `⭐⭐` 难度的任务开始，比如文档或简单功能。

### Q: 遇到问题怎么办？
A: 
1. 先查文档和 Issues
2. 飞书群提问
3. 创建 Issue 详细描述问题

### Q: 代码提交后多久能合并？
A: 一般 1-3 天，无为会审查。

### Q: 可以同时做多个任务吗？
A: 建议一次专注 1 个任务，完成后再认领新的。

---

## 🎯 成长路径

```
新手 → 熟悉项目 → 独立开发 → 核心贡献者 → 架构师
  │        │          │          │          │
  1 周     2-4 周     1-3 月     3-6 月     6 月+
```

### 新手阶段（第 1 周）
- ✅ 配置开发环境
- ✅ 成功编译项目
- ✅ 完成第一个小任务

### 独立开发（1-3 月）
- ✅ 理解项目架构
- ✅ 独立完成模块开发
- ✅ 参与技术讨论

### 核心贡献者（3-6 月）
- ✅ 负责核心模块
- ✅ 代码审查
- ✅ 技术方案设计

---

## 📅 更新记录

| 日期 | 更新内容 | 更新人 |
|------|----------|--------|
| 2026-03-18 | 初始版本 | 无为 |

---

**欢迎加入！有任何问题随时联系！** 🚀
