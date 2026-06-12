#include "logic_view.h"
#include <cstdio>
#include "uart_decode.h"

#define V_STAT_BORDER_X 4
#define V_STAT_BORDER_Y 4
#define V_MEAS_Y        (V_STAT_BORDER_Y + 14)
#define V_CUR_X         320
#define V_DECODE_Y      (V_MEAS_Y + 14)

constexpr int SCREEN_W = 480;
constexpr int SCREEN_H = 320;

constexpr int MENU_H = 50;
constexpr int LEFT_W = 72;
constexpr int WAVE_X = LEFT_W;
constexpr int WAVE_W = SCREEN_W - LEFT_W;

constexpr int NORMAL_ROW_H = 34;
constexpr int MINI_ROW_H   = 18;

constexpr uint8_t COL_BG        = 0;
constexpr uint8_t COL_TEXT      = 7;
constexpr uint8_t COL_GRID      = 8;
constexpr uint8_t COL_WAVE      = 2;
constexpr uint8_t COL_HIGH      = 3;
constexpr uint8_t COL_SELECT    = 1;
constexpr uint8_t COL_SELECT_BG = 1;

constexpr uint8_t COL_MARKER_A = 4;
constexpr uint8_t COL_MARKER_B = 5;

std::string formatFrequency(double hz)
{
    char buf[32];

    if(hz >= 1000000.0)
        snprintf(buf,sizeof(buf),"%.1fMHz",hz/1000000.0);
    else if(hz >= 1000.0)
        snprintf(buf,sizeof(buf),"%.1fkHz",hz/1000.0);
    else
        snprintf(buf,sizeof(buf),"%.1fHz",hz);

    return buf;
}

static inline bool bitAt(
    const LogicEventCapture& cap,
    uint32_t sample,
    int ch)
{
    return channelAtSample(cap, sample, uint8_t(ch));
}

static inline bool channelVisible(
    const LogicViewState& state,
    int ch)
{
    if(ch < 0) return false;
    if(state.channels.empty()) return true;
    if(size_t(ch) >= state.channels.size()) return true;
    return state.channels[ch].visible;
}

static inline bool channelMinimised(
    const LogicViewState& state,
    int ch)
{
    if(ch < 0)  return false;
    if(state.channels.empty())  return false;
    if(size_t(ch) >= state.channels.size()) return false;
    return state.channels[ch].minimised;
}

void LogicView::draw(
    ScreenBuffer& screen,
    const LogicEventCapture& cap,
    const LogicViewState& state)
{
    char title[64];
    int y = MENU_H;
    uint32_t firstSample = state.firstSample;
    uint32_t samplesPerPixel = state.samplesPerPixel == 0 ? 1 : state.samplesPerPixel;
    const int maxChannels = cap.channelCount > 8 ? 8 : cap.channelCount;
    double t = double(state.cursorSample) / double(cap.sampleRateHz);
    screen.clear(COL_BG);

    std::snprintf(title,sizeof(title),
        "frq:%s s:%u spp:%u 1st:%u cur:%u %.1fus",
        formatFrequency(cap.sampleRateHz).c_str(),
        cap.sampleCount,
        state.samplesPerPixel,
        state.firstSample,
        state.cursorSample,
        t * 1000000.0
    );

    screen.drawText(V_STAT_BORDER_X, V_STAT_BORDER_Y, title, COL_TEXT);
    screen.lineH(0, SCREEN_W - 1, MENU_H - 1, COL_GRID);

    for(int ch = 0; ch < maxChannels; ++ch) {
        if(!channelVisible(state, ch)) continue;
        int rowH = channelMinimised(state, ch)
            ? MINI_ROW_H
            : NORMAL_ROW_H;

        if(y + rowH >= SCREEN_H)  break;
        bool selected = (ch == state.selectedChannel);
        char label[20];

        if(selected) {
            screen.fillRect(0, y, LEFT_W, rowH, COL_SELECT_BG);
            screen.lineH(0, SCREEN_W - 1, y, COL_SELECT);
            screen.lineH(0, SCREEN_W - 1, y + rowH - 1, COL_SELECT);
            screen.lineV(0, y, y + rowH - 1, COL_SELECT);
            screen.lineV(SCREEN_W - 1, y, y + rowH - 1, COL_SELECT);
        } else {
            screen.lineH(0, SCREEN_W - 1, y, COL_GRID);
        }

        if(channelMinimised(state, ch)) {
            std::snprintf(label, sizeof(label), "CH%d -", ch);
        } else {
            std::snprintf(label, sizeof(label), "CH%d", ch);
        }

        screen.drawText(4, y + 3, label, COL_TEXT);

        drawChannel(
            screen,
            cap,
            ch,
            y + 2,
            rowH - 4,
            firstSample,
            samplesPerPixel
        );

        y += rowH;
    }

    drawCursor(screen, cap, state);
    drawMeasurements(screen, cap, state);
    if(state.markerASet)
        drawSampleMarker(screen, state, state.markerA, COL_MARKER_A);

    if(state.markerBSet)
        drawSampleMarker(screen, state, state.markerB, COL_MARKER_B);    
}

