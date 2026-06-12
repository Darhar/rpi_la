#include "capture_file.h"

#include <cstdio>
#include <cstring>

static constexpr char CAP_MAGIC[8] = { 'L','A','0','1','C','A','P',0 };
static constexpr uint32_t CAP_VERSION = 1;

struct FileHeader {
    char magic[8];
    uint32_t version;
    uint32_t headerSize;
};

struct ChunkHeader {
    char id[8];
    uint32_t version;
    uint32_t size;
};

struct SavedChannelView {
    uint8_t visible;
    uint8_t minimised;
    uint32_t protocolType;
    uint32_t protocolParameter;
};

static bool writeBytes(FILE* f, const void* data, size_t size)
{
    return fwrite(data, 1, size, f) == size;
}

static bool readBytes(FILE* f, void* data, size_t size)
{
    return fread(data, 1, size, f) == size;
}

static bool writeChunk(FILE* f, const char* id, const void* data, uint32_t size)
{
    ChunkHeader ch = {};
    strncpy(ch.id, id, sizeof(ch.id) - 1);
    ch.version = 1;
    ch.size = size;

    return writeBytes(f, &ch, sizeof(ch)) &&
           writeBytes(f, data, size);
}

static bool readChunkHeader(FILE* f, ChunkHeader& ch)
{
    return readBytes(f, &ch, sizeof(ch));
}

