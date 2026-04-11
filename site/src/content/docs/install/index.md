---
title: Install
description: Install fcitx5-vinput on Linux.
---

Choose the path that matches how you prefer to install software on Linux.

## Packages

- **Arch Linux (AUR)**: `fcitx5-vinput-bin`
- **Fedora (COPR)**: `xifan/fcitx5-vinput-bin`
- **Ubuntu 24.04 (PPA)**: `ppa:xifan233/ppa`
- **Ubuntu / Debian**: install the `.deb` package from GitHub Releases
- **Nix**: use the flake package
- **Flatpak**: install the Fcitx5 add-on build

## Build from source

```bash
sudo bash scripts/build-sherpa-onnx.sh
cmake --preset release-clang-mold
cmake --build --preset release-clang-mold
sudo cmake --install build
```

Or use `just`:

```bash
just sherpa
just configure-release
just build
sudo just install
```

## Release packages

- [GitHub Releases](https://github.com/xifan2333/fcitx5-vinput/releases)
