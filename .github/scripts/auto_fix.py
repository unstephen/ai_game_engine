#!/usr/bin/env python3
"""
自动修复常见 CI 错误并创建 PR
"""

import argparse
import json
import os
import subprocess
import sys
import urllib.request
from urllib.error import HTTPError

GITHUB_API = "https://api.github.com"

def fix_math_functions():
    """修复数学函数问题"""
    print("🔧 修复数学函数问题...")
    
    core_h = "Source/Runtime/Core/Public/Core.h"
    if not os.path.exists(core_h):
        print(f"  ⚠️ 文件不存在：{core_h}")
        return False
    
    with open(core_h, 'r') as f:
        content = f.read()
    
    # 检查是否已经修复
    if "#include <math.h>" in content and "#include <cmath>" in content:
        print("  ✅ 数学函数问题已修复")
        return True
    
    # 添加 math.h 包含
    if "#include <cmath>" in content:
        content = content.replace(
            "#include <cmath>",
            "#include <cmath>\n#include <math.h>"
        )
    
    with open(core_h, 'w') as f:
        f.write(content)
    
    print("  ✅ 已修复数学函数问题")
    return True

def fix_windows_header_order():
    """修复 Windows 头文件顺序"""
    print("🔧 修复 Windows 头文件顺序...")
    
    core_h = "Source/Runtime/Core/Public/Core.h"
    if not os.path.exists(core_h):
        return False
    
    with open(core_h, 'r') as f:
        content = f.read()
    
    # 检查 WIN32_LEAN_AND_MEAN 是否在 Windows.h 之前
    if "WIN32_LEAN_AND_MEAN" in content and "#include <Windows.h>" in content:
        win_pos = content.find("#include <Windows.h>")
        lean_pos = content.find("WIN32_LEAN_AND_MEAN")
        
        if lean_pos > win_pos:
            print("  ⚠️ 需要修复 Windows.h 顺序")
            # 这里简化处理，实际需要更复杂的逻辑
            return False
    
    print("  ✅ Windows 头文件顺序正确")
    return True

def create_fix_pr(fix_types, token=None):
    """创建修复 PR"""
    print("🤖 创建自动修复 PR...")
    
    # 切换到修复分支
    branch_name = "auto-fix/ci-failure"
    
    try:
        # 创建并切换分支
        subprocess.run(["git", "checkout", "-b", branch_name], check=True)
        
        # 应用修复
        fixes_applied = []
        
        if "math_function" in fix_types:
            if fix_math_functions():
                fixes_applied.append("数学函数")
        
        if "windows_header_order" in fix_types:
            if fix_windows_header_order():
                fixes_applied.append("Windows 头文件顺序")
        
        if not fixes_applied:
            print("  ⚠️ 没有应用任何修复")
            return False
        
        # 提交更改
        subprocess.run(["git", "add", "-A"], check=True)
        subprocess.run(["git", "commit", "-m", f"chore: 自动修复 CI 错误 ({', '.join(fixes_applied)})"], check=True)
        
        # 推送到远程
        subprocess.run(["git", "push", "origin", branch_name], check=True)
        
        # 创建 PR
        headers = {
            "Accept": "application/vnd.github.v3+json",
            "Content-Type": "application/json"
        }
        if token:
            headers["Authorization"] = f"token {token}"
        
        pr_data = {
            "title": f"🤖 [Auto-Fix] 修复 CI 编译错误",
            "body": f"""# 自动修复 PR

**触发原因**: CI 编译失败自动修复

**修复内容**:
{chr(10).join([f'- ✅ {fix}' for fix in fixes_applied])}

**自动修复工具**: .github/scripts/auto_fix.py

---

*此 PR 由 CI 失败自动修复系统创建*

**注意**: 请人工审查修复内容后再合并！
""",
            "head": branch_name,
            "base": "dev"
        }
        
        url = f"{GITHUB_API}/repos/unstephen/ai_game_engine/pulls"
        req = urllib.request.Request(
            url,
            data=json.dumps(pr_data).encode(),
            headers=headers
        )
        
        with urllib.request.urlopen(req) as resp:
            result = json.loads(resp.read().decode())
            print(f"✅ PR 创建成功：{result['html_url']}")
            return True
            
    except subprocess.CalledProcessError as e:
        print(f"❌ Git 操作失败：{e}")
        return False
    except HTTPError as e:
        print(f"❌ 创建 PR 失败：{e}")
        return False

def main():
    parser = argparse.ArgumentParser(description='自动修复 CI 错误')
    parser.add_argument('--fix-types', required=True, help='修复类型列表 (逗号分隔)')
    parser.add_argument('--token', default=os.environ.get('GITHUB_TOKEN'),
                       help='GitHub Token')
    args = parser.parse_args()
    
    fix_types = args.fix_types.split(',')
    print(f"🔧 开始自动修复：{', '.join(fix_types)}")
    
    # 配置 Git 用户
    subprocess.run(["git", "config", "user.name", "OpenClaw Auto-Fix Bot"])
    subprocess.run(["git", "config", "user.email", "autofix@bot.local"])
    
    # 创建 PR
    success = create_fix_pr(fix_types, args.token)
    
    sys.exit(0 if success else 1)

if __name__ == '__main__':
    main()
