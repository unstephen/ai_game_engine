# 引擎开发助手 - 工作流示例

## CI 失败自动处理

### 流程图
```
CI 失败
    ↓
下载编译日志 (build-output.log)
    ↓
分析错误类型
    ↓
创建 GitHub Issue
    ↓
附带错误日志和修复建议
    ↓
@无为 查看 Issue
```

### 工作流配置
```yaml
name: Create Issue on Failure

on:
  workflow_run:
    workflows: ["CI"]
    types: [completed]

permissions:
  contents: read
  issues: write
  actions: read

jobs:
  create-issue:
    if: ${{ github.event.workflow_run.conclusion == 'failure' }}
    runs-on: ubuntu-latest
    
    steps:
    - name: Download build log
      uses: actions/download-artifact@v4
      with:
        name: build-error-log
        github-token: ${{ secrets.GITHUB_TOKEN }}
    
    - name: Create Issue with Error Log
      uses: actions/github-script@v7
      with:
        script: |
          const errorLog = fs.readFileSync('build_output.log', 'utf8');
          await github.rest.issues.create({
            title: `[CI 失败] Windows 编译错误`,
            body: `## 编译错误\n\n\`\`\`\n${errorLog}\n\`\`\``
          });
```

---

## 代码审查自动化

### PR 审查流程
```
PR 创建
    ↓
自动运行 clang-format
    ↓
自动运行 cppcheck
    ↓
生成审查报告
    ↓
@引擎开发助手 评论
```

### 审查配置
```yaml
name: Code Review

on:
  pull_request:
    branches: [ dev, main ]

jobs:
  review:
    runs-on: ubuntu-latest
    
    steps:
    - uses: actions/checkout@v4
    
    - name: Run clang-format
      run: |
        find Source -name '*.cpp' -o -name '*.h' | xargs clang-format -i
        git diff --exit-code
    
    - name: Run cppcheck
      run: |
        cppcheck --enable=warning,performance,portability Source/
    
    - name: Create Review Comment
      uses: actions/github-script@v7
      with:
        script: |
          // 生成审查报告
```

---

## 每日站会汇报

### 飞书机器人配置
```yaml
name: Daily Standup

on:
  schedule:
    - cron: '0 2 * * *'  # 每天 10:00 (Asia/Shanghai)

jobs:
  standup:
    runs-on: ubuntu-latest
    
    steps:
    - name: Send Standup to Feishu
      uses: actions/github-script@v7
      with:
        script: |
          // 获取今日 CI 状态
          // 获取今日 Commits
          // 获取待处理 PRs
          // 发送到飞书群
```

### 汇报模板
```markdown
🐉 引擎开发日报 - 2026-03-18

【今日 CI 状态】
✅ Linux 语法检查：通过
❌ Windows 编译：失败 (见 Issue #3)

【今日 Commits】
- fix: 修复 mingw 交叉编译问题
- feat: 添加 CI 自动分析功能

【待处理 PRs】
- #12: 虚拟纹理实现
- #15: D3D12 描述符堆优化

【待办事项】
- [ ] 修复 Windows 编译错误
- [ ] 审查虚拟纹理 PR
```

---

## 技术调研自动化

### 调研任务创建
```yaml
name: Research Task

on:
  issues:
    types: [labeled]

jobs:
  research:
    if: github.event.label.name == 'research-needed'
    runs-on: ubuntu-latest
    
    steps:
    - name: Search GDC/SIGGRAPH
      run: |
        # 搜索相关技术文章
        
    - name: Search GitHub
      run: |
        # 搜索开源实现
        
    - name: Create Research Report
      uses: actions/github-script@v7
      with:
        script: |
          // 生成调研报告
```

---

## 上下文管理

### 项目上下文
```json
{
  "project": "ai_game_engine",
  "version": "0.1.0",
  "branches": {
    "main": "稳定版本",
    "dev": "开发分支"
  },
  "ci_status": {
    "linux": "✅",
    "windows": "❌"
  },
  "active_issues": 3,
  "active_prs": 2
}
```

### 会话记忆
```markdown
## 当前会话上下文

**用户**: 无为
**任务**: 修复 Windows 编译错误
**相关文件**:
- Source/Runtime/Core/Public/Core.h
- .github/workflows/build.yml

**历史操作**:
1. 分析了编译日志
2. 发现 Windows.h 包含顺序问题
3. 提交了修复

**下一步**:
- 等待 CI 验证
- 如果通过，合并到 dev
```

---

## 错误处理

### 常见错误及处理

| 错误类型 | 处理方式 |
|----------|----------|
| CI 失败 | 自动创建 Issue + 日志 |
| 编译错误 | 分析错误类型 + 修复建议 |
| 代码格式 | 自动运行 clang-format |
| 静态分析 | 运行 cppcheck + 报告 |

### 升级机制
```
一级：自动修复 (clang-format)
二级：创建 Issue (编译错误)
三级：@无为 (架构决策)
```

---

*最后更新：2026-03-18*
