#include <unistd.h>
#include <vector>
#include <cstdio>
#include <stdint.h>
#include <string>
#include "../terminal/screenbuffer.h"
#include "../gfx/renderer.h"
#include "ili9488.h"
#include "drivers/cardkb.h"
#include "gfx/font8x16.h"
#include "gfx/palette.h"
#include "../../drivers/gusman_logic.h"
#include "../../logic/logic_view.h"
#include "../../logic/logic_events.h"
#include "../../logic/pwm_measure.h"
#include "../../logic/protocols/protocol_candidate.h"
#include "../../logic/protocols/protocol_registry.h"
#include "../../logic/capture_file.h"
#include "../../logic/export/export_csv.h"

#define KEY_ESC             (0xB1)
#define KEY_UP              (0xB5)
#define KEY_DOWN            (0xB6)
#define KEY_LEFT            (0xB4)
#define KEY_RIGHT           (0xB7)
#define KEY_BACKSPACE       (0x08)
#define KEY_CTRL_x          (0xA7)
#define KEY_CTRL_o          (0x95)
#define KEY_CTRL_c          (0xA8)
#define KEY_CTRL_ESC        (0x80)
#define KEY_CTRL_UP         (0x99)
#define KEY_CTRL_DOWN       (0xA4)
#define KEY_CTRL_ENTER      (0xA3)
#define KEY_ENTER           (0x0D)

constexpr uint32_t WAVE_W = 408;
enum class ExportFormat {
    Native,
    CsvTransitions,
    CsvBusEvents,
    CsvSamples,
    Vcd
};

struct ExportUi {
    int selected = 0;
    ExportFormat format = ExportFormat::CsvTransitions;
};

enum class AppMode {
    View,
    Settings,
    Export
};

static const char* exportFormatName(ExportFormat f)
{
    switch(f) {
        case ExportFormat::Native:         return "Native .la01";
        case ExportFormat::CsvTransitions: return "CSV transitions";
        case ExportFormat::CsvBusEvents:   return "CSV bus events";
        case ExportFormat::CsvSamples:     return "CSV samples";
        case ExportFormat::Vcd:            return "VCD";
    }

    return "?";
}

static constexpr ExportFormat EXPORT_FORMATS[] = {
    ExportFormat::Native,
    ExportFormat::CsvTransitions,
    ExportFormat::CsvBusEvents,
    ExportFormat::CsvSamples,
    ExportFormat::Vcd
};

struct ProtocolSettings {
    bool autoAnalyse = true;

    bool pwmEnabled = true;
    bool uartEnabled = true;
};

struct CaptureSettings {
    uint32_t sampleRateHz = 100000;
    uint32_t postSamples  = 2048;
    uint32_t uartBaud     = 9600;
    ProtocolSettings protocol;
};


struct SettingsUi {
    int selected = 0;
};

static constexpr uint32_t SAMPLE_RATES[] = {
    40000, 100000, 250000, 500000, 1000000, 2000000
};

static constexpr uint32_t SAMPLE_COUNTS[] = {
    1024, 2048, 4096, 8192, 16384
};

static constexpr uint32_t UART_BAUDS[] = {
    9600, 19200, 38400, 57600, 115200
};

template<typename T, size_t N>
static int indexOf(const T (&arr)[N], T value)
{
    for(size_t i = 0; i < N; ++i) {
        if(arr[i] == value)
            return int(i);
    }

    return 0;
}

template<typename T, size_t N>
static T changeValue(const T (&arr)[N], T current, int delta)
{
    int idx = indexOf(arr, current);
    idx += delta;

    if(idx < 0)
        idx = 0;

    if(idx >= int(N))
        idx = int(N) - 1;

    return arr[idx];
}

