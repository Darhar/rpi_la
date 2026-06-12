#include "logic_events.h"

#include <algorithm>

LogicEventCapture buildLogicEvents(
    const GusmanCapture& cap,
    uint32_t sampleRateHz)
{
    LogicEventCapture out {};

    out.sampleCount = cap.sampleCount;
    out.channelCount = cap.channelCount;
    out.sampleRateHz = sampleRateHz;
    out.channelEvents.resize(cap.channelCount);

    if(cap.samples.empty() || cap.sampleCount == 0) {
        return out;
    }

    uint32_t prev = cap.samples[0];

    // Existing whole-bus initial state.
    out.events.push_back({0, prev});

    // Initial state for each channel.
    for(uint8_t ch = 0; ch < cap.channelCount; ++ch) {
        bool level = (prev & (1u << ch)) != 0;
        out.channelEvents[ch].push_back(
            makeLogicChannelEvent(0, level)
        );
    }

    for(uint32_t i = 1; i < cap.sampleCount && i < cap.samples.size(); ++i) {
        uint32_t state = cap.samples[i];

        if(state == prev) {
            continue;
        }

        // Existing whole-bus event.
        out.events.push_back({i, state});

        uint32_t changed = state ^ prev;

        for(uint8_t ch = 0; ch < cap.channelCount; ++ch) {
            if(changed & (1u << ch)) {
                bool level = (state & (1u << ch)) != 0;
                out.channelEvents[ch].push_back(
                    makeLogicChannelEvent(i, level)
                );
            }
        }

        prev = state;
    }

    return out;
}

int findNextEvent(const LogicEventCapture& ev, uint32_t sample)
{
    auto it = std::upper_bound(
        ev.events.begin(),
        ev.events.end(),
        sample,
        [](uint32_t s, const LogicEvent& e) {
            return s < e.sampleIndex;
        }
    );

    if(it == ev.events.end())
        return -1;

    return int(it - ev.events.begin());
}

int findPrevEvent(const LogicEventCapture& ev, uint32_t sample)
{
    auto it = std::lower_bound(
        ev.events.begin(),
        ev.events.end(),
        sample,
        [](const LogicEvent& e, uint32_t s) {
            return e.sampleIndex < s;
        }
    );

    if(it == ev.events.begin())
        return -1;

    --it;
    return int(it - ev.events.begin());
}

uint32_t stateAtSample(const LogicEventCapture& ev, uint32_t sample)
{
    if(ev.events.empty())
        return 0;

    auto it = std::upper_bound(
        ev.events.begin(),
        ev.events.end(),
        sample,
        [](uint32_t s, const LogicEvent& e) {
            return s < e.sampleIndex;
        }
    );

    if(it == ev.events.begin())
        return ev.events.front().state;

    --it;
    return it->state;
}
int findNextChannelEvent(const LogicEventCapture& ev,
                         uint32_t sample,
                         uint8_t channel)
{
    if(ev.events.size() < 2)
        return -1;

    uint32_t prevState = stateAtSample(ev, sample);
    bool prevBit = (prevState >> channel) & 1u;

    int start = findNextEvent(ev, sample);
    if(start < 0)
        return -1;

    for(size_t i = size_t(start); i < ev.events.size(); ++i) {
        bool bit = (ev.events[i].state >> channel) & 1u;

        if(bit != prevBit)
            return int(i);

        prevBit = bit;
    }

    return -1;
}

int findPrevChannelEvent(const LogicEventCapture& ev,
                         uint32_t sample,
                         uint8_t channel)
{
    if(ev.events.size() < 2)
        return -1;

    int start = findPrevEvent(ev, sample);
    if(start < 0)
        return -1;

    bool currentBit = channelAtSample(ev, sample, channel);

    for(int i = start; i >= 0; --i) {
        bool bit = (ev.events[size_t(i)].state >> channel) & 1u;

        if(bit != currentBit)
            return i;

        currentBit = bit;
    }

    return -1;
}
bool channelAtSample(const LogicEventCapture& ev,
                     uint32_t sample,
                     uint8_t channel)
{
    return (stateAtSample(ev, sample) >> channel) & 1u;
}