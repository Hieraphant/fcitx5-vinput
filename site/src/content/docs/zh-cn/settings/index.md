---
title: 全局设置
description: 录音设备、增益、VAD 等基础配置。
---

## 概念

全局设置控制 Vinput 的基础运行环境，与 ASR 引擎和场景选择无关。

对应配置：

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

## 录音设备

选择麦克风输入源。Vinput 通过 PipeWire 获取音频，这里列出的是系统可用的采集设备。

### GUI 操作

在 Vinput GUI 的 **控制** 页面选择设备。

### CLI 操作

```bash
vinput device list              # 列出可用录音设备
vinput device use <名称>         # 设置当前设备
```

## 音频增益

`input_gain` 控制录音音量的放大倍数。如果麦克风音量太小导致识别效果差，可以调高此值。

```bash
vinput config set /asr/input_gain 5.0
```

## 音频归一化

`normalize_audio` 开启后会对音频做归一化处理，让音量更稳定。

```bash
vinput config set /asr/normalize_audio true
```

## VAD（语音活动检测）

VAD 检测录音中是否有人在说话。开启后，静音片段会被自动过滤，减少无效识别。

```bash
vinput config set /asr/vad/enabled true
```

## 语言

`default_language` 设置默认语言，影响界面显示和部分模型行为。

```bash
vinput config set /global/default_language zh
```

## 通用 CLI

```bash
vinput config get <JSON Pointer>        # 读取任意配置项
vinput config set <JSON Pointer> <值>   # 写入任意配置项
vinput config edit core                 # 编辑核心配置 config.json
vinput config edit fcitx                # 编辑 Fcitx 插件配置 vinput.conf
vinput daemon restart                   # 重启守护进程使配置生效
```
