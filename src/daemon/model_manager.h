#pragma once

#include <string>
#include <vector>

struct ModelPaths {
    std::string model;     // model.onnx 或 model.int8.onnx
    std::string tokens;    // tokens.txt
};

class ModelManager {
public:
    explicit ModelManager(const std::string& base_dir = "",
                          const std::string& model_name = "paraformer-zh");

    bool EnsureModels();
    ModelPaths GetPaths() const;
    std::vector<std::string> ListModels() const;
    std::string GetBaseDir() const;
    std::string GetModelName() const;

private:
    bool IsValidModelDir(const std::string& model_name) const;
    std::string base_dir_;
    std::string model_name_;
};
