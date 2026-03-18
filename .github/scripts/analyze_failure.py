#!/usr/bin/env python3
"""
CI 失败自动分析脚本
下载编译日志 → 分析错误类型 → 生成修复建议
"""

import argparse
import json
import os
import re
import sys
import urllib.request
from urllib.error import HTTPError

GITHUB_API = "https://api.github.com"

# 常见错误模式及修复建议
ERROR_PATTERNS = {
    "math_function": {
        "pattern": r"(acosf|asinf|atanf|sinf|cosf|tanf|sqrt|std::sqrt|std::cos).*is not a member",
        "severity": "high",
        "title": "MSVC 数学函数未定义",
        "solution": """## 问题分析
MSVC 下 `<cmath>` 的数学函数需要在 `std::` 命名空间中调用，或者需要正确的头文件包含顺序。

## 修复方案
1. 确保 `Core.h` 中 `<cmath>` 和 `<math.h>` 在 `<Windows.h>` 之前包含
2. 使用 `std::` 前缀调用数学函数（如 `std::sqrt()` 而非 `sqrt()`）

## 参考修复
```cpp
// Core.h 正确顺序
#include <cmath>
#include <math.h>
#include <Windows.h>  // 在最后
```""",
        "auto_fix": True
    },
    
    "windows_header_order": {
        "pattern": r"WIN32_LEAN_AND_MEAN.*after|Windows\.h.*before",
        "severity": "high",
        "title": "Windows 头文件包含顺序错误",
        "solution": """## 问题分析
`WIN32_LEAN_AND_MEAN` 和 `NOMINMAX` 必须在包含 `<Windows.h>` 之前定义。

## 修复方案
```cpp
// 正确做法
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
```""",
        "auto_fix": True
    },
    
    "missing_header": {
        "pattern": r"fatal error C1083:.*cannot open include file: '(.+?)'",
        "severity": "medium",
        "title": "缺少头文件",
        "solution": """## 问题分析
编译时找不到指定的头文件。

## 修复方案
1. 检查头文件路径是否正确
2. 确认 CMakeLists.txt 中包含了正确的 include 目录
3. 如果是第三方库，确认已正确安装""",
        "auto_fix": False
    },
    
    "missing_library": {
        "pattern": r"unresolved external symbol.*referenced in function",
        "severity": "medium",
        "title": "链接库缺失",
        "solution": """## 问题分析
链接时找不到符号，通常是缺少库文件。

## 修复方案
1. 在 CMakeLists.txt 中添加 `target_link_libraries`
2. 确认库文件路径正确
3. Windows 下可能需要 `#pragma comment(lib, "xxx.lib")`""",
        "auto_fix": False
    },
    
    "d3dx12_missing": {
        "pattern": r"d3dx12\.h.*No such file",
        "severity": "high",
        "title": "缺少 d3dx12.h 辅助头文件",
        "solution": """## 问题分析
`d3dx12.h` 不是 Windows SDK 的一部分，需要单独获取。

## 修复方案
1. 从 Microsoft 官方仓库获取：https://github.com/microsoft/DirectX-Headers
2. 或者在代码中本地定义需要的辅助结构（推荐）
3. 避免依赖此头文件""",
        "auto_fix": False
    },
    
    "pragma_comment": {
        "pattern": r"LNK1181.*cannot open input file.*\.lib",
        "severity": "medium",
        "title": "链接库文件缺失",
        "solution": """## 问题分析
`#pragma comment(lib, "xxx.lib")` 指定的库文件找不到。

## 修复方案
1. 在 CMakeLists.txt 中显式添加 `target_link_libraries(xxx d3d12 dxgi)`
2. 不要依赖 `#pragma comment`，CMake 可能不生效""",
        "auto_fix": False
    }
}

def get_workflow_logs(run_id, token=None):
    """获取工作流日志"""
    headers = {"Accept": "application/vnd.github.v3+json"}
    if token:
        headers["Authorization"] = f"token {token}"
    
    url = f"{GITHUB_API}/repos/unstephen/ai_game_engine/actions/runs/{run_id}/jobs"
    try:
        with urllib.request.urlopen(urllib.request.Request(url, headers=headers)) as resp:
            return json.loads(resp.read().decode())
    except HTTPError as e:
        print(f"Error fetching jobs: {e}")
        return None

def get_job_logs(job_id, token=None):
    """获取作业详细日志"""
    headers = {"Accept": "application/vnd.github.v3+json"}
    if token:
        headers["Authorization"] = f"token {token}"
    
    url = f"{GITHUB_API}/repos/unstephen/ai_game_engine/actions/jobs/{job_id}/logs"
    try:
        with urllib.request.urlopen(urllib.request.Request(url, headers=headers)) as resp:
            return resp.read().decode('utf-8', errors='ignore')
    except HTTPError as e:
        print(f"Error fetching logs: {e}")
        return None

