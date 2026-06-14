#pragma once

#include <cstdint>

struct ProtocolSettings {
    bool autoAnalyse = true;
    bool pwmEnabled = true;
    bool uartEnabled = true;
};

struct CaptureSettings {
    uint32_t sampleRateHz = 100000;
    uint32_t postSamples = 2048;
    uint32_t uartBaud = 9600;
    ProtocolSettings protocol;
};