static bool performExport(
    ExportFormat format,
    const GusmanCapture& cap,
    const LogicEventCapture& events,
    const LogicViewState& viewState,
    const CaptureSettings& settings)
{
    switch(format) {
        case ExportFormat::Native:
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
                "~/captures/export.la01",
                file);
        }

        case ExportFormat::CsvTransitions:
            return exportCaptureCsv(
                "~/captures/transitions_long.csv",
                events,
                &cap,
                CsvExportFormat::ChannelTransitionsLong);

        case ExportFormat::CsvBusEvents:
            return exportCaptureCsv(
                "~/captures/bus_events.csv",
                events,
                &cap,
                CsvExportFormat::EventBusTransitions);

        case ExportFormat::CsvSamples:
            return exportCaptureCsv(
                "~/captures/samples_wide.csv",
                events,
                &cap,
                CsvExportFormat::SamplesWide);

        case ExportFormat::Vcd:
            // Add exportCaptureVcd() later.
            return false;
    }

    return false;
}
static bool bitAt(const GusmanCapture& cap, uint32_t sample, uint8_t ch)
{
    return (cap.samples[sample] >> ch) & 1u;
}

static void clampViewState(LogicViewState& s,
                           const LogicEventCapture& cap)
{
    if(cap.sampleCount == 0)
        return;

    if(s.cursorSample >= cap.sampleCount)
        s.cursorSample = cap.sampleCount - 1;

    if(s.firstSample >= cap.sampleCount)
        s.firstSample = cap.sampleCount - 1;

    if(s.samplesPerPixel == 0) s.samplesPerPixel = 1;

    if(s.selectedChannel >= cap.channelCount)
        s.selectedChannel = cap.channelCount - 1;
}

static void centreCursor(LogicViewState& s, const LogicEventCapture& cap)
{
    constexpr uint32_t WAVE_W = 408;

    uint32_t visibleSamples = WAVE_W * s.samplesPerPixel;

    if(s.cursorSample > visibleSamples / 2)
        s.firstSample = s.cursorSample - visibleSamples / 2;
    else
        s.firstSample = 0;

    if(s.firstSample >= cap.sampleCount)
        s.firstSample = 0;
}

void ensureCursorVisible(LogicViewState& state)
{
    uint32_t visibleSamples =  WAVE_W * state.samplesPerPixel;

    if(state.cursorSample < state.firstSample)
    {
        state.firstSample = state.cursorSample;
    }

    if(state.cursorSample >=
       state.firstSample + visibleSamples)
    {
        state.firstSample =
            state.cursorSample -
            (visibleSamples / 2);
    }
}

