#pragma once

#include <fcitx-config/configuration.h>
#include <fcitx-config/option.h>
#include <fcitx-utils/key.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

inline constexpr const char* kVinputConfigPath = "conf/vinput.conf";

struct VinputSettings {
    fcitx::KeyList triggerKeys{fcitx::Key(FcitxKey_Control_R)};
    fcitx::KeyList sceneMenuKey{fcitx::Key(FcitxKey_F9)};
    fcitx::KeyList pagePrevKeys{
        fcitx::Key(FcitxKey_Page_Up),
        fcitx::Key(FcitxKey_KP_Page_Up),
    };
    fcitx::KeyList pageNextKeys{
        fcitx::Key(FcitxKey_Page_Down),
        fcitx::Key(FcitxKey_KP_Page_Down),
    };
    std::string captureDevice;
    std::string modelName{"paraformer-zh"};
    bool llmEnabled = false;
    std::string llmBaseUrl;
    std::string llmApiKey;
    std::string llmModel;
    int llmCandidateCount = 1;
    int llmTimeoutMs = 4000;
};

struct CaptureDeviceChoice {
    std::string value;
    std::string label;
};

struct StringEnumAnnotation {
    std::vector<std::pair<std::string, std::string>> items;
    std::string tooltip;

    bool skipDescription() const { return false; }
    bool skipSave() const { return false; }
    void dumpDescription(fcitx::RawConfig& config) const;
};

class VinputConfig : public fcitx::Configuration {
public:
    VinputConfig(const VinputSettings& settings,
                 std::vector<CaptureDeviceChoice> devices,
                 std::vector<std::string> models,
                 std::string model_base_dir,
                 bool chinese_ui);
    VinputConfig(const VinputConfig&) = delete;
    VinputConfig& operator=(const VinputConfig&) = delete;

    const char* typeName() const override { return "VinputConfig"; }
    VinputSettings settings() const;

    fcitx::Option<fcitx::KeyList, fcitx::ListConstrain<fcitx::KeyConstrain>,
                  fcitx::DefaultMarshaller<fcitx::KeyList>,
                  fcitx::ToolTipAnnotation>
        triggerKey;

    fcitx::Option<std::string, fcitx::NoConstrain<std::string>,
                  fcitx::DefaultMarshaller<std::string>, StringEnumAnnotation>
        captureDevice;

    fcitx::Option<std::string, fcitx::NoConstrain<std::string>,
                  fcitx::DefaultMarshaller<std::string>, StringEnumAnnotation>
        modelName;

    fcitx::Option<bool, fcitx::NoConstrain<bool>,
                  fcitx::DefaultMarshaller<bool>, fcitx::ToolTipAnnotation>
        llmEnabled;

    fcitx::Option<fcitx::KeyList, fcitx::ListConstrain<fcitx::KeyConstrain>,
                  fcitx::DefaultMarshaller<fcitx::KeyList>,
                  fcitx::ToolTipAnnotation>
        sceneMenuKey;

    fcitx::Option<fcitx::KeyList, fcitx::ListConstrain<fcitx::KeyConstrain>,
                  fcitx::DefaultMarshaller<fcitx::KeyList>,
                  fcitx::ToolTipAnnotation>
        pagePrevKeys;

    fcitx::Option<fcitx::KeyList, fcitx::ListConstrain<fcitx::KeyConstrain>,
                  fcitx::DefaultMarshaller<fcitx::KeyList>,
                  fcitx::ToolTipAnnotation>
        pageNextKeys;

    fcitx::Option<std::string, fcitx::NoConstrain<std::string>,
                  fcitx::DefaultMarshaller<std::string>,
                  fcitx::ToolTipAnnotation>
        llmBaseUrl;

    fcitx::Option<std::string, fcitx::NoConstrain<std::string>,
                  fcitx::DefaultMarshaller<std::string>,
                  fcitx::ToolTipAnnotation>
        llmApiKey;

    fcitx::Option<std::string, fcitx::NoConstrain<std::string>,
                  fcitx::DefaultMarshaller<std::string>,
                  fcitx::ToolTipAnnotation>
        llmModel;

    fcitx::Option<int, fcitx::IntConstrain, fcitx::DefaultMarshaller<int>,
                  fcitx::ToolTipAnnotation>
        llmCandidateCount;

    fcitx::Option<int, fcitx::IntConstrain, fcitx::DefaultMarshaller<int>,
                  fcitx::ToolTipAnnotation>
        llmTimeoutMs;
};

bool UseChineseUi();
std::vector<CaptureDeviceChoice> ListCaptureDevices();
VinputSettings LoadVinputSettings();
bool SaveVinputSettings(const VinputSettings& settings);
std::unique_ptr<VinputConfig> BuildVinputConfig(const VinputSettings& settings);