void LogicView::drawChannel(
    ScreenBuffer& screen,
    const LogicEventCapture& cap,
    int ch,
    int y,
    int h,
    uint32_t firstSample,
    uint32_t samplesPerPixel)
{
    if(h < 8) return;
    if(samplesPerPixel == 0) samplesPerPixel = 1;

    int highY = y + 3;
    int lowY  = y + h - 4;
    bool prev = bitAt(cap, firstSample, ch);

    for(int px = 0; px < WAVE_W; ++px) {

        uint32_t s0 = firstSample + uint32_t(px) * samplesPerPixel;
        uint32_t s1 = s0 + samplesPerPixel;

        if(s0 >= cap.sampleCount) break;

        bool seenHigh = false;
        bool seenLow = false;

        for(uint32_t s = s0; s < s1 && s < cap.sampleCount; ++s) {
            bool b = bitAt(cap, s, ch);

            if(b)
                seenHigh = true;
            else
                seenLow = true;
        }

        int x = WAVE_X + px;

        if(seenHigh && seenLow) {
            screen.lineV(x, highY, lowY, COL_HIGH);
        } else if(seenHigh) {
            screen.setPixel(x, highY, COL_WAVE);
        } else {
            screen.setPixel(x, lowY, COL_WAVE);
        }

        bool current = seenHigh && !seenLow;
        if(px > 0 && current != prev)
            screen.lineV(x, highY, lowY, COL_WAVE);
        prev = current;
    }
}

void LogicView::drawCursor(
    ScreenBuffer& screen,
    const LogicEventCapture&,
    const LogicViewState& state)
{
    if(state.samplesPerPixel == 0) return;
    if(state.cursorSample < state.firstSample) return;

    uint32_t offset = state.cursorSample - state.firstSample;
    uint32_t px = offset / state.samplesPerPixel;

    if(px >= WAVE_W) return;
    int x = WAVE_X + int(px);
    screen.lineV(x, MENU_H, SCREEN_H - 1, COL_SELECT);
}

void LogicView::drawSampleMarker(
    ScreenBuffer& screen,
    const LogicViewState& state,
    uint32_t sample,
    uint8_t colour)
{
    if(sample < state.firstSample)
        return;

    uint32_t offset = sample - state.firstSample;
    uint32_t px = offset / state.samplesPerPixel;

    if(px >= WAVE_W)
        return;

    int x = WAVE_X + int(px);

    screen.lineV(x, MENU_H, SCREEN_H - 1, colour);
}

void LogicView::drawMeasurements(
    ScreenBuffer& screen,
    const LogicEventCapture& cap,
    const LogicViewState& state)
{
        
    char m[96];

    if(state.markerASet && state.markerBSet && cap.sampleRateHz != 0) {
        uint32_t a = state.markerA;
        uint32_t b = state.markerB;
        uint32_t delta = (a > b) ? (a - b) : (b - a);
        double dt = double(delta) / double(cap.sampleRateHz);
        double freq = dt > 0.0 ? 1.0 / dt : 0.0;

        std::snprintf(
            m,
            sizeof(m),
            "A:%u B:%u d:%u %.3fus %.3fHz",
            a,
            b,
            delta,
            dt * 1000000.0,
            freq);
    }
    else if(state.markerASet) {
        std::snprintf(m, sizeof(m), "A:%u", state.markerA);
    }
    else if(state.markerBSet) {
        std::snprintf(m, sizeof(m), "B:%u", state.markerB);
    }
    else {
        std::snprintf(m, sizeof(m), "No markers set");
    }

    screen.drawText(V_STAT_BORDER_X, V_MEAS_Y, m, COL_TEXT);

    PwmMeasurement pwm;

    if(measurePwm(cap, state.selectedChannel, state.cursorSample, pwm)) {
        char p[96];

        std::snprintf(
            p,
            sizeof(p),
            "PWM CH%u F:%.2fHz D:%.1f%% H:%.2fus L:%.2fus",
            pwm.channel,
            pwm.frequencyHz,
            pwm.dutyPercent,
            pwm.highUs,
            pwm.lowUs
        );

        screen.drawText(V_STAT_BORDER_X, V_DECODE_Y, p, COL_TEXT);
    }
    else {
        char p[64];

        std::snprintf(
            p,
            sizeof(p),
            "PWM CH%u: no full cycle",
            state.selectedChannel
        );

        screen.drawText(V_STAT_BORDER_X, V_DECODE_Y, p, COL_TEXT);
    } 
    
/*
    //screen.drawText(V_STAT_BORDER_X, V_DECODE_Y, "UART decode active", COL_TEXT);

    UartDecodeResult uart;

    if(decodeUart8N1(cap, state.selectedChannel, 9600, uart)) {
        char text[96];
        int pos = 0;

        pos += std::snprintf(
            text + pos,
            sizeof(text) - pos,
            "UART CH%u %u: ",
            uart.channel,
            uart.baud);

        for(size_t i = 0;
            i < uart.bytes.size() && pos < int(sizeof(text)) - 8;
            ++i)
        {
            uint8_t c = uart.bytes[i].value;

            if(c >= 32 && c <= 126) {
                text[pos++] = char(c);
            }
            else {
                if(c == '\n') {
                    pos += std::snprintf(text + pos, sizeof(text) - pos, "<LF>");
                }
                else if(c == '\r') {
                    pos += std::snprintf(text + pos, sizeof(text) - pos, "<CR>");
                }
                else if(c >= 32 && c <= 126) {
                    text[pos++] = char(c);
                }
                else {
                    pos += std::snprintf(text + pos, sizeof(text) - pos, "<%02X>", c);
                }
            }
        }

        text[pos] = 0;

        screen.drawText(V_STAT_BORDER_X, V_DECODE_Y, text, COL_TEXT);
    }
    else {
        screen.drawText(V_STAT_BORDER_X, V_DECODE_Y,"UART: no decode", COL_TEXT);
    }*/
}
