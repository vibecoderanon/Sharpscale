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

MainGui::MainGui() : m_current_title_id(0), m_is_game_running(false),
    m_status_item(nullptr), m_res_item(nullptr), m_viewport_item(nullptr),
    m_mode_item(nullptr), m_filter_item(nullptr), m_aspect_item(nullptr) {
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

void MainGui::updateTelemetry() {
    if (!m_status_item) return;

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

    if (s_shmem_ptr && s_shmem_ptr->is_plugin_alive) {
        m_status_item->setValue(s_shmem_ptr->is_docked ? "Active (Docked)" : "Active (Handheld)");
        if (m_res_item && s_shmem_ptr->src_width > 0) {
            char res_buf[64];
            snprintf(res_buf, sizeof(res_buf), "%ux%u -> %ux%u",
                s_shmem_ptr->src_width, s_shmem_ptr->src_height,
                s_shmem_ptr->vp_w, s_shmem_ptr->vp_h);
            m_res_item->setValue(res_buf);
        }
        if (m_viewport_item && s_shmem_ptr->vp_w > 0) {
            char vp_buf[64];
            snprintf(vp_buf, sizeof(vp_buf), "%ux%u @ (%u,%u)",
                s_shmem_ptr->vp_w, s_shmem_ptr->vp_h,
                s_shmem_ptr->vp_x, s_shmem_ptr->vp_y);
            m_viewport_item->setValue(vp_buf);
        }
    } else if (m_is_game_running) {
        m_status_item->setValue("Hooked");
    } else {
        m_status_item->setValue("Standby");
    }
}

tsl::elm::Element* MainGui::createUI() {
    auto frame = new tsl::elm::OverlayFrame("Sharpscale-NX", "v" SHARPSCALE_NX_VERSION_STRING);
    auto list = new tsl::elm::List();

    // Section 0: Engine & Game Status
    list->addItem(new tsl::elm::CategoryHeader("Sharpscale Engine"));

    m_status_item = new tsl::elm::ListItem("Status");
    m_res_item = new tsl::elm::ListItem("Resolution");
    m_viewport_item = new tsl::elm::ListItem("Viewport");

    updateTelemetry();

    list->addItem(m_status_item);
    if (m_is_game_running) {
        char title_buf[32];
        snprintf(title_buf, sizeof(title_buf), "%016llX", (unsigned long long)m_current_title_id);
        auto titleItem = new tsl::elm::ListItem("Title ID");
        titleItem->setValue(title_buf);
        list->addItem(titleItem);
    }
    list->addItem(m_res_item);
    list->addItem(m_viewport_item);

    // Section 1: Scaling Mode
    list->addItem(new tsl::elm::CategoryHeader("Display Scaling"));

    const std::vector<std::string> scaling_modes = {
        "Original",
        "Integer",
        "Real (1:1)",
        "Fit"
    };

    m_mode_item = new tsl::elm::ListItem("Scaling Mode");
    m_mode_item->setValue(scaling_modes[m_config.scaling_mode]);
    m_mode_item->setClickListener([this, scaling_modes](u64 keys) {
        if (keys & HidNpadButton_A) {
            m_config.scaling_mode = static_cast<SharpscaleScalingMode>((m_config.scaling_mode + 1) % 4);
            m_mode_item->setValue(scaling_modes[m_config.scaling_mode]);
            saveConfig();
            updateTelemetry();
            return true;
        }
        return false;
    });
    list->addItem(m_mode_item);

    // Section 2: Filtering & Post-Processing
    list->addItem(new tsl::elm::CategoryHeader("Filters & Sharpening"));

    const std::vector<std::string> filter_types = {
        "Point (Nearest)",
        "Bilinear",
        "Sharp Bilinear",
        "AMD CAS",
        "Bicubic"
    };

    m_filter_item = new tsl::elm::ListItem("Filter Type");
    m_filter_item->setValue(filter_types[m_config.filter_type]);
    m_filter_item->setClickListener([this, filter_types](u64 keys) {
        if (keys & HidNpadButton_A) {
            m_config.filter_type = static_cast<SharpscaleFilterType>((m_config.filter_type + 1) % 5);
            m_filter_item->setValue(filter_types[m_config.filter_type]);
            saveConfig();
            updateTelemetry();
            return true;
        }
        return false;
    });
    list->addItem(m_filter_item);

    // Section 3: Aspect Ratio Override
    list->addItem(new tsl::elm::CategoryHeader("Geometry & Aspect Ratio"));

    const std::vector<std::string> aspect_ratios = {
        "Auto",
        "16:9",
        "4:3",
        "3:2 (GBA)",
        "1:1",
        "10:9 (GB)"
    };

    m_aspect_item = new tsl::elm::ListItem("Aspect Ratio");
    m_aspect_item->setValue(aspect_ratios[m_config.aspect_ratio]);
    m_aspect_item->setClickListener([this, aspect_ratios](u64 keys) {
        if (keys & HidNpadButton_A) {
            m_config.aspect_ratio = static_cast<SharpscaleAspectRatio>((m_config.aspect_ratio + 1) % 6);
            m_aspect_item->setValue(aspect_ratios[m_config.aspect_ratio]);
            saveConfig();
            updateTelemetry();
            return true;
        }
        return false;
    });
    list->addItem(m_aspect_item);

    // Section 4: Capture & Lossless Output
    list->addItem(new tsl::elm::CategoryHeader("Video Capture"));
    auto captureToggle = new tsl::elm::ToggleListItem("Unlock 1080p Stream (SysDVR)", m_config.force_1080p_capture);
    captureToggle->setStateChangedListener([this](bool state) {
        m_config.force_1080p_capture = state;
        saveConfig();
        updateTelemetry();
    });
    list->addItem(captureToggle);

    frame->setContent(list);
    return frame;
}

void MainGui::update() {
    updateTelemetry();
}

int main(int argc, char **argv) {
    return tsl::loop<SharpscaleOverlay>(argc, argv);
}
