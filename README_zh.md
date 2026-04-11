<div align="center">

# fcitx5-vinput

**Fcitx5 语音输入方案 — 本地与云端 ASR、LLM 改写、多发行版支持**

[![License](https://img.shields.io/github/license/xifan2333/fcitx5-vinput)](LICENSE)
[![CI](https://github.com/xifan2333/fcitx5-vinput/actions/workflows/ci.yml/badge.svg)](https://github.com/xifan2333/fcitx5-vinput/actions/workflows/ci.yml)
[![Release](https://img.shields.io/github/v/release/xifan2333/fcitx5-vinput)](https://github.com/xifan2333/fcitx5-vinput/releases)
[![AUR](https://img.shields.io/aur/version/fcitx5-vinput-bin)](https://aur.archlinux.org/packages/fcitx5-vinput-bin)
[![Downloads](https://img.shields.io/github/downloads/xifan2333/fcitx5-vinput/total)](https://github.com/xifan2333/fcitx5-vinput/releases)

[English](README.md) | [中文](README_zh.md) | [文档站点](https://xifan2333.github.io/fcitx5-vinput/zh-cn/)

https://github.com/user-attachments/assets/5a548a68-153c-4842-bab6-926f30bb720e

</div>

## 功能特性

- **两种触发模式** — 短按切换录音，长按即说即停（push-to-talk）
- **本地与云端 ASR** — 离线 [sherpa-onnx](https://github.com/k2-fsa/sherpa-onnx) 模型或云端提供商（豆包、阿里百炼、ElevenLabs、OpenAI 兼容），`F8` 运行时切换
- **LLM 后处理** — 通过场景实现纠错、格式化、翻译
- **命令模式** — 选中文本，语音指令直接改写
- **GUI 与 CLI** — `vinput-gui` 快速上手，`vinput` CLI 完整控制
- **多发行版** — Arch、Fedora、Ubuntu/Debian、Nix、Flatpak

## 快速安装

```bash
# Arch Linux (AUR)
yay -S fcitx5-vinput-bin

# Fedora (COPR)
sudo dnf copr enable xifan/fcitx5-vinput-bin && sudo dnf install fcitx5-vinput

# Ubuntu 24.04 (PPA)
sudo add-apt-repository ppa:xifan233/ppa && sudo apt update && sudo apt install fcitx5-vinput
```

Nix、Flatpak、Debian、本地安装包和源码构建请查看[安装页面](https://xifan2333.github.io/fcitx5-vinput/zh-cn/install/)。

## 快速开始

```bash
systemctl --user enable --now vinput-daemon.service
fcitx5 -r
```

打开 **Vinput GUI** → **资源 → 模型** → 下载并激活一个模型。然后：

- **短按** `Alt_R` — 开始/停止录音
- **长按** `Alt_R` — 即说即停

完整的配置指南、ASR 配置、场景与 LLM、资源仓库贡献规范请查看[文档站点](https://xifan2333.github.io/fcitx5-vinput/zh-cn/)。

## 许可证

[MIT](LICENSE)
