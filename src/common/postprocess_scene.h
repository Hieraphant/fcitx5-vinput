#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace vinput::scene {

inline constexpr const char* kDefault = "default";
inline constexpr const char* kFormal = "formal";
inline constexpr const char* kCode = "code";
inline constexpr const char* kTranslate = "translate";
inline constexpr const char* kConfigPath = "conf/vinput-scenes.json";
inline constexpr const char* kPkgDataPath = "vinput/scenes.json";

struct Definition {
    std::string id;
    std::string label;
    std::string labelZh;
    std::string labelEn;
    bool llm = false;
    std::string prompt;
};

struct Config {
    std::string defaultSceneId;
    std::vector<Definition> scenes;
};

const Config& DefaultConfig();
Config LoadConfig();
const Definition* Find(const Config& config, std::string_view scene_id);
const Definition& Resolve(const Config& config, std::string_view scene_id);
std::string DisplayLabel(const Definition& scene, bool chinese_ui);

}  // namespace vinput::scene
