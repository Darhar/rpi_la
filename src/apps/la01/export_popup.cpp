#include "export_popup.h"

const char* exportFormatName(ExportFormat fomt)
{
    switch(fomt){

        case ExportFormat::Raw:
            return "Raw";
        case ExportFormat::CsvRaw:
            return "CsvRaw";
        case ExportFormat::CsvTransitions:
            return "CsvTransitions";
        case ExportFormat::Vcd:
            return "Vcd";
        case ExportFormat::CsvBusEvents:
            return "CsvBusEvents";
        case ExportFormat::CsvSamples:
            return "CsvSamples";
        default:
            return "?";
    }

}

void drawExportOverlay(
    ScreenBuffer& screen,
    const ExportUi& ui)
{
    constexpr uint8_t COL_BG     = 0;
    constexpr uint8_t COL_TEXT   = 7;
    constexpr uint8_t COL_SELECT = 3;
    constexpr uint8_t COL_BOX    = 8;

    screen.fillRect(40, 40, 400, 220, COL_BG);

    screen.lineH(40, 439, 40, COL_BOX);
    screen.lineH(40, 439, 259, COL_BOX);
    screen.lineV(40, 40, 259, COL_BOX);
    screen.lineV(439, 40, 259, COL_BOX);

    screen.drawText(64, 56, "EXPORT CAPTURE", COL_TEXT);

    char line[96];

    std::snprintf(
        line,
        sizeof(line),
        "%c Format: %s",
        ui.selected == 0 ? '>' : ' ',
        exportFormatName(ui.format));

    screen.drawText(
        64,
        88,
        line,
        ui.selected == 0 ? COL_SELECT : COL_TEXT);

    screen.drawText(64, 136, "Enter export", COL_TEXT);
    screen.drawText(64, 152, "Left/Right format", COL_TEXT);
    screen.drawText(64, 168, "Esc cancel", COL_TEXT);

    screen.drawText(64, 208, "Files saved to:", COL_TEXT);
    screen.drawText(64, 224, "/home/darren/captures/", COL_TEXT);
}

bool performExport(
    ExportFormat format,
    const GusmanCapture& cap,
    const LogicEventCapture& events,
    const LogicViewState& viewState,
    const CaptureSettings& settings)
{

    switch(format) {
        case ExportFormat::Raw:
        {
            CaptureFile file = {};

            file.metadata.sampleRateHz = events.sampleRateHz;
            file.metadata.sampleCount = events.sampleCount;
            file.metadata.channelCount = events.channelCount;
            file.metadata.preTriggerSamples = 0;
            file.metadata.postTriggerSamples = settings.postSamples;

            std::strncpy(
                file.metadata.name,
                "logic capture",
                sizeof(file.metadata.name) - 1);

            std::strncpy(
                file.metadata.notes,
                "Saved from LA01",
                sizeof(file.metadata.notes) - 1);

            file.raw = cap;
            file.events = events;
            file.viewState = viewState;

            return saveCaptureFile(
                "/home/darren/captures/export.la01",
                file);
        }

        case ExportFormat::CsvTransitions:
            return exportCaptureCsv(
                "/home/darren/captures/transitions_long.csv",
                events,
                &cap,
                CsvExportFormat::ChannelTransitionsLong);

        case ExportFormat::CsvBusEvents:
            return exportCaptureCsv(
                "/home/darren/captures/bus_events.csv",
                events,
                &cap,
                CsvExportFormat::EventBusTransitions);

        case ExportFormat::CsvSamples:
            return exportCaptureCsv(
                "/home/darren/captures/samples_wide.csv",
                events,
                &cap,
                CsvExportFormat::SamplesWide);
        case ExportFormat::CsvRaw:
            // Add CsvRaw() later
            return false;

        case ExportFormat::Vcd:
            // Add exportCaptureVcd() later.
            return false;
    }

    return false;
}