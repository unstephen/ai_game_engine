# 分支工作流

## 分支说明

| 分支 | 用途 | 保护 |
|------|------|------|
| `main` | 稳定版本，可发布的代码 | ✅ 需要 CI 通过 |
| `dev` | 开发分支，日常开发在这里 | 自动 CI 检查 |

---

## 开发流程

### 1️⃣ 日常开发

```bash
# 切换到 dev 分支
git checkout dev

# 拉取最新代码
git pull

# 编写代码...

# 提交（会自动运行 pre-commit 检查）
git add .
git commit -m "feat: 添加新功能"

# 推送到 dev（触发 GitHub Actions）
git push
```

### 2️⃣ 合并到 main

当 dev 分支稳定后：

**方式一：Pull Request（推荐）**
1. 打开 https://github.com/unstephen/ai_game_engine/pull/new/dev
2. 创建 PR：`dev` → `main`
3. 等待 CI 检查通过
4. 合并 PR

**方式二：命令行**
```bash
git checkout main
git pull
git merge dev
git push
```

---

## 分支保护设置（推荐）

在 GitHub 设置分支保护，防止直接 push 到 main：

1. 打开 https://github.com/unstephen/ai_game_engine/settings/branches
2. 点击 **Add branch protection rule**
3. Branch name pattern: `main`
4. 勾选：
   - ✅ Require a pull request before merging
   - ✅ Require status checks to pass before merging
     - 选择 `lint-linux` 和 `build-windows`
5. 点击 **Create**

---

## 工作流图

```
                    ┌─────────────┐
                    │   dev 分支   │
                    │  (日常开发)  │
                    └──────┬──────┘
                           │
                    git push
                           │
                           ▼
                    ┌─────────────┐
                    │ GitHub CI   │
                    │ - cppcheck  │
                    │ - 编译 exe  │
                    └──────┬──────┘
                           │
              ┌────────────┴────────────┐
              │                         │
           ✅ 通过                    ❌ 失败
              │                         │
              ▼                         ▼
       ┌─────────────┐           ┌─────────────┐
       │ 合并到 main │           │ 修复代码    │
       │ (PR/merge)  │           │ 重新 push   │
       └─────────────┘           └─────────────┘
              │
              ▼
       ┌─────────────┐
       │ 下载 exe    │
       │ (Artifacts) │
       └─────────────┘
```

---

## 快捷命令

```bash
# 查看当前分支
git branch

# 切换分支
git checkout dev     # 切换到 dev
git checkout main    # 切换到 main

# 同步远程分支
git fetch --all

# 查看状态
git status
```