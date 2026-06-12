#include "uart_decode.h"

static inline bool bitAt(
    const LogicEventCapture& cap,
    uint32_t sample,
    uint8_t channel)
{
    return channelAtSample(cap, sample, channel);
}

static inline bool bitOf(uint32_t state, uint8_t ch)
{
    return ((state >> ch) & 1u) != 0;
}

bool decodeUart8N1(
    const LogicEventCapture& cap,
    uint8_t channel,
    uint32_t baud,
    UartDecodeResult& out)
{
    out = UartDecodeResult{};
    out.channel = channel;
    out.baud = baud;

    if(channel >= cap.channelCount)
        return false;

    if(cap.sampleRateHz == 0 || baud == 0)
        return false;

    if(cap.events.size() < 2)
        return false;


    double samplesPerBit = double(cap.sampleRateHz) / double(baud);
    out.bitSamples = uint32_t(samplesPerBit + 0.5);
    uint32_t prevState = cap.events[0].state;

    for(size_t i = 1; i < cap.events.size(); ++i) {
        const LogicEvent& ev = cap.events[i];

        bool prevBit = bitOf(prevState, channel);
        bool newBit  = bitOf(ev.state, channel);

        prevState = ev.state;

        uint32_t startSample = ev.sampleIndex;

        // Require line to have been high for at least half a bit before start.
        // This rejects falling edges inside data bits.

        uint32_t idleCheckSamples = uint32_t(samplesPerBit * 1.5);

        if(startSample < idleCheckSamples) continue;

        bool idleWasHigh = true;

        for(uint32_t s = startSample - idleCheckSamples; s < startSample; ++s)
        {
            if(!bitAt(cap, s, channel)) {
                idleWasHigh = false;
                break;
            }
        }

        if(!idleWasHigh) continue;

        // Confirm middle of start bit is still low
        uint32_t startMid = startSample + samplesPerBit / 2;

        if(startMid >= cap.sampleCount) break;

        if(bitAt(cap, startMid, channel)) {
            continue;
        }

        uint8_t value = 0;
        double samplesPerBit = double(cap.sampleRateHz) / double(baud);

        for(int bit = 0; bit < 8; ++bit) {
                uint32_t sample =
                    uint32_t(
                        double(startSample) +
                        (1.5 + double(bit)) * samplesPerBit +
                        0.5
                    );

            if(sample >= cap.sampleCount)
                break;

            if(bitAt(cap, sample, channel)) {
                value |= uint8_t(1u << bit);
            }
        }

       uint32_t stopSample =
        uint32_t(
            double(startSample) +
            9.5 * samplesPerBit +
            0.5
        );

        if(stopSample >= cap.sampleCount)
            break;

        bool stopBit = bitAt(cap, stopSample, channel);

        UartDecodedByte b;
        b.startSample = startSample;

        b.endSample =
            uint32_t(
                double(startSample) +
                10.0 * samplesPerBit +
                0.5
            );

        b.value = value;
        b.framingOk = stopBit;

        out.bytes.push_back(b);

        // Skip ahead close to the end of this character
        while(i + 1 < cap.events.size() &&
              cap.events[i + 1].sampleIndex < b.endSample) {
            ++i;
        }

        if(out.bytes.size() >= 64)
            break;
    }

    out.valid = !out.bytes.empty();
    return out.valid;
}