bool saveCaptureFile(const char* path, const CaptureFile& capture)
{
    FILE* f = fopen(path, "wb");
    if(!f) return false;

    FileHeader header = {};
    memcpy(header.magic, CAP_MAGIC, sizeof(header.magic));
    header.version = CAP_VERSION;
    header.headerSize = sizeof(FileHeader);

    if(!writeBytes(f, &header, sizeof(header))) {
        fclose(f);
        return false;
    }

    if(!writeChunk(f, "META", &capture.metadata, sizeof(CaptureMetadata))) {
        fclose(f);
        return false;
    }

    /*
        RAW chunk format:
            GusmanCapture fixed fields
            sample vector count
            sample data
            timestamp vector count
            timestamp data
    */

    {
        uint32_t sampleVecCount = capture.raw.samples.size();
        uint32_t timestampVecCount = capture.raw.timestamps.size();

        uint32_t rawChunkSize =
            sizeof(capture.raw.sampleCount) +
            sizeof(capture.raw.channelCount) +
            sizeof(capture.raw.mode) +
            sizeof(sampleVecCount) +
            sampleVecCount * sizeof(uint32_t) +
            sizeof(timestampVecCount) +
            timestampVecCount * sizeof(uint32_t);

        ChunkHeader ch = {};
        strncpy(ch.id, "RAW", sizeof(ch.id) - 1);
        ch.version = 1;
        ch.size = rawChunkSize;

        if(!writeBytes(f, &ch, sizeof(ch)) ||
           !writeBytes(f, &capture.raw.sampleCount, sizeof(capture.raw.sampleCount)) ||
           !writeBytes(f, &capture.raw.channelCount, sizeof(capture.raw.channelCount)) ||
           !writeBytes(f, &capture.raw.mode, sizeof(capture.raw.mode)) ||
           !writeBytes(f, &sampleVecCount, sizeof(sampleVecCount)) ||
           !writeBytes(f, capture.raw.samples.data(), sampleVecCount * sizeof(uint32_t)) ||
           !writeBytes(f, &timestampVecCount, sizeof(timestampVecCount)) ||
           !writeBytes(f, capture.raw.timestamps.data(), timestampVecCount * sizeof(uint32_t))) {
            fclose(f);
            return false;
        }
    }

    {
        uint32_t channelCount = capture.viewState.channels.size();

        uint32_t viewChunkSize =
            sizeof(capture.viewState.firstSample) +
            sizeof(capture.viewState.samplesPerPixel) +
            sizeof(capture.viewState.cursorSample) +
            sizeof(capture.viewState.markerA) +
            sizeof(capture.viewState.markerB) +
            sizeof(capture.viewState.markerASet) +
            sizeof(capture.viewState.markerBSet) +
            sizeof(capture.viewState.selectedChannel) +
            sizeof(channelCount) +
            channelCount * sizeof(SavedChannelView);

        ChunkHeader ch = {};
        strncpy(ch.id, "VIEW", sizeof(ch.id) - 1);
        ch.version = 1;
        ch.size = viewChunkSize;

        if(!writeBytes(f, &ch, sizeof(ch)) ||
        !writeBytes(f, &capture.viewState.firstSample, sizeof(capture.viewState.firstSample)) ||
        !writeBytes(f, &capture.viewState.samplesPerPixel, sizeof(capture.viewState.samplesPerPixel)) ||
        !writeBytes(f, &capture.viewState.cursorSample, sizeof(capture.viewState.cursorSample)) ||
        !writeBytes(f, &capture.viewState.markerA, sizeof(capture.viewState.markerA)) ||
        !writeBytes(f, &capture.viewState.markerB, sizeof(capture.viewState.markerB)) ||
        !writeBytes(f, &capture.viewState.markerASet, sizeof(capture.viewState.markerASet)) ||
        !writeBytes(f, &capture.viewState.markerBSet, sizeof(capture.viewState.markerBSet)) ||
        !writeBytes(f, &capture.viewState.selectedChannel, sizeof(capture.viewState.selectedChannel)) ||
        !writeBytes(f, &channelCount, sizeof(channelCount))) {
            fclose(f);
            return false;
        }

        for(const auto& c : capture.viewState.channels) {
            SavedChannelView sc = {};
            sc.visible = c.visible ? 1 : 0;
            sc.minimised = c.minimised ? 1 : 0;
            sc.protocolType = static_cast<uint32_t>(c.protocol.type);
            sc.protocolParameter = c.protocol.parameter;

            if(!writeBytes(f, &sc, sizeof(sc))) {
                fclose(f);
                return false;
            }
        }
    }

    /*
        EVENTS chunk format:
            sampleCount
            channelCount
            sampleRateHz
            event count
            event data
    */

    {
        uint32_t eventCount = capture.events.events.size();

        uint32_t eventChunkSize =
            sizeof(capture.events.sampleCount) +
            sizeof(capture.events.channelCount) +
            sizeof(capture.events.sampleRateHz) +
            sizeof(eventCount) +
            eventCount * sizeof(LogicEvent);

        ChunkHeader ch = {};
        strncpy(ch.id, "EVENTS", sizeof(ch.id) - 1);
        ch.version = 1;
        ch.size = eventChunkSize;

        if(!writeBytes(f, &ch, sizeof(ch)) ||
           !writeBytes(f, &capture.events.sampleCount, sizeof(capture.events.sampleCount)) ||
           !writeBytes(f, &capture.events.channelCount, sizeof(capture.events.channelCount)) ||
           !writeBytes(f, &capture.events.sampleRateHz, sizeof(capture.events.sampleRateHz)) ||
           !writeBytes(f, &eventCount, sizeof(eventCount)) ||
           !writeBytes(f, capture.events.events.data(), eventCount * sizeof(LogicEvent))) {
            fclose(f);
            return false;
        }
    }

    fclose(f);
    return true;
}

