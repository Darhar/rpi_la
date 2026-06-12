#pragma once

#include "logic_events.h"

#include <cstdint>

struct PwmMeasurement {
    bool valid = false;

    uint8_t channel = 0;

    uint32_t risingSample = 0;
    uint32_t fallingSample = 0;
    uint32_t nextRisingSample = 0;

    uint32_t highSamples = 0;
    uint32_t lowSamples = 0;
    uint32_t periodSamples = 0;

    double highUs = 0.0;
    double lowUs = 0.0;
    double periodUs = 0.0;

    double dutyPercent = 0.0;
    double frequencyHz = 0.0;
};

bool measurePwm(
    const LogicEventCapture& cap,
    uint8_t channel,
    uint32_t startSample,
    PwmMeasurement& out);

bool measurePwmAroundCursor(
    const LogicEventCapture& cap,
    uint8_t channel,
    uint32_t cursorSample,
    PwmMeasurement& out);    