# LLM Adaptor 脚本

这个目录保存项目维护的官方 LLM adaptor 脚本源码。它们不再作为内置资源
随包安装，而是由 `vinput-registry` 发布，再由客户端按需下载到本地。
adaptor 是本地的 OpenAI 兼容桥接进程，它和 `config.json` 里的 LLM
provider 不同，后者指向场景实际调用的 API 端点。

- 本仓库源码目录：`data/llm-adaptors/`
- 用户本地安装路径：`~/.config/vinput/llm-adaptors/`
- 安装方式：`vinput adaptor install <id>`
- 运行时状态路径：`${XDG_RUNTIME_DIR:-/tmp}/vinput/adaptors/`

可通过以下命令管理官方和用户 adaptor：

- `vinput adaptor list`
- `vinput adaptor list --available`
- `vinput adaptor install <id>`
- `vinput adaptor start <id>`
- `vinput adaptor stop <id>`

运行时调用使用显式命令配置：

- `command`：可执行文件或解释器
- `args`：脚本路径和额外参数
- `env`：环境变量覆盖

元数据块格式如下：

```text
# ==vinput-adaptor==
# @name         MTranServer Proxy
# @description  为 MTranServer 提供 OpenAI 兼容代理
# @author       xifan
# @version      1.0.0
# ==/vinput-adaptor==
```
