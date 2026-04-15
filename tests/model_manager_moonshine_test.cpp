#include "common/asr/model_manager.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

namespace {

std::string UniqueSuffix() {
  return std::to_string(static_cast<unsigned long long>(std::rand()));
}

fs::path MakeModelDir(const fs::path &base_dir) {
  return base_dir / "sherpa-onnx" / "moonshine-test";
}

bool WriteTextFile(const fs::path &path, const std::string &content) {
  std::ofstream out(path);
  if (!out.is_open()) {
    return false;
  }
  out << content;
  return out.good();
}

bool WriteBinaryPlaceholder(const fs::path &path) {
  return WriteTextFile(path, "placeholder\n");
}

bool WriteMoonshineMetadata(const fs::path &model_dir, bool include_cached_decoder) {
  const char *json_prefix = R"JSON({
  "backend": "sherpa-offline",
  "runtime": "offline",
  "family": "moonshine",
  "language": "en",
  "model": {
    "tokens": "tokens.txt",
    "moonshine": {
      "preprocessor": "preprocess.onnx",
      "encoder": "encode.int8.onnx",
      "uncached_decoder": "uncached_decode.int8.onnx",
)JSON";

  const char *json_cached = R"JSON(      "cached_decoder": "cached_decode.int8.onnx"
    }
  }
}
)JSON";

  const char *json_without_cached = R"JSON(      "cached_decoder": "missing.onnx"
    }
  }
}
)JSON";

  return WriteTextFile(model_dir / "vinput-model.json",
                       std::string(json_prefix) +
                           (include_cached_decoder ? json_cached
                                                   : json_without_cached));
}

bool Expect(bool condition, const std::string &message) {
  if (!condition) {
    std::cerr << message << std::endl;
    return false;
  }
  return true;
}

}  // namespace

int main() {
  const fs::path base_dir =
      fs::temp_directory_path() / ("vinput-moonshine-test-" + UniqueSuffix());
  const fs::path model_dir = MakeModelDir(base_dir);
  std::error_code ec;
  fs::create_directories(model_dir, ec);
  if (ec) {
    std::cerr << "failed to create temp model dir: " << ec.message() << std::endl;
    return 1;
  }

  auto cleanup = [&] {
    std::error_code cleanup_ec;
    fs::remove_all(base_dir, cleanup_ec);
  };

  if (!WriteBinaryPlaceholder(model_dir / "tokens.txt") ||
      !WriteBinaryPlaceholder(model_dir / "preprocess.onnx") ||
      !WriteBinaryPlaceholder(model_dir / "encode.int8.onnx") ||
      !WriteBinaryPlaceholder(model_dir / "uncached_decode.int8.onnx") ||
      !WriteBinaryPlaceholder(model_dir / "cached_decode.int8.onnx") ||
      !WriteMoonshineMetadata(model_dir, true)) {
    std::cerr << "failed to stage valid Moonshine model files" << std::endl;
    cleanup();
    return 1;
  }

  const std::string model_id = "model.sherpa-onnx.moonshine-test";
  ModelManager manager(base_dir.string(), model_id);

  std::string error;
  if (!Expect(manager.Validate(model_id, &error),
              "expected valid Moonshine model to validate, got: " + error)) {
    cleanup();
    return 1;
  }

  fs::remove(model_dir / "cached_decode.int8.onnx", ec);
  if (ec) {
    std::cerr << "failed to remove cached decoder: " << ec.message() << std::endl;
    cleanup();
    return 1;
  }

  error.clear();
  if (!Expect(!manager.Validate(model_id, &error),
              "expected Moonshine validation failure when cached decoder is missing")) {
    cleanup();
    return 1;
  }

  if (!Expect(error.find("cached_decoder") != std::string::npos,
              "expected cached_decoder in validation error, got: " + error)) {
    cleanup();
    return 1;
  }

  cleanup();
  return 0;
}
