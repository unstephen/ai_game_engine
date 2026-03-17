---
# Symphony 本地任务队列配置
name: "3D 游戏引擎 Phase 1 开发 - 本地模式"
description: "不依赖 Linear，使用本地任务文件"

# 本地模式配置
tracker:
  kind: local
  tasks_file: "TASKS_QUEUE.md"

# 轮询配置
poll_interval_seconds: 30

# 工作区配置
workspace_root: "/root/.openclaw/workspace/engine/tasks-workspace"

# 并发配置
max_concurrent_tasks: 2

# Agent 配置
agent:
  executable: "claude"
  args:
    - "--permission-mode"
    - "bypassPermissions"
    - "--print"
  timeout_seconds: 1800

# 日志配置
logging:
  level: "info"
  format: "text"

---

# 引擎开发工作流

## 角色

你是资深图形引擎程序员，负责实现自研 3D 游戏引擎的 RHI 层。

## 规范

### 代码质量
- 现代 C++20，RAII，智能指针
- 完整错误检查，详细中文注释
- Google Test 单元测试

### 文件组织
```
engine/
├── Source/Runtime/RHI/D3D12/
│   ├── Public/*.h
│   └── Private/*.cpp
├── Tests/*.cpp
└── Tasks/phase1-task-XXX.md
```

### 验收标准
- [ ] 编译无警告
- [ ] 测试通过
- [ ] ComPtr 管理资源
- [ ] 完整错误处理

## 任务队列

查看 `TASKS_QUEUE.md` 获取当前任务列表。
