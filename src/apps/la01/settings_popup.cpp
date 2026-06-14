#include "settings_popup.h"
#include "../../gfx/text.h"

#include <cstdio>

void drawSettingsOverlay(
    ScreenBuffer& screen,
    const CaptureSettings& settings,
    const SettingsUi& ui)
{
    constexpr uint8_t COL_BG     = 0;
    constexpr uint8_t COL_TEXT   = 7;
    constexpr uint8_t COL_SELECT = 3;
    constexpr uint8_t COL_BOX    = 8;

    screen.fillRect(40, 40, 400, 220, COL_BG);

    screen.lineH(40, 439, 40, COL_BOX);
    screen.lineH(40, 439, 259, COL_BOX);
    screen.lineV(40, 40, 259, COL_BOX);
    screen.lineV(439, 40, 259, COL_BOX);

    screen.drawText(64, 56, "CAPTURE SETTINGS", COL_TEXT);

    char line[64];

    std::snprintf(
        line,
        sizeof(line),
        "%c Sample rate: %u Hz",
        ui.selected == 0 ? '>' : ' ',
        settings.sampleRateHz);
    screen.drawText(64, 88, line, ui.selected == 0 ? COL_SELECT : COL_TEXT);

    std::snprintf(
        line,
        sizeof(line),
        "%c Samples:     %u",
        ui.selected == 1 ? '>' : ' ',
        settings.postSamples);
    screen.drawText(64, 112, line, ui.selected == 1 ? COL_SELECT : COL_TEXT);

    std::snprintf(
        line,
        sizeof(line),
        "%c UART baud:   %u",
        ui.selected == 2 ? '>' : ' ',
        settings.uartBaud);
    screen.drawText(64, 136, line, ui.selected == 2 ? COL_SELECT : COL_TEXT);

    screen.drawText(64, 184, "Up/Down select", COL_TEXT);
    screen.drawText(64, 200, "Left/Right change", COL_TEXT);
    screen.drawText(64, 216, "Enter recapture", COL_TEXT);
    screen.drawText(64, 232, "Esc cancel", COL_TEXT);
}
