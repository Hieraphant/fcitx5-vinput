---
title: Settings
description: Capture device, gain, VAD, and other global settings.
---

## Concepts

Global settings control Vinput's base runtime environment, independent of ASR engine or scene selection.

Corresponding config:

```json
{
  "global": {
    "default_language": "en",
    "capture_device": "rnnoise_source"
  },
  "asr": {
    "normalize_audio": true,
    "input_gain": 5.0,
    "vad": {
      "enabled": true
    }
  }
}
```

## Capture device

Select the microphone input source. Vinput captures audio via PipeWire — this lists available capture devices on your system.

### GUI

In Vinput GUI, select the device on the **Control** page.

### CLI

```bash
vinput device list              # List available capture devices
vinput device use <name>        # Set active device
```

## Input gain

`input_gain` controls the recording volume multiplier. Increase this if your microphone is too quiet for good recognition.

```bash
vinput config set /asr/input_gain 5.0
```

## Audio normalization

`normalize_audio` normalizes the audio signal for more consistent volume levels.

```bash
vinput config set /asr/normalize_audio true
```

## VAD (Voice Activity Detection)

VAD detects whether someone is speaking. When enabled, silent segments are filtered out automatically, reducing unnecessary recognition attempts.

```bash
vinput config set /asr/vad/enabled true
```

## Language

`default_language` sets the default language, affecting UI display and some model behavior.

```bash
vinput config set /global/default_language zh
```

## Configuration file locations

| File | Path |
|------|------|
| Plugin config (keybindings, etc.) | `~/.config/fcitx5/conf/vinput.conf` |
| Core config (model, LLM, scenes) | `~/.config/vinput/config.json` |
| Model directory | `~/.local/share/vinput/models/` |

### Flatpak paths

| File | Path |
|------|------|
| Plugin config (keybindings, etc.) | `~/.var/app/org.fcitx.Fcitx5/config/fcitx5/conf/vinput.conf` |
| Core config (model, LLM, scenes) | `~/.var/app/org.fcitx.Fcitx5/config/vinput/config.json` |
| Model directory | `~/.var/app/org.fcitx.Fcitx5/data/vinput/models/` |

## General CLI

```bash
vinput config get <JSON Pointer>        # Read any config value
vinput config set <JSON Pointer> <val>  # Write any config value
vinput config edit core                 # Edit core config (config.json)
vinput config edit fcitx                # Edit Fcitx addon config (vinput.conf)
vinput daemon restart                   # Restart daemon to apply changes
```
