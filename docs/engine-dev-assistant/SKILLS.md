# 引擎开发助手 - 技能配置

## 已集成的技能

### 1. game-engine-research
**位置**: `~/.openclaw/skills/game-engine-research/SKILL.md`

**能力**:
- GDC/SIGGRAPH 技术文章搜索
- UE/Unity 架构分析
- 开源引擎代码调研
- 知乎技术文章整理

**触发词**:
- "调研 XXX 技术"
- "看看 UE 是怎么实现 XXX 的"
- "找一下 GDC 关于 XXX 的分享"

---

### 2. cpp-coding
**内置技能**

**能力**:
- C++ 代码编写
- 代码审查
- Bug 修复
- 性能优化

**触发词**:
- "帮我写一个 XXX 类"
- "审查一下这段代码"
- "这个编译错误怎么修"

---

### 3. cmake-build
**内置技能**

**能力**:
- CMakeLists.txt 配置
- 编译错误诊断
- 跨平台编译配置
- 依赖管理

**触发词**:
- "CMake 找不到 XXX 库"
- "帮我添加 XXX 编译选项"
- "Windows 编译失败"

---

### 4. ci-automation
**位置**: `.github/workflows/`

**能力**:
- CI 工作流维护
- 自动 Issue 创建
- 编译日志分析
-  artifact 管理

**触发词**:
- "CI 又失败了"
- "帮我看看编译日志"
- "自动创建 Issue"

---

## 技能调用示例

### 技术调研
```
用户：调研一下虚拟纹理技术
助手：[调用 game-engine-research 技能]
      1. 搜索 GDC 相关演讲
      2. 查找 UE5 实现文档
      3. 分析开源引擎代码
      4. 生成调研报告
```

### 代码审查
```
用户：审查这个 PR
助手：[调用 cpp-coding 技能]
      1. 运行 clang-format
      2. 运行 cppcheck
      3. 检查命名规范
      4. 提供改进建议
```

### 编译诊断
```
用户：CI 编译失败了
助手：[调用 ci-automation 技能]
      1. 下载编译日志
      2. 分析错误类型
      3. 创建 Issue
      4. 提供修复建议
```

---

## 技能扩展

### 添加新技能
1. 在 `~/.openclaw/skills/` 创建技能目录
2. 编写 `SKILL.md` 说明文件
3. 在 `SKILLS.md` 中注册
4. 测试技能触发

### 技能配置
```json
{
  "skill-name": {
    "enabled": true,
    "priority": 1,
    "triggers": ["关键词 1", "关键词 2"],
    "config": {
      "option1": "value1",
      "option2": "value2"
    }
  }
}
```

---

## 技能优先级

1. **ci-automation** (P0) - CI 相关自动处理
2. **cpp-coding** (P1) - 代码开发
3. **cmake-build** (P1) - 编译系统
4. **game-engine-research** (P2) - 技术调研

---

*最后更新：2026-03-18*
