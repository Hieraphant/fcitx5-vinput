# ASR Provider 脚本

这个目录保存项目维护的官方云端 ASR provider 脚本源码。它们不再作为内置资源
随包安装，而是由 `vinput-registry` 发布，再由客户端按需下载到本地：

- 本仓库源码目录：`data/asr-providers/`
- 用户本地安装路径：`~/.config/vinput/asr-providers/`
- 安装方式：`vinput asr install <id>`

受管理的 provider 应使用显式命令配置：

- `command`：可执行文件或解释器
- `args`：脚本路径和额外参数
- `env`：环境变量覆盖

当前官方脚本包括：

- `elevenlabs_speech_to_text.py`
- `openai_compatible_speech_to_text.py`
- `doubao_speech_to_text.py`

其中：

- `openai_compatible_speech_to_text.py` 适用于 OpenAI 官方接口、
  SiliconFlow，以及其他 `/v1/audio/transcriptions` 兼容接口
- `doubao_speech_to_text.py` 适用于豆包语音 / 火山引擎录音文件极速版识别接口

推荐统一遵循以下脚本规范：

- 仅使用 Python 标准库；不要引入第三方依赖
- 每个脚本单文件自足；不要导入本目录下的其他 provider 脚本
- 保持统一入口：`main() -> int`
- 保持统一输入函数名：`read_audio_input()`
- 保持统一环境变量读取函数名：
  `get_required_env()`、`get_optional_env()`、`get_optional_int_env()`
- 网络主逻辑统一放在 `transcribe()` 中
- `stdout` 只输出最终文本；`stderr` 只输出错误
- 退出码 `0` 表示成功，`1` 表示远端请求/运行错误，`2` 表示配置或输入错误

可选的元数据块格式如下：

```text
# ==vinput-asr-provider==
# @name         ElevenLabs Speech to Text
# @description  通过 ElevenLabs API 调用云端 ASR
# @author       xifan
# @version      1.0.0
# ==/vinput-asr-provider==
```