bool loadCaptureFile(const char* path, CaptureFile& capture)
{
    FILE* f = fopen(path, "rb");
    if(!f) return false;

    FileHeader header = {};
    if(!readBytes(f, &header, sizeof(header))) {
        fclose(f);
        return false;
    }

    if(memcmp(header.magic, CAP_MAGIC, sizeof(header.magic)) != 0 ||
       header.version != CAP_VERSION) {
        fclose(f);
        return false;
    }

    capture = CaptureFile{};
    ChunkHeader ch = {};

    while(readChunkHeader(f, ch)) {
        if(strncmp(ch.id, "META", 8) == 0) {
            if(ch.size != sizeof(CaptureMetadata)) {
                fclose(f);
                return false;
            }

            if(!readBytes(f, &capture.metadata, sizeof(CaptureMetadata))) {
                fclose(f);
                return false;
            }
        }     

        else if(strncmp(ch.id, "RAW", 8) == 0) {
            uint32_t sampleVecCount = 0;
            uint32_t timestampVecCount = 0;

            if(!readBytes(f, &capture.raw.sampleCount, sizeof(capture.raw.sampleCount)) ||
               !readBytes(f, &capture.raw.channelCount, sizeof(capture.raw.channelCount)) ||
               !readBytes(f, &capture.raw.mode, sizeof(capture.raw.mode)) ||
               !readBytes(f, &sampleVecCount, sizeof(sampleVecCount))) {
                fclose(f);
                return false;
            }

            capture.raw.samples.resize(sampleVecCount);

            if(sampleVecCount > 0 &&
               !readBytes(f, capture.raw.samples.data(), sampleVecCount * sizeof(uint32_t))) {
                fclose(f);
                return false;
            }

            if(!readBytes(f, &timestampVecCount, sizeof(timestampVecCount))) {
                fclose(f);
                return false;
            }

            capture.raw.timestamps.resize(timestampVecCount);

            if(timestampVecCount > 0 &&
               !readBytes(f, capture.raw.timestamps.data(), timestampVecCount * sizeof(uint32_t))) {
                fclose(f);
                return false;
            }
        }
        else if(strncmp(ch.id, "EVENTS", 8) == 0) {
            uint32_t eventCount = 0;

            if(!readBytes(f, &capture.events.sampleCount, sizeof(capture.events.sampleCount)) ||
               !readBytes(f, &capture.events.channelCount, sizeof(capture.events.channelCount)) ||
               !readBytes(f, &capture.events.sampleRateHz, sizeof(capture.events.sampleRateHz)) ||
               !readBytes(f, &eventCount, sizeof(eventCount))) {
                fclose(f);
                return false;
            }

            capture.events.events.resize(eventCount);

            if(eventCount > 0 &&
               !readBytes(f, capture.events.events.data(), eventCount * sizeof(LogicEvent))) {
                fclose(f);
                return false;
            }
        }
        else if(strncmp(ch.id, "VIEW", 8) == 0) {
            uint32_t channelCount = 0;

            if(!readBytes(f, &capture.viewState.firstSample, sizeof(capture.viewState.firstSample)) ||
            !readBytes(f, &capture.viewState.samplesPerPixel, sizeof(capture.viewState.samplesPerPixel)) ||
            !readBytes(f, &capture.viewState.cursorSample, sizeof(capture.viewState.cursorSample)) ||
            !readBytes(f, &capture.viewState.markerA, sizeof(capture.viewState.markerA)) ||
            !readBytes(f, &capture.viewState.markerB, sizeof(capture.viewState.markerB)) ||
            !readBytes(f, &capture.viewState.markerASet, sizeof(capture.viewState.markerASet)) ||
            !readBytes(f, &capture.viewState.markerBSet, sizeof(capture.viewState.markerBSet)) ||
            !readBytes(f, &capture.viewState.selectedChannel, sizeof(capture.viewState.selectedChannel)) ||
            !readBytes(f, &channelCount, sizeof(channelCount))) {
                fclose(f);
                return false;
            }

            capture.viewState.channels.resize(channelCount);

            for(uint32_t i = 0; i < channelCount; ++i) {
                SavedChannelView sc = {};

                if(!readBytes(f, &sc, sizeof(sc))) {
                    fclose(f);
                    return false;
                }

                capture.viewState.channels[i].visible = sc.visible != 0;
                capture.viewState.channels[i].minimised = sc.minimised != 0;
                capture.viewState.channels[i].protocol.type =
                    static_cast<ProtocolType>(sc.protocolType);
                capture.viewState.channels[i].protocol.parameter =
                    sc.protocolParameter;
            }
        }

        else {
            // Unknown future chunk: skip it.
            fseek(f, ch.size, SEEK_CUR);
        }
    }

    fclose(f);
    return true;
}