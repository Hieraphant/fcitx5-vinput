---
title: 安装
description: 在 Linux 上安装 fcitx5-vinput。
---

按你习惯的方式选择安装路径即可。

## 安装包

- **Arch Linux (AUR)**：`fcitx5-vinput-bin`
- **Fedora (COPR)**：`xifan/fcitx5-vinput-bin`
- **Ubuntu 24.04 (PPA)**：`ppa:xifan233/ppa`
- **Ubuntu / Debian**：从 GitHub Releases 安装 `.deb` 包
- **Nix**：使用 flake 包
- **Flatpak**：安装 Fcitx5 Add-on 构建

## 源码构建

```bash
sudo bash scripts/build-sherpa-onnx.sh
cmake --preset release-clang-mold
cmake --build --preset release-clang-mold
sudo cmake --install build
```

也可以用 `just`：

```bash
just sherpa
just configure-release
just build
sudo just install
```

## 发布包

- [GitHub Releases](https://github.com/xifan2333/fcitx5-vinput/releases)
