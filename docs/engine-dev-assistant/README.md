# 引擎开发助手 - 快速启动指南

## 🚀 5 分钟快速配置

### 1. 确认文件结构
```bash
ls -la /root/.openclaw/workspace/agents/engine-dev-assistant/
# 应该看到:
# - AGENT.md      (角色定义)
# - SKILLS.md     (技能配置)
# - WORKFLOWS.md  (工作流示例)
```

### 2. 配置 OpenClaw
编辑 `~/.openclaw/openclaw.json`:
```json
{
  "agents": {
    "engine-dev-assistant": {
      "enabled": true,
      "model": "bailian/qwen3.5-plus",
      "context": {
        "project": "ai_game_engine",
        "workspace": "/root/.openclaw/workspace/engine"
      }
    }
  }
}
```

### 3. 重启 OpenClaw
```bash
openclaw gateway restart
```

### 4. 测试智能体
```
@引擎开发助手 你好，能帮我看看今天的 CI 状态吗？
```

---

## 📋 常用命令

### 启动/停止
```bash
# 启动智能体
openclaw agent start engine-dev-assistant

# 查看状态
openclaw agent status engine-dev-assistant

# 查看日志
openclaw agent logs engine-dev-assistant --tail 50
```

### 配置管理
```bash
# 查看配置
openclaw config get agents.engine-dev-assistant

# 更新配置
openclaw config set agents.engine-dev-assistant.enabled=true

# 重载配置
openclaw gateway restart
```

---

## 💬 使用示例

### 1. 代码审查
```
@引擎开发助手 请审查这个 PR：
https://github.com/unstephen/ai_game_engine/pull/12
```

**预期响应**:
```markdown
## 📋 代码审查报告

### ✅ 优点
- 代码结构清晰
- 注释完整

### ⚠️ 改进建议
- 第 45 行：建议使用智能指针
- 第 78 行：缺少错误处理

### 📝 具体建议
```cpp
// 建议修改
std::unique_ptr<D3D12Device> device;
```
```

---

### 2. 编译错误诊断
```
@引擎开发助手 CI 编译失败了，帮我看看
```

**预期响应**:
```markdown
## 🔍 错误分析

**Run**: #25
**错误类型**: MSVC 编译错误
**位置**: `Source/Runtime/Core/Public/Core.h:28`

### 错误日志
```
fatal error C1083: cannot open include file: 'windows.h'
```

### 修复方案
确保 `<windows.h>` 在标准库头文件之后包含。
```

---

### 3. 技术调研
```
@引擎开发助手 调研一下 UE5 的虚拟纹理实现
```

**预期响应**:
```markdown
## 📊 虚拟纹理技术调研

### UE5 实现概述
- 页面粒度：128x128 texels
- 流式加载：基于视线预测
- 内存管理：虚拟内存映射

### 关键论文
- [GDC 2022] Virtual Texturing in Unreal Engine 5
- [SIGGRAPH 2021] Nanite: Virtualized Geometry

### 实现建议
1. 先实现基础页面映射表
2. 添加流式加载
3. 优化内存管理
```

---

### 4. 代码生成
```
@引擎开发助手 帮我实现一个 D3D12 描述符堆管理器
```

**预期响应**:
```markdown
## 📝 D3D12 描述符堆管理器

### 设计思路
1. 管理多个描述符堆
2. 自动分配描述符句柄
3. 支持动态扩展

### 代码实现
```cpp
class DescriptorHeapManager {
public:
    D3D12_CPU_DESCRIPTOR_HANDLE Allocate();
    void Free(D3D12_CPU_DESCRIPTOR_HANDLE handle);
    
private:
    std::vector<ID3D12DescriptorHeap*> m_heaps;
    std::vector<bool> m_freeList;
};
```

### 使用示例
```cpp
auto handle = heapManager.Allocate();
// 使用描述符
heapManager.Free(handle);
```
```

---

## 🔧 故障排查

### 智能体无响应
```bash
# 检查 OpenClaw 状态
openclaw gateway status

# 查看日志
openclaw agent logs engine-dev-assistant --tail 100

# 重启网关
openclaw gateway restart
```

### 技能未加载
```bash
# 检查技能文件
ls -la ~/.openclaw/skills/game-engine-research/

# 重新加载技能
openclaw skills reload
```

### 配置错误
```bash
# 验证配置
openclaw config validate

# 查看配置
openclaw config get agents
```

---

## 📚 进阶配置

### 自定义技能
1. 创建技能目录：`~/.openclaw/skills/my-skill/`
2. 编写 `SKILL.md`
3. 在 `SKILLS.md` 中注册
4. 测试技能触发

### 自定义工作流
1. 复制 `WORKFLOWS.md` 中的示例
2. 修改触发条件
3. 添加到 `.github/workflows/`
4. 测试工作流

### 多智能体协作
```json
{
  "agents": {
    "engine-dev-assistant": {
      "enabled": true,
      "collaborate_with": ["ci-bot", "review-bot"]
    }
  }
}
```

---

## 🎯 最佳实践

### 1. 明确任务描述
❌ "帮我看看代码"
✅ "请审查 Source/Runtime/RHI/Private/Device.cpp 的内存管理部分"

### 2. 提供上下文
❌ "编译失败了"
✅ "Windows CI Run #25 编译失败，错误在 Core.h"

### 3. 指定输出格式
❌ "调研虚拟纹理"
✅ "调研虚拟纹理，输出 Markdown 报告，包含代码示例"

### 4. 及时反馈
- ✅ 有用的响应：点赞/收藏
- ❌ 错误的响应：指出问题
- 📝 改进建议：提出具体需求

---

## 📞 支持

### 文档
- AGENT.md - 角色定义
- SKILLS.md - 技能配置
- WORKFLOWS.md - 工作流示例

### 日志
```bash
# 查看智能体日志
openclaw agent logs engine-dev-assistant

# 查看网关日志
openclaw gateway logs

# 实时日志
openclaw gateway logs --follow
```

### 社区
- GitHub Issues: https://github.com/unstephen/ai_game_engine/issues
- 飞书群：游戏引擎开发组

---

*最后更新：2026-03-18*
*维护者：无为*
