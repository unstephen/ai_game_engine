# 引擎开发助手 - 飞书集成配置

## 🎯 目标
将"引擎开发助手"配置为独立的飞书机器人，可以在飞书群聊中与团队成员互动。

---

## 📋 配置步骤

### 方案一：使用现有飞书应用（推荐）

**优点**：简单快速，复用现有配置
**缺点**：所有消息通过同一个机器人发送

#### 1. 配置 OpenClaw
编辑 `~/.openclaw/openclaw.json`:
```json
{
  "agents": {
    "engine-dev-assistant": {
      "enabled": true,
      "model": "bailian/qwen3.5-plus",
      "feishu": {
        "appId": "cli_a932a240ec78dccb",
        "appSecret": "hJdDEwRxtCx4B4SqLGWRUgmopLSwlD16",
        "displayName": "引擎开发助手",
        "avatar": "https://example.com/avatar.png"
      }
    }
  },
  "channels": {
    "feishu": {
      "enabled": true,
      "appId": "cli_a932a240ec78dccb",
      "appSecret": "hJdDEwRxtCx4B4SqLGWRUgmopLSwlD16",
      "domain": "feishu",
      "connectionMode": "websocket",
      "requireMention": true,
      "dmPolicy": "allowlist",
      "allowFrom": ["ou_596e89d8200fe9c496ef0a0da986780b"]
    }
  }
}
```

#### 2. 添加关键词触发
编辑 `~/.openclaw/workspace/agents/engine-dev-assistant/AGENT.md`:
```markdown
## 触发词
- "@引擎开发助手"
- "@engine"
- "引擎助手"
```

#### 3. 重启 OpenClaw
```bash
openclaw gateway restart
```

---

### 方案二：创建新的飞书应用（独立机器人）

**优点**：独立的机器人身份，可自定义头像和名称
**缺点**：需要额外配置飞书应用

#### 1. 创建飞书应用
1. 访问 https://open.feishu.cn/app
2. 点击"创建企业自建应用"
3. 填写应用信息：
   - **应用名称**: 引擎开发助手
   - **应用图标**: 上传机器人头像
   - **能力配置**: 开启"机器人"

#### 2. 配置权限
在"权限管理"中添加：
- `im:message` - 发送消息
- `im:chat` - 访问群聊
- `im:resource` - 访问资源

#### 3. 获取凭证
在"凭证与基础信息"中获取：
- `App ID` (cli_xxx)
- `App Secret`

#### 4. 配置事件订阅
在"事件订阅"中：
1. 启用事件订阅
2. 配置订阅地址：`https://your-server.com/feishu/event`
3. 订阅事件：
   - `im.message.receive_v1` - 接收消息

#### 5. 配置机器人
在"机器人"配置页：
1. 启用机器人
2. 配置机器人名称：引擎开发助手
3. 配置头像
4. 配置开场白

#### 6. 发布应用
点击"发布"按钮，等待审核通过。

---

## 🔧 OpenClaw 配置

### 多机器人配置
```json
{
  "agents": {
    "main-assistant": {
      "enabled": true,
      "model": "bailian/qwen3.5-plus",
      "name": "龙景"
    },
    "engine-dev-assistant": {
      "enabled": true,
      "model": "bailian/qwen3.5-plus",
      "name": "引擎开发助手",
      "feishu": {
        "appId": "cli_new_app_id",
        "appSecret": "new_app_secret"
      }
    }
  },
  "channels": {
    "feishu-main": {
      "enabled": true,
      "appId": "cli_a932a240ec78dccb",
      "appSecret": "hJdDEwRxtCx4B4SqLGWRUgmopLSwlD16",
      "agent": "main-assistant"
    },
    "feishu-engine": {
      "enabled": true,
      "appId": "cli_new_app_id",
      "appSecret": "new_app_secret",
      "agent": "engine-dev-assistant"
    }
  }
}
```

---

## 💬 使用方式

### 私聊模式
```
用户：@引擎开发助手 帮我看看今天的 CI 状态
助手：好的，让我检查一下...
```

### 群聊模式
```
群聊：游戏引擎开发组
用户：@引擎开发助手 编译失败了怎么办？
助手：让我分析一下错误日志...
```

---

## 🎨 机器人形象配置

### 头像建议
- 使用引擎相关的图标
- 尺寸：512x512 PNG
- 风格：科技感、简洁

### 人设配置
```markdown
## 角色设定
- **名称**: 引擎开发助手
- **身份**: 游戏引擎开发专家
- **性格**: 专业、耐心、细致
- **专业领域**: C++、D3D12、Vulkan、游戏引擎架构

## 沟通风格
- 技术术语准确
- 提供代码示例
- 附带参考链接
- 简洁直接
```

---

## 📊 监控和日志

### 查看机器人状态
```bash
# 查看引擎助手状态
openclaw agent status engine-dev-assistant

# 查看日志
openclaw agent logs engine-dev-assistant --tail 50

# 实时监控
openclaw agent logs engine-dev-assistant --follow
```

### 飞书后台监控
访问 https://open.feishu.cn/
- 查看应用使用统计
- 查看消息发送记录
- 查看错误日志

---

## 🔒 安全配置

### 访问控制
```json
{
  "feishu": {
    "dmPolicy": "allowlist",
    "allowFrom": [
      "ou_596e89d8200fe9c496ef0a0da986780b"  // 无为
    ],
    "groupPolicy": "allowlist",
    "groups": {
      "oc_xxx": {  // 游戏引擎开发组
        "enabled": true
      }
    }
  }
}
```

### 权限管理
- 只允许授权用户访问
- 群聊需要 @ 才响应
- 敏感操作需要确认

---

## 🚀 快速测试

### 1. 发送测试消息
```bash
# 通过飞书发送
@引擎开发助手 你好，测试

# 预期响应
你好！我是引擎开发助手，随时为你服务！
```

### 2. 测试 CI 查询
```
@引擎开发助手 今天的 CI 过了吗？

预期响应：
让我检查一下...
✅ Run #12: Windows Pre-Check - 通过
❌ Run #25: CI - 失败
```

### 3. 测试代码审查
```
@引擎开发助手 审查这个 PR

预期响应：
## 📋 代码审查报告
...
```

---

## 📞 故障排查

### 机器人无响应
1. 检查 OpenClaw 状态：`openclaw gateway status`
2. 查看日志：`openclaw agent logs engine-dev-assistant`
3. 重启网关：`openclaw gateway restart`

### 飞书应用未授权
1. 访问飞书应用管理后台
2. 确认应用已发布
3. 确认权限已授予

### 消息发送失败
1. 检查 App Secret 是否正确
2. 检查网络连接
3. 查看飞书后台错误日志

---

## 📚 参考资料

- [飞书开放平台](https://open.feishu.cn/)
- [飞书机器人开发文档](https://open.feishu.cn/document/ukTMukTMukTM/uEjNwUjLxYDM14SM2ATN)
- [OpenClaw 飞书集成](https://docs.openclaw.ai/channels/feishu)

---

*最后更新：2026-03-18*
*维护者：无为*
