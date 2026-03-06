#include "model_manager.h"

#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <filesystem>

namespace fs = std::filesystem;

ModelManager::ModelManager(const std::string& base_dir,
                           const std::string& model_name) {
    if (!base_dir.empty()) {
        base_dir_ = base_dir;
    } else {
        const char* xdg = std::getenv("XDG_DATA_HOME");
        if (xdg && xdg[0] != '\0') {
            base_dir_ = std::string(xdg) + "/fcitx5-vinput/models";
        } else {
            const char* home = std::getenv("HOME");
            base_dir_ = std::string(home ? home : "/tmp") +
                        "/.local/share/fcitx5-vinput/models";
        }
    }

    model_name_ = model_name.empty() ? "paraformer-zh" : model_name;
}

bool ModelManager::EnsureModels() {
    auto paths = GetPaths();

    if (!fs::exists(paths.model)) {
        fprintf(stderr,
                "vinput: ASR model not found for '%s' at %s\n"
                "vinput: Please download a sherpa-onnx paraformer model:\n"
                "  mkdir -p %s/<model-name>\n"
                "  # Download from https://github.com/k2-fsa/sherpa-onnx/releases/tag/asr-models\n"
                "  # Put model.int8.onnx/model.onnx and tokens.txt into that directory\n",
                model_name_.c_str(), paths.model.c_str(), base_dir_.c_str());
        return false;
    }

    if (!fs::exists(paths.tokens)) {
        fprintf(stderr, "vinput: tokens.txt not found for '%s' at %s\n",
                model_name_.c_str(), paths.tokens.c_str());
        return false;
    }

    return true;
}

ModelPaths ModelManager::GetPaths() const {
    auto dir = fs::path(base_dir_) / model_name_;
    ModelPaths paths;
    paths.model = (dir / "model.int8.onnx").string();
    paths.tokens = (dir / "tokens.txt").string();

    if (!fs::exists(paths.model)) {
        auto alt = (dir / "model.onnx").string();
        if (fs::exists(alt)) {
            paths.model = alt;
        }
    }

    return paths;
}

std::string ModelManager::GetBaseDir() const {
    return base_dir_;
}

std::vector<std::string> ModelManager::ListModels() const {
    std::vector<std::string> models;
    const auto root = fs::path(base_dir_);
    if (!fs::exists(root) || !fs::is_directory(root)) {
        return models;
    }

    for (const auto& entry : fs::directory_iterator(root)) {
        if (!entry.is_directory()) {
            continue;
        }

        const auto model_name = entry.path().filename().string();
        if (IsValidModelDir(model_name)) {
            models.push_back(model_name);
        }
    }

    std::sort(models.begin(), models.end());
    return models;
}

std::string ModelManager::GetModelName() const {
    return model_name_;
}

bool ModelManager::IsValidModelDir(const std::string& model_name) const {
    const auto dir = fs::path(base_dir_) / model_name;
    if (!fs::exists(dir) || !fs::is_directory(dir)) {
        return false;
    }

    const auto tokens = dir / "tokens.txt";
    const auto int8_model = dir / "model.int8.onnx";
    const auto model = dir / "model.onnx";
    return fs::exists(tokens) && (fs::exists(int8_model) || fs::exists(model));
}
