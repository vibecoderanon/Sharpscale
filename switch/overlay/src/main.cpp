#define TESLA_INIT_IMPL
#include "../include/sharpscale_overlay.hpp"
#include "../../plugin/include/config.h"
#include "../include/SaltyNX.h"
#include <cstdio>
#include <string>
#include <vector>

static SharedMemory s_shmem;
static bool s_shmem_mapped = false;
static SharpscaleSharedMemory* s_shmem_ptr = nullptr;
static Handle s_remote_shmem_handle = 0;

void SharpscaleOverlay::initServices() {
    pminfoInitialize();
    fsdevMountSdmc();

    if (SaltySD_Connect() == 0) {
        if (SaltySD_GetSharedMemoryHandle(&s_remote_shmem_handle) == 0 && s_remote_shmem_handle != 0) {
            shmemLoadRemote(&s_shmem, s_remote_shmem_handle, 0x1000, Perm_Rw);
            if (R_SUCCEEDED(shmemMap(&s_shmem))) {
                s_shmem_mapped = true;
                uint8_t* base = (uint8_t*)shmemGetAddr(&s_shmem);
                if (base) {
                    for (size_t off = 0; off + sizeof(SharpscaleSharedMemory) <= 0x1000; off += 4) {
                        SharpscaleSharedMemory* probe = (SharpscaleSharedMemory*)(base + off);
                        if (probe->magic == SHARPSCALE_SHMEM_MAGIC) {
                            s_shmem_ptr = probe;
                            break;
                        }
                    }
                }
            }
        }
    }
}

void SharpscaleOverlay::exitServices() {
    if (s_shmem_mapped) {
        shmemUnmap(&s_shmem);
        shmemClose(&s_shmem);
        s_shmem_mapped = false;
        s_shmem_ptr = nullptr;
    }
    if (s_remote_shmem_handle != 0) {
        svcCloseHandle(s_remote_shmem_handle);
        s_remote_shmem_handle = 0;
    }
    SaltySD_Term();
    fsdevUnmountDevice("sdmc");
    pminfoExit();
}

void SharpscaleOverlay::onShow() {}
void SharpscaleOverlay::onHide() {}

std::unique_ptr<tsl::Gui> SharpscaleOverlay::loadInitialGui() {
    return std::make_unique<MainGui>();
}

MainGui::MainGui() : m_current_title_id(0), m_is_game_running(false) {
    s32 num_pids = 0;
    u64 pids[64] = {0};
    if (R_SUCCEEDED(svcGetProcessList(&num_pids, pids, 64))) {
        for (s32 i = 0; i < num_pids; i++) {
            u64 tid = 0;
            if (R_SUCCEEDED(pminfoGetProgramId(&tid, pids[i]))) {
                if (tid >= 0x0100000000010000ULL && tid <= 0x01FFFFFFFFFFFFFFULL) {
                    m_current_title_id = tid;
                    m_is_game_running = true;
                    break;
                }
            }
        }
    }

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

    if (!s_shmem_ptr && s_shmem_mapped) {
        uint8_t* base = (uint8_t*)shmemGetAddr(&s_shmem);
        if (base) {
            for (size_t off = 0; off + sizeof(SharpscaleSharedMemory) <= 0x1000; off += 4) {
                SharpscaleSharedMemory* probe = (SharpscaleSharedMemory*)(base + off);
                if (probe->magic == SHARPSCALE_SHMEM_MAGIC) {
                    s_shmem_ptr = probe;
                    break;
                }
            }
        }
    }

    if (s_shmem_ptr) {
        s_shmem_ptr->scaling_mode = static_cast<uint8_t>(m_config.scaling_mode);
        s_shmem_ptr->filter_type = static_cast<uint8_t>(m_config.filter_type);
        s_shmem_ptr->aspect_ratio = static_cast<uint8_t>(m_config.aspect_ratio);
        s_shmem_ptr->sharpness = m_config.sharpness_strength;
        s_shmem_ptr->force_1080p = m_config.force_1080p_capture ? 1 : 0;
        s_shmem_ptr->show_osd = m_config.show_osd_notification ? 1 : 0;
        s_shmem_ptr->sequence_id++;
    }
}

tsl::elm::Element* MainGui::createUI() {
    auto frame = new tsl::elm::OverlayFrame("Sharpscale-NX", "v" SHARPSCALE_NX_VERSION_STRING);
    auto list = new tsl::elm::List();

    // Section 0: Engine & Game Status
    list->addItem(new tsl::elm::CategoryHeader("Sharpscale-NX Engine"));

    char status_buf[96];
    if (s_shmem_ptr && s_shmem_ptr->is_plugin_alive) {
        snprintf(status_buf, sizeof(status_buf), "Active (%ux%u -> %ux%u)",
            s_shmem_ptr->src_width, s_shmem_ptr->src_height,
            s_shmem_ptr->vp_w, s_shmem_ptr->vp_h);
    } else if (m_is_game_running) {
        snprintf(status_buf, sizeof(status_buf), "Hooked (Title: %016llX)", (unsigned long long)m_current_title_id);
    } else {
        snprintf(status_buf, sizeof(status_buf), "Standby (No game active)");
    }
    auto statusItem = new tsl::elm::ListItem("Engine Status");
    statusItem->setValue(status_buf);
    list->addItem(statusItem);

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
