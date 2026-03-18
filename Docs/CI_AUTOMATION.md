# 🤖 CI 自动化流程文档

## 概述

本项目实现了三层自动化 CI 检查流程，确保 Windows 编译稳定性，减少人工介入。

---

## 📊 工作流程

```
代码提交
    │
    ├─→ [快速检查 - 5 分钟]
    │   ├─→ mingw 交叉编译 (ubuntu)
    │   └─→ Linux 语法检查 (cppcheck)
    │
    └─→ [完整检查 - 10 分钟]
        └─→ Windows 真实编译 (windows-latest)
            │
            ├─→ ✅ 通过 → 完成
            │
            └─→ ❌ 失败 → 自动分析 → 创建修复
```

---

## 🔧 工作流文件

### 1. windows-precheck.yml
**用途**: mingw-w64 交叉编译预检查  
**触发**: push 到 dev 分支 / PR  
**运行时间**: ~5 分钟  
**目的**: 提前发现 Windows 编译问题，不用等 Windows 作业

### 2. build.yml (主 CI)
**用途**: 完整 CI 检查  
**触发**: push 到 main/dev / PR  
**运行时间**: ~10 分钟  

**作业依赖**:
```
mingw-check ──┐
              ├─→ build-windows
lint-linux  ──┘
```

### 3. auto-fix.yml
**用途**: CI 失败自动分析和修复  
**触发**: CI 工作流失败时  
**功能**:
- 下载编译日志
- 分析错误类型
- 创建 GitHub Issue
- 自动创建修复 PR（如果可能）

---

## 🤖 自动分析的错误类型

| 错误类型 | 自动检测 | 自动修复 | 说明 |
|----------|----------|----------|------|
| 数学函数未定义 | ✅ | ✅ | `acosf`, `std::sqrt` 等 |
| Windows 头文件顺序 | ✅ | ✅ | `WIN32_LEAN_AND_MEAN` 位置 |
| 缺少头文件 | ✅ | ❌ | 需要人工添加 |
| 链接库缺失 | ✅ | ❌ | 需要人工配置 CMake |
| d3dx12.h 缺失 | ✅ | ❌ | 需要本地定义辅助结构 |

---

## 📝 脚本说明

### analyze_failure.py
**位置**: `.github/scripts/analyze_failure.py`

**功能**:
1. 获取失败作业的详细日志
2. 匹配已知错误模式
3. 生成修复建议
4. 创建 GitHub Issue

**运行方式**:
```bash
python3 .github/scripts/analyze_failure.py \
  --run-id <workflow_run_id> \
  --token $GITHUB_TOKEN
```

### auto_fix.py
**位置**: `.github/scripts/auto_fix.py`

**功能**:
1. 应用自动修复（头文件顺序、数学函数等）
2. 创建修复分支
3. 提交并推送
4. 创建 Pull Request

**运行方式**:
```bash
python3 .github/scripts/auto_fix.py \
  --fix-types "math_function,windows_header_order" \
  --token $GITHUB_TOKEN
```

---

## 🎯 使用场景

### 场景 1: 小问题（头文件顺序）
```
提交代码
  ↓
mingw 检查失败
  ↓
自动分析 → 检测到头文件顺序问题
  ↓
自动创建修复 PR
  ↓
人工审查 → 合并
```
**人工介入**: 只需审查 PR

### 场景 2: 复杂问题（缺少库）
```
提交代码
  ↓
Windows 编译失败
  ↓
自动分析 → 检测到链接库缺失
  ↓
创建 Issue（附修复建议）
  ↓
人工修复 → 提交
```
**人工介入**: 根据 Issue 建议修复

### 场景 3: 未知问题
```
提交代码
  ↓
CI 失败
  ↓
自动分析 → 未匹配已知模式
  ↓
创建 Issue（附错误日志）
  ↓
人工分析 → 修复 → 更新脚本
```
**人工介入**: 分析并修复，同时更新自动分析脚本

---

## ⚙️ 配置说明

### 需要的 Secrets
- `GITHUB_TOKEN`: GitHub 自动提供，无需手动配置

### 可选配置
在 `.github/scripts/analyze_failure.py` 中修改：
- `ERROR_PATTERNS`: 添加新的错误模式
- 修复建议和自动修复逻辑

---

## 📈 效果指标

| 指标 | 目标 | 当前 |
|------|------|------|
| 快速检查时间 | < 5 分钟 | ~3 分钟 |
| 完整检查时间 | < 15 分钟 | ~10 分钟 |
| 自动修复率 | > 50% | 待统计 |
| 人工介入次数 | 减少 70% | 待统计 |

---

## 🔮 未来改进

- [ ] 添加更多错误模式识别
- [ ] 支持更多自动修复场景
- [ ] 集成飞书通知
- [ ] 统计 CI 成功率趋势
- [ ] 自动回滚连续失败的提交

---

## 📞 故障排除

### Q: 自动修复 PR 创建失败？
A: 检查 `.github/scripts/auto_fix.py` 的日志，确认 Git 配置正确。

### Q: 分析脚本找不到错误？
A: 在 `ERROR_PATTERNS` 中添加新的错误模式。

### Q: mingw 检查和 MSVC 结果不一致？
A: 这是正常的，mingw 用于快速检查，最终以 MSVC 为准。

---

*文档最后更新：2026-03-18*
