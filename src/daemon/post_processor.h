#pragma once

#include "common/recognition_result.h"
#include "common/postprocess_scene.h"
#include "common/vinput_config.h"

class PostProcessor {
public:
    PostProcessor();
    ~PostProcessor();

    vinput::result::Payload Process(const std::string& raw_text,
                                    const vinput::scene::Definition& scene,
                                    const VinputSettings& settings) const;
};
