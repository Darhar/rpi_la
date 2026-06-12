#include "pwm_measure.h"

static inline bool bitOf(uint32_t state, uint8_t ch)
{
    return ((state >> ch) & 1u) != 0;
}

bool measurePwm(
    const LogicEventCapture& cap,
    uint8_t channel,
    uint32_t startSample,
    PwmMeasurement& out)
{
    out = PwmMeasurement{};
    out.channel = channel;

    if(cap.sampleRateHz == 0)
        return false;

    if(channel >= cap.channelCount)
        return false;

    if(cap.events.size() < 3)
        return false;

    bool haveRise1 = false;
    bool haveFall  = false;

    uint32_t rise1 = 0;
    uint32_t fall  = 0;
    uint32_t rise2 = 0;

    uint32_t prevState = cap.events[0].state;

    for(size_t i = 1; i < cap.events.size(); ++i) {
        const LogicEvent& ev = cap.events[i];

        uint32_t s = ev.sampleIndex;
        uint32_t newState = ev.state;

        bool prevBit = bitOf(prevState, channel);
        bool newBit  = bitOf(newState, channel);

        prevState = newState;

        if(prevBit == newBit)
            continue;

        if(s < startSample)
            continue;

        bool rising  = (!prevBit && newBit);
        bool falling = (prevBit && !newBit);

        if(!haveRise1) {
            if(rising) {
                rise1 = s;
                haveRise1 = true;
            }

            continue;
        }

        if(!haveFall) {
            if(falling) {
                fall = s;
                haveFall = true;
            }

            continue;
        }

        if(rising) {
            rise2 = s;
            break;
        }
    }

    if(!haveRise1 || !haveFall || rise2 <= rise1)
        return false;

    uint32_t highSamples = fall - rise1;
    uint32_t periodSamples = rise2 - rise1;

    if(highSamples == 0 || periodSamples == 0)
        return false;

    if(highSamples >= periodSamples)
        return false;

    uint32_t lowSamples = periodSamples - highSamples;

    out.valid = true;
    out.risingSample = rise1;
    out.fallingSample = fall;
    out.nextRisingSample = rise2;

    out.highSamples = highSamples;
    out.lowSamples = lowSamples;
    out.periodSamples = periodSamples;

    out.highUs =
        double(highSamples) * 1000000.0 / double(cap.sampleRateHz);

    out.lowUs =
        double(lowSamples) * 1000000.0 / double(cap.sampleRateHz);

    out.periodUs =
        double(periodSamples) * 1000000.0 / double(cap.sampleRateHz);

    out.dutyPercent =
        100.0 * double(highSamples) / double(periodSamples);

    out.frequencyHz =
        double(cap.sampleRateHz) / double(periodSamples);

    return true;
}

bool measurePwmAroundCursor(
    const LogicEventCapture& cap,
    uint8_t channel,
    uint32_t cursorSample,
    PwmMeasurement& out)
{
    out = PwmMeasurement{};
    out.channel = channel;

    if(cap.sampleRateHz == 0)
        return false;

    if(channel >= cap.channelCount)
        return false;

    if(cap.events.size() < 3)
        return false;

    struct Edge {
        uint32_t sample;
        bool rising;
    };

    std::vector<Edge> edges;

    uint32_t prevState = cap.events[0].state;

    for(size_t i = 1; i < cap.events.size(); ++i) {
        const LogicEvent& ev = cap.events[i];

        bool prevBit = bitOf(prevState, channel);
        bool newBit  = bitOf(ev.state, channel);

        if(prevBit != newBit) {
            edges.push_back({
                ev.sampleIndex,
                !prevBit && newBit
            });
        }

        prevState = ev.state;
    }

    if(edges.size() < 3)
        return false;

    for(size_t i = 0; i + 2 < edges.size(); ++i) {
        const Edge& r1 = edges[i];
        const Edge& f  = edges[i + 1];
        const Edge& r2 = edges[i + 2];

        if(!r1.rising)
            continue;

        if(f.rising)
            continue;

        if(!r2.rising)
            continue;

        if(cursorSample < r1.sample)
            continue;

        if(cursorSample > r2.sample)
            continue;

        uint32_t highSamples = f.sample - r1.sample;
        uint32_t periodSamples = r2.sample - r1.sample;

        if(highSamples == 0 || periodSamples == 0)
            return false;

        if(highSamples >= periodSamples)
            return false;

        uint32_t lowSamples = periodSamples - highSamples;

        out.valid = true;

        out.risingSample = r1.sample;
        out.fallingSample = f.sample;
        out.nextRisingSample = r2.sample;

        out.highSamples = highSamples;
        out.lowSamples = lowSamples;
        out.periodSamples = periodSamples;

        out.highUs =
            double(highSamples) * 1000000.0 / double(cap.sampleRateHz);

        out.lowUs =
            double(lowSamples) * 1000000.0 / double(cap.sampleRateHz);

        out.periodUs =
            double(periodSamples) * 1000000.0 / double(cap.sampleRateHz);

        out.dutyPercent =
            100.0 * double(highSamples) / double(periodSamples);

        out.frequencyHz =
            double(cap.sampleRateHz) / double(periodSamples);

        return true;
    }

    return false;
}