def analyze_logs(logs):
    """分析日志，识别错误类型"""
    findings = []
    
    for error_type, config in ERROR_PATTERNS.items():
        matches = re.findall(config["pattern"], logs, re.IGNORECASE)
        if matches:
            findings.append({
                "type": error_type,
                "severity": config["severity"],
                "title": config["title"],
                "solution": config["solution"],
                "auto_fix": config.get("auto_fix", False),
                "matches": len(matches)
            })
    
    # 提取具体错误行
    error_lines = []
    for line in logs.split('\n'):
        if 'error' in line.lower() and ('C' in line or 'LNK' in line):
            error_lines.append(line.strip()[:200])
    
    return {
        "findings": findings,
        "error_lines": list(set(error_lines))[:10]  # 去重，最多 10 条
    }

def create_issue(analysis, run_id, token=None):
    """创建 GitHub Issue"""
    headers = {
        "Accept": "application/vnd.github.v3+json",
        "Content-Type": "application/json"
    }
    if token:
        headers["Authorization"] = f"token {token}"
    
    # 生成 Issue 内容
    title = f"[CI 失败] 自动分析 - Run #{run_id}"
    
    body = f"""# CI 失败自动分析报告

**Run ID**: #{run_id}
**分析时间**: {os.popen('date').read().strip()}

---

## 🔍 分析结果

"""
    
    for finding in analysis["findings"]:
        body += f"""### {finding['title']}
- **严重程度**: {finding['severity']}
- **出现次数**: {finding['matches']}
- **可自动修复**: {'✅ 是' if finding['auto_fix'] else '❌ 否'}

{finding['solution']}

---

"""
    
    if analysis["error_lines"]:
        body += """## 📋 错误日志摘要

```
"""
        body += '\n'.join(analysis["error_lines"])
        body += """
```

---

"""
    
    body += """## 🤖 下一步

1. **可自动修复的问题**: 系统将自动创建修复 PR
2. **需要人工干预的问题**: 请根据上述建议手动修复
3. **误报反馈**: 如果这是误报，请关闭此 Issue

---

*此 Issue 由 CI 失败自动分析系统创建*
"""
    
    data = {
        "title": title,
        "body": body,
        "labels": ["bug", "ci-failure", "auto-generated"]
    }
    
    url = f"{GITHUB_API}/repos/unstephen/ai_game_engine/issues"
    try:
        req = urllib.request.Request(
            url,
            data=json.dumps(data).encode(),
            headers=headers
        )
        with urllib.request.urlopen(req) as resp:
            result = json.loads(resp.read().decode())
            print(f"✅ Issue created: {result['html_url']}")
            return result
    except HTTPError as e:
        print(f"Error creating issue: {e}")
        return None

def main():
    parser = argparse.ArgumentParser(description='CI 失败自动分析')
    parser.add_argument('--run-id', required=True, help='Workflow Run ID')
    parser.add_argument('--token', default=os.environ.get('GITHUB_TOKEN'), 
                       help='GitHub Token')
    args = parser.parse_args()
    
    print(f"🔍 分析 Run #{args.run_id} 的失败原因...")
    
    # 获取作业列表
    jobs = get_workflow_logs(args.run_id, args.token)
    if not jobs:
        sys.exit(1)
    
    # 分析失败的作业
    all_findings = []
    all_errors = []
    
    for job in jobs.get('jobs', []):
        if job.get('conclusion') == 'failure':
            print(f"📊 分析作业：{job['name']}")
            logs = get_job_logs(job['id'], args.token)
            if logs:
                analysis = analyze_logs(logs)
                all_findings.extend(analysis['findings'])
                all_errors.extend(analysis['error_lines'])
    
    if not all_findings:
        print("⚠️ 未识别到已知错误模式，需要人工分析")
        all_findings.append({
            "type": "unknown",
            "severity": "medium",
            "title": "未知错误",
            "solution": "请手动检查编译日志",
            "auto_fix": False,
            "matches": 1
        })
    
    # 创建 Issue
    analysis = {"findings": all_findings, "error_lines": all_errors}
    issue = create_issue(analysis, args.run_id, args.token)
    
    # 输出摘要
    print("\n📋 分析摘要:")
    for finding in all_findings:
        print(f"  - {finding['title']} ({finding['severity']})")
    
    if issue:
        print(f"\n✅ 已创建 Issue: {issue['html_url']}")
    
    # 如果有可自动修复的问题，触发修复流程
    auto_fixable = [f for f in all_findings if f.get('auto_fix')]
    if auto_fixable:
        print(f"\n🤖 发现 {len(auto_fixable)} 个可自动修复的问题")
        print("::set-output name=need_fix::true")
        print(f"::set-output name=fix_types::{','.join([f['type'] for f in auto_fixable])}")

if __name__ == '__main__':
    main()
