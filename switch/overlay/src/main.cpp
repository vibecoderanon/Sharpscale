#define TESLA_INIT_IMPL
#include "../include/sharpscale_overlay.hpp"
#include "../../plugin/include/config.h"
#include <cstdio>
#include <string>
#include <vector>

void SharpscaleOverlay::initServices() {
    // Initialize libnx services (pminfo, etc.)
}

void SharpscaleOverlay::exitServices() {
    // Clean up libnx services
}

void SharpscaleOverlay::onShow() {}
void SharpscaleOverlay::onHide() {}

std::unique_ptr<tsl::Gui> SharpscaleOverlay::loadInitialGui() {
    return std::make_unique<MainGui>();
}

MainGui::MainGui() : m_current_title_id(0), m_is_game_running(false) {
    refreshConfig();
}

void MainGui::refreshConfig() {
    config_load_defaults(&m_config);
    config_load_global(&m_config);
    if (m_current_title_id != 0) {
        config_load_title(m_current_title_id, &m_config);
    }
}

void MainGui::saveConfig() {
    if (m_current_title_id != 0) {
        config_save_title(m_current_title_id, &m_config);
    } else {
        config_save_global(&m_config);
    }
}

tsl::elm::Element* MainGui::createUI() {
    auto frame = new tsl::elm::OverlayFrame("Sharpscale-NX", "v" SHARPSCALE_NX_VERSION_STRING);
    auto list = new tsl::elm::List();

    // Section 1: Scaling Mode
    list->addItem(new tsl::elm::CategoryHeader("Display Scaling"));

    const std::vector<std::string> scaling_modes = {
        "Original (System Bilinear)",
        "Integer (Pixel Perfect Max Fit)",
        "Real (1:1 Native Centered)",
        "Fit (Aspect Correct Fit)"
    };

    auto modeItem = new tsl::elm::ListItem("Scaling Mode");
    modeItem->setValue(scaling_modes[m_config.scaling_mode]);
    modeItem->setClickListener([this, modeItem, scaling_modes](u64 keys) {
        if (keys & HidNpadButton_A) {
            m_config.scaling_mode = static_cast<SharpscaleScalingMode>((m_config.scaling_mode + 1) % 4);
            modeItem->setValue(scaling_modes[m_config.scaling_mode]);
            saveConfig();
            return true;
        }
        return false;
    });
    list->addItem(modeItem);

    // Section 2: Filtering & Post-Processing
    list->addItem(new tsl::elm::CategoryHeader("Filters & Sharpening"));

    const std::vector<std::string> filter_types = {
        "Point (Nearest Neighbor)",
        "Bilinear",
        "Sharp Bilinear",
        "AMD CAS (Contrast Adaptive)",
        "Bicubic Spline"
    };

    auto filterItem = new tsl::elm::ListItem("Filter Type");
    filterItem->setValue(filter_types[m_config.filter_type]);
    filterItem->setClickListener([this, filterItem, filter_types](u64 keys) {
        if (keys & HidNpadButton_A) {
            m_config.filter_type = static_cast<SharpscaleFilterType>((m_config.filter_type + 1) % 5);
            filterItem->setValue(filter_types[m_config.filter_type]);
            saveConfig();
            return true;
        }
        return false;
    });
    list->addItem(filterItem);

    // Section 3: Aspect Ratio Override
    list->addItem(new tsl::elm::CategoryHeader("Geometry & Aspect Ratio"));

    const std::vector<std::string> aspect_ratios = {
        "Auto (Game Default)",
        "16:9 Widescreen",
        "4:3 Retro Standard",
        "3:2 GBA Native",
        "1:1 Square Pixel",
        "10:9 Game Boy Native"
    };

    auto aspectItem = new tsl::elm::ListItem("Aspect Ratio");
    aspectItem->setValue(aspect_ratios[m_config.aspect_ratio]);
    aspectItem->setClickListener([this, aspectItem, aspect_ratios](u64 keys) {
        if (keys & HidNpadButton_A) {
            m_config.aspect_ratio = static_cast<SharpscaleAspectRatio>((m_config.aspect_ratio + 1) % 6);
            aspectItem->setValue(aspect_ratios[m_config.aspect_ratio]);
            saveConfig();
            return true;
        }
        return false;
    });
    list->addItem(aspectItem);

    // Section 4: Capture & Lossless Output
    list->addItem(new tsl::elm::CategoryHeader("Video Capture"));
    auto captureToggle = new tsl::elm::ToggleListItem("Unlock 1080p Stream (SysDVR)", m_config.force_1080p_capture);
    captureToggle->setStateChangedListener([this](bool state) {
        m_config.force_1080p_capture = state;
        saveConfig();
    });
    list->addItem(captureToggle);

    frame->setContent(list);
    return frame;
}

void MainGui::update() {
    // Handle dynamic state changes
}

int main(int argc, char **argv) {
    return tsl::loop<SharpscaleOverlay>(argc, argv);
}
