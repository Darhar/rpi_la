#pragma once

#include "../terminal/screenbuffer.h"
#include "logic_events.h"
#include <cstdint>
#include <vector>
#include "pwm_measure.h"
#include "protocols/protocol_types.h"


struct ChannelProtocol {
    ProtocolType type = ProtocolType::None;
    uint32_t parameter = 0;
};

struct LogicChannelView {
    bool visible = true;
    bool minimised = false;
    ChannelProtocol protocol;
};

struct LogicViewState {
    uint32_t firstSample = 0;
    uint32_t samplesPerPixel = 1;
    uint32_t cursorSample = 0;
    uint32_t markerA = 0;
    uint32_t markerB = 0;
    bool markerASet = false;
    bool markerBSet = false;
    uint8_t selectedChannel = 0;
    std::vector<LogicChannelView> channels;
};

void clampViewState(
    LogicViewState& state,
    const LogicEventCapture& cap
);

void centreCursor(
    LogicViewState& state,
    const LogicEventCapture& cap
);

void ensureCursorVisible(
    LogicViewState& state,
    const LogicEventCapture& cap
);

class LogicView {
public:
    //void draw(ScreenBuffer& screen, const LogicEventCapture& cap);
    void draw(ScreenBuffer& screen, const LogicEventCapture& cap, const LogicViewState& state);
    void drawCursor(ScreenBuffer& screen,
                    const LogicEventCapture& cap,
                    const LogicViewState& state);

    void drawMeasurements(ScreenBuffer& screen,
                        const LogicEventCapture& cap,
                        const LogicViewState& state);  
    void drawSampleMarker(
        ScreenBuffer& screen,
        const LogicViewState& state,
        uint32_t sample,
        uint8_t colour);

private:
    void drawChannel(
        ScreenBuffer& screen,
        const LogicEventCapture& cap,
        int ch,
        int y,
        int h,
        uint32_t firstSample,
        uint32_t samplesPerPixel
    );
};
