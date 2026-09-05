#ifndef SHARPSCALE_OVERLAY_HPP
#define SHARPSCALE_OVERLAY_HPP

#include <tesla.hpp>
#include "../../plugin/include/sharpscale_nx.h"

class SharpscaleOverlay : public tsl::Overlay {
public:
    virtual void initServices() override;
    virtual void exitServices() override;
    virtual void onShow() override;
    virtual void onHide() override;
    virtual std::unique_ptr<tsl::Gui> loadInitialGui() override;
};

class MainGui : public tsl::Gui {
private:
    SharpscaleConfig m_config;
    uint64_t m_current_title_id;
    bool m_is_game_running;

public:
    MainGui();
    virtual tsl::elm::Element* createUI() override;
    virtual void update() override;

private:
    void refreshConfig();
    void saveConfig();
};

#endif // SHARPSCALE_OVERLAY_HPP
