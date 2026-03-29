#pragma once

#include <string>

struct AsrConfig {
  std::string language;
  std::string hotwords_file;
  int thread_num = 4;
  bool normalize_audio = true;
  float input_gain = 1.0f;
  bool vad_enabled = true;
  std::string vad_model_path;
};
