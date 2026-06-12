#pragma once

#include "../logic_events.h"
#include "../../drivers/gusman_logic.h"

enum class CsvExportFormat {
    ChannelTransitionsLong,
    ChannelTransitionsWide,
    EventBusTransitions,
    SamplesWide
};

bool exportCaptureCsv(
    const char* path,
    const LogicEventCapture& events,
    const GusmanCapture* raw,
    CsvExportFormat format);
    