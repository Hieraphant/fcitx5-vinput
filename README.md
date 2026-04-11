<div align="center">

# fcitx5-vinput

**Voice input for Fcitx5 — local and cloud ASR, LLM rewriting, cross-distro packages**

[![License](https://img.shields.io/github/license/xifan2333/fcitx5-vinput)](LICENSE)
[![CI](https://github.com/xifan2333/fcitx5-vinput/actions/workflows/ci.yml/badge.svg)](https://github.com/xifan2333/fcitx5-vinput/actions/workflows/ci.yml)
[![Release](https://img.shields.io/github/v/release/xifan2333/fcitx5-vinput)](https://github.com/xifan2333/fcitx5-vinput/releases)
[![AUR](https://img.shields.io/aur/version/fcitx5-vinput-bin)](https://aur.archlinux.org/packages/fcitx5-vinput-bin)
[![Downloads](https://img.shields.io/github/downloads/xifan2333/fcitx5-vinput/total)](https://github.com/xifan2333/fcitx5-vinput/releases)

[English](README.md) | [中文](README_zh.md) | [Documentation](https://xifan2333.github.io/fcitx5-vinput/)

https://github.com/user-attachments/assets/5a548a68-153c-4842-bab6-926f30bb720e

</div>

## Features

- **Two trigger modes** — tap to toggle recording, or hold to push-to-talk
- **Local & cloud ASR** — offline [sherpa-onnx](https://github.com/k2-fsa/sherpa-onnx) models or cloud providers (Doubao, Aliyun Bailian, ElevenLabs, OpenAI-compatible), switchable at runtime with `F8`
- **LLM post-processing** — error correction, formatting, translation via scenes
- **Command mode** — select text, speak an instruction, release to apply
- **GUI & CLI** — `vinput-gui` for quick setup, `vinput` CLI for full control
- **Cross-distro** — Arch, Fedora, Ubuntu/Debian, Nix, Flatpak

## Quick install

```bash
# Arch Linux (AUR)
yay -S fcitx5-vinput-bin

# Fedora (COPR)
sudo dnf copr enable xifan/fcitx5-vinput-bin && sudo dnf install fcitx5-vinput

# Ubuntu 24.04 (PPA)
sudo add-apt-repository ppa:xifan233/ppa && sudo apt update && sudo apt install fcitx5-vinput
```

For Nix, Flatpak, Debian, local packages, and build from source, see the [Install](https://xifan2333.github.io/fcitx5-vinput/install/) page.

## Quick start

```bash
systemctl --user enable --now vinput-daemon.service
fcitx5 -r
```

Open **Vinput GUI** → **Resources → Models** → download and activate a model. Then:

- **Tap** `Alt_R` — start/stop recording
- **Hold** `Alt_R` — push-to-talk

For full setup guide, ASR configuration, scenes & LLM, and registry contribution, see the [documentation site](https://xifan2333.github.io/fcitx5-vinput/).

## License

[MIT](LICENSE)