static void drawSettingsOverlay(
    ScreenBuffer& screen,
    const CaptureSettings& settings,
    const SettingsUi& ui)
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

    screen.drawText(64, 56, "CAPTURE SETTINGS", COL_TEXT);

    char line[64];

    std::snprintf(
        line,
        sizeof(line),
        "%c Sample rate: %u Hz",
        ui.selected == 0 ? '>' : ' ',
        settings.sampleRateHz);
    screen.drawText(64, 88, line, ui.selected == 0 ? COL_SELECT : COL_TEXT);

    std::snprintf(
        line,
        sizeof(line),
        "%c Samples:     %u",
        ui.selected == 1 ? '>' : ' ',
        settings.postSamples);
    screen.drawText(64, 112, line, ui.selected == 1 ? COL_SELECT : COL_TEXT);

    std::snprintf(
        line,
        sizeof(line),
        "%c UART baud:   %u",
        ui.selected == 2 ? '>' : ' ',
        settings.uartBaud);
    screen.drawText(64, 136, line, ui.selected == 2 ? COL_SELECT : COL_TEXT);

    screen.drawText(64, 184, "Up/Down select", COL_TEXT);
    screen.drawText(64, 200, "Left/Right change", COL_TEXT);
    screen.drawText(64, 216, "Enter recapture", COL_TEXT);
    screen.drawText(64, 232, "Esc cancel", COL_TEXT);
}
static void drawExportOverlay(
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
int main()
{
    bool captureRequested = false;
    bool redrawRequested = true;
    LogicViewState viewState;
ExportUi exportUi;    
    viewState.firstSample = 0;
    viewState.cursorSample = 0;
    viewState.selectedChannel = 0;
    AppMode appMode = AppMode::View;
    CaptureSettings settings;
    CaptureSettings editSettings;
    SettingsUi settingsUi;
std::vector<ProtocolCandidate> protocolCandidates;
ProtocolRegistry protocolRegistry;

    printf("Starting Logic Analyser\n");
    bool running = true;

    ILI9488 lcd;
    if(!lcd.init())
    {
        printf("LCD init failed\n");
        return 1;
    }
    
    ScreenBuffer screen;
    Renderer renderer(lcd, screen);
    CardKB kb;
    initFont();
    if(!kb.init())
    {
        printf("Failed to initialize keyboard\n");
        return 1;
    }

    screen.clear(0);

//--------------------
    GusmanLogicAnalyzer la;

    if(!la.openDevice("/dev/ttyACM0")) {
        std::printf("open failed: %s\n", la.lastError().c_str());
        return 1;
    }

    GusmanCaptureRequest req;
    for(int i = 0; i < 24; ++i)
        req.channels[i] = i;
    req.triggerType = 3; // blast
    req.trigger = 0;
    req.inverted = 1;    // try 0 if this fails
    req.channelCount = 8;
    req.captureMode = GusmanCaptureMode::Mode8;
    req.frequency = settings.sampleRateHz;
    req.postSamples = settings.postSamples;
    req.preSamples = 0;
    req.loopCount = 0;
    req.measure = 0;   
    std::printf("requesting capture...\n");
    if(!la.requestCapture(req)) {
        std::printf("request failed: %s\n", la.lastError().c_str());
        return 1;
    }
    GusmanCapture cap;
    if(!la.readCapture(cap, req.captureMode, req.channelCount)) {
        std::printf("read failed: %s\n", la.lastError().c_str());
        return 1;
    }
    std::printf("capture received\n");
    std::printf("samples: %u\n", cap.sampleCount);
    std::printf("channels: %u\n", cap.channelCount);
    std::printf("timestamps: %zu\n", cap.timestamps.size());
    uint32_t maxPrint = cap.sampleCount < 32 ? cap.sampleCount : 32;

    LogicEventCapture events = buildLogicEvents(cap, req.frequency);

//-------------

    protocolCandidates.clear();

    if(settings.protocol.autoAnalyse) {
        protocolRegistry.analyse(
            events,
            protocolCandidates);
    }

    viewState.firstSample = 0;
    viewState.cursorSample = 0;
    viewState.samplesPerPixel = (events.sampleCount + WAVE_W - 1) / WAVE_W;
    if(viewState.samplesPerPixel < 1) viewState.samplesPerPixel = 1;
    viewState.channels.clear();
    viewState.channels.resize(events.channelCount);
    viewState.selectedChannel = 0;
    std::printf("events: %zu\n", events.events.size());
    LogicView view;
    while(running)
    {
        uint8_t key;

        if(captureRequested)
        {
            captureRequested = false;
            std::printf("requesting capture...\n");

            req.frequency = settings.sampleRateHz;
            req.postSamples = settings.postSamples;
            if(!la.requestCapture(req)) {
                std::printf("request failed: %s\n", la.lastError().c_str());
                return 1;
            }
            if(!la.readCapture(cap, req.captureMode, req.channelCount)) {
                std::printf("read failed: %s\n", la.lastError().c_str());
                return 1;
            }
            std::printf("capture received\n");
            std::printf("samples: %u\n", cap.sampleCount);
               //capture = analyser.capture(req);

            events = buildLogicEvents(cap, settings.sampleRateHz);

            protocolCandidates.clear();

            if(settings.protocol.autoAnalyse) {
                protocolRegistry.analyse(
                    events,
                    protocolCandidates);
            }

            viewState.firstSample = 0;
            viewState.cursorSample = 0;
            viewState.markerASet = false;
            viewState.markerBSet = false;

            viewState.samplesPerPixel =  (events.sampleCount + WAVE_W - 1) / WAVE_W;
            if(viewState.samplesPerPixel < 1) viewState.samplesPerPixel = 1;

            viewState.channels.clear();
            viewState.channels.resize(events.channelCount);
            clampViewState(viewState, events);

            redrawRequested = true;

        }

        if(kb.poll(key))
        {
            printf("key:0x%02X\n", key);
if(appMode == AppMode::Export) {
    switch(key) {
        case KEY_LEFT:
            exportUi.format =
                changeValue(EXPORT_FORMATS, exportUi.format, -1);
            break;

        case KEY_RIGHT:
            exportUi.format =
                changeValue(EXPORT_FORMATS, exportUi.format, 1);
            break;

        case KEY_ENTER:
        {
            bool ok = performExport(
                exportUi.format,
                cap,
                events,
                viewState,
                settings);

            std::printf(
                "export %s: %s\n",
                exportFormatName(exportUi.format),
                ok ? "ok" : "failed");

            appMode = AppMode::View;
            break;
        }

        case KEY_ESC:
        case 'e':
            appMode = AppMode::View;
            break;
    }

    redrawRequested = true;
    continue;
}

            if(appMode == AppMode::Settings) {
                switch(key) {
                    case KEY_UP:
                        if(settingsUi.selected > 0)
                            settingsUi.selected--;
                        break;

                    case KEY_DOWN:
                        if(settingsUi.selected < 2)
                            settingsUi.selected++;
                        break;

                    case KEY_LEFT:
                        if(settingsUi.selected == 0) {
                            editSettings.sampleRateHz =
                                changeValue(SAMPLE_RATES, editSettings.sampleRateHz, -1);
                        }
                        else if(settingsUi.selected == 1) {
                            editSettings.postSamples =
                                changeValue(SAMPLE_COUNTS, editSettings.postSamples, -1);
                        }
                        else if(settingsUi.selected == 2) {
                            editSettings.uartBaud =
                                changeValue(UART_BAUDS, editSettings.uartBaud, -1);
                        }
                        break;

                    case KEY_RIGHT:
                        if(settingsUi.selected == 0) {
                            editSettings.sampleRateHz =
                                changeValue(SAMPLE_RATES, editSettings.sampleRateHz, 1);
                        }
                        else if(settingsUi.selected == 1) {
                            editSettings.postSamples =
                                changeValue(SAMPLE_COUNTS, editSettings.postSamples, 1);
                        }
                        else if(settingsUi.selected == 2) {
                            editSettings.uartBaud =
                                changeValue(UART_BAUDS, editSettings.uartBaud, 1);
                        }
                        break;

                    case KEY_ENTER:
                        settings = editSettings;
                        appMode = AppMode::View;
                        captureRequested = true;
                        break;

                    case 's':
                        appMode = AppMode::View;
                        break;
                }

                redrawRequested = true;
                continue;
            }

            switch(key)
            {
case 'e':
    exportUi.selected = 0;
    exportUi.format = ExportFormat::CsvTransitions;
    appMode = AppMode::Export;
    redrawRequested = true;
    break;                
                case '+':
                case 'f':
                printf("Zoom In\n");
                    if(viewState.samplesPerPixel > 1) viewState.samplesPerPixel /= 2;
                    centreCursor(viewState, events);
                    break;

                case '-':
                case 'g':
                printf("Zoom Out\n");
                    viewState.samplesPerPixel *= 2;
                    centreCursor(viewState, events);
                    break;

                case 'n':
                case 'N':
                {
                    printf("Next event on channel %u\n", viewState.selectedChannel);
                    int idx = findNextChannelEvent(
                        events,
                        viewState.cursorSample,
                        viewState.selectedChannel
                    );
                    printf("idx: %d\n", idx);
                    if(idx >= 0) {
                        viewState.cursorSample = events.events[size_t(idx)].sampleIndex;
                        centreCursor(viewState, events);
                    }

                    break;
                }

                case 'p':
                case 'P':
                {
                    printf("Previous event on channel %u\n", viewState.selectedChannel);
                    int idx = findPrevChannelEvent(
                        events,
                        viewState.cursorSample,
                        viewState.selectedChannel
                    );

                    if(idx >= 0) {
                        printf("Event at sample %u\n", events.events[size_t(idx)].sampleIndex);
                        viewState.cursorSample = events.events[size_t(idx)].sampleIndex;
                        centreCursor(viewState, events);
                    }

                    break;
                }

                case 'a':
                case 'A':
                printf("Set marker A at sample %u\n", viewState.cursorSample);
                    viewState.markerA = viewState.cursorSample;
                    viewState.markerASet = true;
                    break;

                case 'b':
                case 'B':
                    viewState.markerB = viewState.cursorSample;
                    viewState.markerBSet = true;
                    printf("Set marker B at sample %u\n", viewState.cursorSample);
                    break;
                case 'c':
                case 'C':
                {
                    viewState.markerASet = false;
                    viewState.markerBSet = false;
                    break;
                }                    
                case 'h':
                case 'H':
                {
                    auto& ch = viewState.channels[viewState.selectedChannel];
                    ch.visible = !ch.visible;
                    break;
                }

                case 'r':
                case 'R':
                    captureRequested = true;
                    viewState.markerASet = false;
                    viewState.markerBSet = false;                    
                    break;
                case 'm':
                case 'M':
                {
                    PwmMeasurement pwm;

                    if(measurePwmAroundCursor(
                        events,
                        viewState.selectedChannel,
                        viewState.cursorSample,
                        pwm))
                    {
                        viewState.markerA = pwm.risingSample;
                        viewState.markerB = pwm.nextRisingSample;
                        viewState.markerASet = true;
                        viewState.markerBSet = true;

                        viewState.cursorSample = pwm.fallingSample;

                        ensureCursorVisible(viewState);
                    }

                    break;
                }
                case 's':
                case 'S':
                
                    editSettings = settings;
                    settingsUi.selected = 0;
                    appMode = AppMode::Settings;
                    redrawRequested = true;
                    break;                
                case KEY_UP:
                    if(viewState.selectedChannel > 0)
                        viewState.selectedChannel--;
                    break;

                case KEY_DOWN:
                    if(viewState.selectedChannel + 1 < events.channelCount)
                        viewState.selectedChannel++;
                    break;

                case KEY_LEFT:
                {
                    uint32_t step = viewState.samplesPerPixel * 40;
                    viewState.firstSample =
                        (viewState.firstSample > step) ? viewState.firstSample - step : 0;
                    break;
                }

                case KEY_RIGHT:
                {
                    uint32_t step = viewState.samplesPerPixel * 40;
                    uint32_t maxFirst =
                        events.sampleCount > 1 ? events.sampleCount - 1 : 0;

                    viewState.firstSample += step;
                    if(viewState.firstSample > maxFirst)
                        viewState.firstSample = maxFirst;
                    break;
                }
                case KEY_ENTER:
                {
                    auto& ch = viewState.channels[viewState.selectedChannel];
                    ch.minimised = !ch.minimised;
                    break;
                }
                case KEY_BACKSPACE:
                {
                    uint8_t bs = 0x7F;

                    break;
                }
                case KEY_CTRL_x:
                {
                    uint8_t c = 0x18; // Ctrl+x

                    break;
                }
                case KEY_CTRL_o:
                {
                    uint8_t c = 0x0F; // Ctrl+o
                    break;
                }
                case KEY_CTRL_c:
                {
                    uint8_t c = 0x03; // Ctrl+c
                    break;
                }
                case KEY_CTRL_ESC:
                {
                    //screen.clearScreen();
                    break;
                }  
                case KEY_CTRL_UP:
                    break;
                case KEY_CTRL_DOWN:
                    break;
                case KEY_CTRL_ENTER:
                    running = false;
                    break;
                default:
                    break;
            }

        }
        clampViewState(viewState, events);
        //view.draw(screen, events, viewState);

        view.draw(screen, events, viewState);

        if(appMode == AppMode::Settings) {
            drawSettingsOverlay(screen, editSettings, settingsUi);
        }
if(appMode == AppMode::Export) {
    view.draw(screen, events, viewState);
    drawExportOverlay(screen, exportUi);
}
        renderer.render();

        //usleep(3000); // about 33 FPS target
    }

    return 0;
}
