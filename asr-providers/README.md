# ASR Provider Scripts

This directory contains built-in ASR provider scripts discovered by short name.

- Built-in install path: `/usr/share/fcitx5-vinput/asr-providers/`
- User override path: `~/.config/vinput/asr-providers/`

Short-name resolution only uses the directories above. If you configure an
absolute path in `asr.providers[].command`, the script can live anywhere as
long as it is runnable.

The optional metadata block format is:

```text
# ==vinput-asr-provider==
# @name         ElevenLabs Speech to Text
# @description  Cloud ASR via ElevenLabs API
# @author       xifan
# @version      1.0.0
# ==/vinput-asr-provider==
```
