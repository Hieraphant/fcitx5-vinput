# LLM Adaptors

This directory contains built-in LLM adaptor scripts. An adaptor is a local
OpenAI-compatible bridge process. It is different from an LLM provider entry in
`config.json`, which points to the API endpoint that scenes actually call.

- Built-in install path: `/usr/share/fcitx5-vinput/llm-adaptors/`
- User override path: `~/.config/vinput/llm-adaptors/`
- Runtime state path: `${XDG_RUNTIME_DIR:-/tmp}/vinput/adaptors/`

Manage built-in and user adaptors with:

- `vinput adaptor list`
- `vinput adaptor start <id>`
- `vinput adaptor stop <id>`

The metadata block format is:

```text
# ==vinput-adaptor==
# @name         MTranServer Proxy
# @description  OpenAI-compatible proxy for MTranServer
# @author       xifan
# @version      1.0.0
# ==/vinput-adaptor==
```
