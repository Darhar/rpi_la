#pragma once

#include "../drivers/gusman_logic.h"

#include <cstdint>
#include <vector>

struct LogicEvent {
    uint32_t sampleIndex;   // sample where this state begins
    uint32_t state;         // channel bitfield from this sample onward
};


struct LogicChannelEvent {
    uint32_t sampleIndex;
    uint8_t state;
};

static constexpr uint8_t LOGIC_CH_EVENT_LEVEL = 0x01;

inline LogicChannelEvent makeLogicChannelEvent(uint32_t sampleIndex, bool level)
{
    LogicChannelEvent ev {};
    ev.sampleIndex = sampleIndex;
    ev.state = level ? LOGIC_CH_EVENT_LEVEL : 0;
    return ev;
}

inline bool logicChannelEventLevel(const LogicChannelEvent& ev)
{
    return (ev.state & LOGIC_CH_EVENT_LEVEL) != 0;
}

struct LogicEventCapture {
    uint32_t sampleCount = 0;
    uint8_t channelCount = 0;
    uint32_t sampleRateHz = 0;

    std::vector<LogicEvent> events;

    std::vector<std::vector<LogicChannelEvent>> channelEvents;
};

LogicEventCapture buildLogicEvents(
    const GusmanCapture& cap,
    uint32_t sampleRateHz);

int findNextEvent(const LogicEventCapture& ev, uint32_t sample);
int findPrevEvent(const LogicEventCapture& ev, uint32_t sample);
int findNextChannelEvent(const LogicEventCapture& ev,
                         uint32_t sample,
                         uint8_t channel);

int findPrevChannelEvent(const LogicEventCapture& ev,
                         uint32_t sample,
                         uint8_t channel);
uint32_t stateAtSample(const LogicEventCapture& ev, uint32_t sample);
bool channelAtSample(const LogicEventCapture& ev,
                     uint32_t sample,
                     uint8_t channel);