#pragma once

#include <string>

struct AsrConfig {
  std::string language;
  std::string hotwords_file;
  int thread_num = 4;
  bool vad_enabled = true;
  std::string vad_model_path;
};
