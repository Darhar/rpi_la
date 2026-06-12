#include "export_csv.h"

#include <cstdio>
#include <cstdint>

static double sampleToUs(uint32_t sample, uint32_t sampleRateHz)
{
    if(sampleRateHz == 0) return 0.0;
    return (double(sample) * 1000000.0) / double(sampleRateHz);
}

static bool exportChannelTransitionsLong(
    FILE* f,
    const LogicEventCapture& cap)
{
    std::fprintf(f, "sample,time_us,channel,level\n");
    printf("event size:%d\n",cap.channelEvents.size());
    for(size_t ch = 0; ch < cap.channelEvents.size(); ++ch) {
        const auto& evs = cap.channelEvents[ch];

        for(const auto& ev : evs) {
            std::fprintf(
                f,
                "%u,%.3f,%zu,%u\n",
                ev.sampleIndex,
                sampleToUs(ev.sampleIndex, cap.sampleRateHz),
                ch,
                logicChannelEventLevel(ev) ? 1 : 0
            );
        }
    }

    return true;
}

static bool exportEventBusTransitions(
    FILE* f,
    const LogicEventCapture& cap)
{
    std::fprintf(f, "sample,time_us,state_hex,state_dec\n");

    for(const auto& ev : cap.events) {
        std::fprintf(
            f,
            "%u,%.3f,0x%08x,%u\n",
            ev.sampleIndex,
            sampleToUs(ev.sampleIndex, cap.sampleRateHz),
            ev.state,
            ev.state
        );
    }

    return true;
}

static bool exportSamplesWide(
    FILE* f,
    const GusmanCapture& raw,
    uint32_t sampleRateHz)
{
    std::fprintf(f, "sample,time_us,state");

    for(uint8_t ch = 0; ch < raw.channelCount; ++ch) {
        std::fprintf(f, ",CH%u", ch);
    }

    std::fprintf(f, "\n");

    for(uint32_t i = 0; i < raw.sampleCount && i < raw.samples.size(); ++i) {
        uint32_t state = raw.samples[i];

        std::fprintf(
            f,
            "%u,%.3f,0x%08x",
            i,
            sampleToUs(i, sampleRateHz),
            state
        );

        for(uint8_t ch = 0; ch < raw.channelCount; ++ch) {
            std::fprintf(f, ",%u", (state & (1u << ch)) ? 1 : 0);
        }

        std::fprintf(f, "\n");
    }

    return true;
}

static bool exportChannelTransitionsWide(
    FILE* f,
    const LogicEventCapture& cap)
{
    std::fprintf(f, "sample,time_us");

    for(uint8_t ch = 0; ch < cap.channelCount; ++ch) {
        std::fprintf(f, ",CH%u", ch);
    }

    std::fprintf(f, "\n");

    for(uint8_t ch = 0; ch < cap.channelEvents.size(); ++ch) {
        for(const auto& ev : cap.channelEvents[ch]) {
            std::fprintf(
                f,
                "%u,%.3f",
                ev.sampleIndex,
                sampleToUs(ev.sampleIndex, cap.sampleRateHz)
            );

            for(uint8_t outCh = 0; outCh < cap.channelCount; ++outCh) {
                if(outCh == ch) {
                    std::fprintf(f, ",%u",
                        logicChannelEventLevel(ev) ? 1 : 0);
                } else {
                    std::fprintf(f, ",");
                }
            }

            std::fprintf(f, "\n");
        }
    }

    return true;
}

bool exportCaptureCsv(
    const char* path,
    const LogicEventCapture& events,
    const GusmanCapture* raw,
    CsvExportFormat format)
{
    FILE* f = std::fopen(path, "w");
    if(!f) {
        return false;
    }

    bool ok = false;

    switch(format) {
        case CsvExportFormat::ChannelTransitionsLong:
            ok = exportChannelTransitionsLong(f, events);
            break;

        case CsvExportFormat::ChannelTransitionsWide:
            ok = exportChannelTransitionsWide(f, events);
            break;

        case CsvExportFormat::EventBusTransitions:
            ok = exportEventBusTransitions(f, events);
            break;

        case CsvExportFormat::SamplesWide:
            if(raw) {
                ok = exportSamplesWide(f, *raw, events.sampleRateHz);
            }
            break;
    }

    std::fclose(f);
    return ok;
}