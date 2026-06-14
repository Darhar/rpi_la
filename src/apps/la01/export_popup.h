#pragma once

#include "app_settings.h"
#include "../../drivers/gusman_logic.h"
#include "../../logic/logic_events.h"
#include "../../logic/logic_view.h"
#include "../../terminal/screenbuffer.h"
#include "../../logic/capture_controller.h"
#include "../../logic/export/export_csv.h"
#include <cstdint>
#include <cstddef>

enum class ExportFormat : uint8_t {
    Raw = 0,
    CsvRaw,
    CsvTransitions,
    CsvBusEvents,
    CsvSamples,
    Vcd
};

struct ExportUi {
    int selected = 0;
    ExportFormat format = ExportFormat::CsvTransitions;
};

static constexpr ExportFormat EXPORT_FORMATS[] = {
    ExportFormat::Raw,
    ExportFormat::CsvRaw,
    ExportFormat::CsvTransitions,
    ExportFormat::Vcd,
    ExportFormat::CsvBusEvents,
    ExportFormat::CsvSamples
};

const char* exportFormatName(ExportFormat f);

bool performExport(
    ExportFormat format,
    const GusmanCapture& raw,
    const LogicEventCapture& events,
    const LogicViewState& viewState,
    const CaptureSettings& settings
);

void drawExportOverlay(
    ScreenBuffer& screen,
    const ExportUi& ui
);