#pragma once

#include "app_keys.h"
#include "app_settings.h"
#include "settings_popup.h"
#include "export_popup.h"

#include "drivers/cardkb.h"
#include "drivers/gusman_logic.h"
#include "drivers/ili9488.h"

#include "gfx/renderer.h"
#include "terminal/screenbuffer.h"

#include "logic/logic_events.h"
#include "logic/logic_view.h"
#include "logic/protocols/protocol_candidate.h"
#include "logic/protocols/protocol_registry.h"

#include <cstdint>
#include <memory>
#include <vector>

enum class AppMode {
    View,
    Settings,
    Export
};

static constexpr uint32_t APP_WAVE_W = 408;

class LaApp {
public:
    bool init();
    int run();

private:
    bool capture();
    void redraw();

    void handleKey(uint8_t key);
    void handleViewKey(uint8_t key);
    void handleSettingsKey(uint8_t key);
    void handleExportKey(uint8_t key);

    void resetViewAfterCapture();
    void requestRedraw();

private:
    bool m_running = true;
    bool m_captureRequested = false;
    bool m_redrawRequested = true;

    AppMode m_appMode = AppMode::View;

    ILI9488 m_lcd;
    ScreenBuffer m_screen;
    std::unique_ptr<Renderer> m_renderer;

    CardKB m_keyboard;

    GusmanLogicAnalyzer m_analyser;
    GusmanCaptureRequest m_request;
    GusmanCapture m_rawCapture;

    LogicEventCapture m_events;
    LogicViewState m_viewState;
    LogicView m_view;

    CaptureSettings m_settings;
    CaptureSettings m_editSettings;

    SettingsUi m_settingsUi;
    ExportUi m_exportUi;

    std::vector<ProtocolCandidate> m_protocolCandidates;
    ProtocolRegistry m_protocolRegistry;
};