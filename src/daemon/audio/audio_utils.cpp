#include "audio_utils.h"

#include <algorithm>
#include <cmath>

namespace vinput::audio {

void PeakNormalize(std::vector<float> &samples, float target_peak,
                   float low_threshold) {
  if (samples.empty()) return;

  float peak = 0.0f;
  for (const float s : samples) {
    float a = std::fabs(s);
    if (a > peak) peak = a;
  }

  if (peak < 1e-8f || peak >= low_threshold) return;

  float scale = target_peak / peak;
  for (float &s : samples) {
    s *= scale;
  }
}

void ApplyGain(std::vector<float> &samples, float gain) {
  if (samples.empty() || gain == 1.0f) {
    return;
  }

  for (float &s : samples) {
    s = std::clamp(s * gain, -1.0f, 1.0f);
  }
}

}  // namespace vinput::audio
