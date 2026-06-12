#pragma once

#include "logic_events.h"

#include <cstdint>
#include <vector>

struct UartDecodedByte {
    uint32_t startSample = 0;
    uint32_t endSample = 0;
    uint8_t value = 0;
    bool framingOk = false;
};

struct UartDecodeResult {
    bool valid = false;
    uint8_t channel = 0;
    uint32_t baud = 0;
    uint32_t bitSamples = 0;
    std::vector<UartDecodedByte> bytes;
};

bool decodeUart8N1(
    const LogicEventCapture& cap,
    uint8_t channel,
    uint32_t baud,
    UartDecodeResult& out);