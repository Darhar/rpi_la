#pragma once

#include "protocol_types.h"

struct ProtocolCandidate {
    ProtocolType type = ProtocolType::None;

    uint8_t primaryChannel = 255;
    uint8_t secondaryChannel = 255;
    uint8_t clockChannel = 255;
    uint8_t enableChannel = 255;

    float confidence = 0.0f;

    uint32_t estimatedRate = 0;
};