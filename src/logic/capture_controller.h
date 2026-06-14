#pragma once

#include <cstdint>
#include <vector>

#include "../drivers/gusman_logic.h"
#include "logic_events.h"
#include "protocols/protocol_types.h"
#include "logic_view.h"

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

struct CaptureMarker {
    char name = 0;              // 'A', 'B', 'C'
    uint32_t sampleIndex = 0;
    bool valid = false;
};

struct CaptureChannelInfo {
    bool visible = true;
    bool minimised = false;

    ProtocolType protocol = ProtocolType::None;
    uint32_t protocolParameter = 0;
};

struct CaptureMetadata {
    uint32_t sampleRateHz = 0;
    uint32_t sampleCount = 0;
    uint8_t channelCount = 0;

    uint32_t preTriggerSamples = 0;
    uint32_t postTriggerSamples = 0;

    char name[64] = {};
    char notes[256] = {};
};

struct CaptureFile {
    CaptureMetadata metadata;

    GusmanCapture raw;
    LogicEventCapture events;

    LogicViewState viewState;
};



struct CaptureResult
{
    GusmanCapture raw;
    LogicEventCapture events;
};

bool performCapture(
    GusmanLogicAnalyzer& la,
    GusmanCaptureRequest& req,
    uint32_t sampleRateHz,
    CaptureResult& result
);

bool saveCaptureFile(const char* path, const CaptureFile& capture);
bool loadCaptureFile(const char* path, CaptureFile& capture);

//requestCapture()
//readCapture()
//buildLogicEvents()