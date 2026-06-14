#include "la_app.h"

#include "gfx/font8x16.h"

#include <cstdio>
#include <cstdlib>
#include <unistd.h>

bool LaApp::init()
{
    std::printf("Starting Logic Analyser\n");

    initFont();

    m_lcd.init();

    m_renderer = std::make_unique<Renderer>(m_lcd, m_screen);

    if(!m_keyboard.init()) {
        std::printf("keyboard init failed\n");
        return false;
    }

    m_analyserAvailable = tryOpenAnalyser();

    for(int i = 0; i < 24; ++i)
        m_request.channels[i] = i;

    m_request.triggerType = 3;
    m_request.trigger = 0;
    m_request.inverted = 1;
    m_request.triggerValue = 0;

    m_request.channelCount = 8;
    m_request.captureMode = GusmanCaptureMode::Mode8;

    m_request.preSamples = 0;
    m_request.loopCount = 0;
    m_request.measure = 0;

    m_request.frequency = m_settings.sampleRateHz;
    m_request.postSamples = m_settings.postSamples;

    if(m_analyserAvailable) {
        capture();
    } else {
        initialiseEmptyCapture();
    }

        requestRedraw();
        return true;
}

int LaApp::run()
{
    while(m_running) {
        uint8_t key = 0;

        if(m_captureRequested) {
            m_captureRequested = false;
            capture();
            requestRedraw();
        }

        if(m_keyboard.poll(key)) {
            std::printf("key: 0x%02X\n", key);
            handleKey(key);
        }

        clampViewState(m_viewState, m_events);

        if(m_redrawRequested) {
            m_redrawRequested = false;
            redraw();
        }

        usleep(1000);
    }

    return 0;
}

bool LaApp::capture()
{

    if(!m_analyserAvailable) {
        m_analyserAvailable = tryOpenAnalyser();

        if(!m_analyserAvailable) {
            initialiseEmptyCapture();
            return false;
        }
    }

    std::printf("requesting capture...\n");

    m_request.frequency = m_settings.sampleRateHz;
    m_request.postSamples = m_settings.postSamples;

    if(!m_analyser.requestCapture(m_request)) {
        std::printf("request failed: %s\n",
                    m_analyser.lastError().c_str());
        m_analyserAvailable = false;
        initialiseEmptyCapture();
        return false;
    }

    if(!m_analyser.readCapture(
        m_rawCapture,
        m_request.captureMode,
        m_request.channelCount
    )) {
        std::printf("read failed: %s\n",
                    m_analyser.lastError().c_str());
        m_analyserAvailable = false;
        initialiseEmptyCapture();
        return false;
    }

    std::printf("capture received\n");
    std::printf("samples: %u\n", m_rawCapture.sampleCount);
    std::printf("channels: %u\n", m_rawCapture.channelCount);

    m_events = buildLogicEvents(
        m_rawCapture,
        m_settings.sampleRateHz
    );

    std::printf("events: %zu\n", m_events.events.size());

    m_protocolCandidates.clear();

    if(m_settings.protocol.autoAnalyse) {
        m_protocolRegistry.analyse(
            m_events,
            m_protocolCandidates
        );
    }

    resetViewAfterCapture();

    return true;
}

bool LaApp::tryOpenAnalyser()
{
    if(m_analyser.openDevice("/dev/ttyACM0")) {
        std::printf("analyser connected\n");
        return true;
    }

    std::printf(
        "analyser open failed: %s\n",
        m_analyser.lastError().c_str()
    );

    return false;
}

void LaApp::initView(){
    m_viewState.firstSample = 0;
    m_viewState.cursorSample = 0;
    m_viewState.markerA = 0;
    m_viewState.markerB = 0;
    m_viewState.markerASet = false;
    m_viewState.markerBSet = false;
    m_viewState.selectedChannel = 0;
    m_viewState.channels.clear();
    m_viewState.channels.resize(m_events.channelCount);    
}

void LaApp::initialiseEmptyCapture()
{
    m_rawCapture = GusmanCapture{};

    m_events = LogicEventCapture{};
    m_events.sampleCount = 0;
    m_events.channelCount = m_request.channelCount;
    m_events.sampleRateHz = m_settings.sampleRateHz;
    m_events.events.clear();
    m_events.channelEvents.clear();
    m_events.channelEvents.resize(m_events.channelCount);

    initView();
    m_viewState.samplesPerPixel = 1;

}

void LaApp::resetViewAfterCapture()
{

    initView();
    m_viewState.samplesPerPixel =
        (m_events.sampleCount + APP_WAVE_W - 1) / APP_WAVE_W;

    if(m_viewState.samplesPerPixel < 1)
        m_viewState.samplesPerPixel = 1;

    clampViewState(m_viewState, m_events);
}

void LaApp::redraw()
{
    m_view.draw(m_screen, m_events, m_viewState);

    if(m_appMode == AppMode::Settings) {
        drawSettingsOverlay(
            m_screen,
            m_editSettings,
            m_settingsUi
        );
    }

    if(m_appMode == AppMode::Export) {
        drawExportOverlay(
            m_screen,
            m_exportUi
        );
    }

    m_renderer->render();
}

void LaApp::requestRedraw()
{
    m_redrawRequested = true;
}

void LaApp::handleKey(uint8_t key)
{
    switch(m_appMode) {
        case AppMode::View:
            handleViewKey(key);
            break;

        case AppMode::Settings:
            handleSettingsKey(key);
            break;

        case AppMode::Export:
            handleExportKey(key);
            break;
    }

    clampViewState(m_viewState, m_events);
    requestRedraw();
}

void LaApp::handleViewKey(uint8_t key)
{
    switch(key) {
        case '+':
        case '=':
        case 'f':
        case 'F':
            if(m_viewState.samplesPerPixel > 1)
                m_viewState.samplesPerPixel /= 2;

            centreCursor(m_viewState, m_events);
            break;

        case '-':
        case '_':
        case 'g':
        case 'G':
            if(m_viewState.samplesPerPixel < 0x80000000u)
                m_viewState.samplesPerPixel *= 2;

            centreCursor(m_viewState, m_events);
            break;

        case KEY_LEFT:
        {
            uint32_t step = m_viewState.samplesPerPixel * 40;

            if(m_viewState.firstSample > step)
                m_viewState.firstSample -= step;
            else
                m_viewState.firstSample = 0;

            break;
        }

        case KEY_RIGHT:
        {
            uint32_t step = m_viewState.samplesPerPixel * 40;

            m_viewState.firstSample += step;

            if(m_events.sampleCount > 0 &&
               m_viewState.firstSample >= m_events.sampleCount) {
                m_viewState.firstSample = m_events.sampleCount - 1;
            }

            break;
        }

        case KEY_UP:
            if(m_viewState.selectedChannel > 0)
                m_viewState.selectedChannel--;
            break;

        case KEY_DOWN:
            if(m_viewState.selectedChannel + 1 < m_events.channelCount)
                m_viewState.selectedChannel++;
            break;

        case KEY_ENTER:
            if(m_viewState.selectedChannel <
               m_viewState.channels.size()) {
                auto& row =
                    m_viewState.channels[m_viewState.selectedChannel];

                row.minimised = !row.minimised;
            }
            break;

        case 'n':
        case 'N':
        {
            int idx = findNextChannelEvent(
                m_events,
                m_viewState.cursorSample,
                m_viewState.selectedChannel
            );

            if(idx >= 0) {
                m_viewState.cursorSample =
                    m_events.events[size_t(idx)].sampleIndex;

                centreCursor(m_viewState, m_events);
            }

            break;
        }

        case 'p':
        case 'P':
        {
            int idx = findPrevChannelEvent(
                m_events,
                m_viewState.cursorSample,
                m_viewState.selectedChannel
            );

            if(idx >= 0) {
                m_viewState.cursorSample =
                    m_events.events[size_t(idx)].sampleIndex;

                centreCursor(m_viewState, m_events);
            }

            break;
        }

        case 'a':
        case 'A':
            m_viewState.markerA = m_viewState.cursorSample;
            m_viewState.markerASet = true;
            break;

        case 'b':
        case 'B':
            m_viewState.markerB = m_viewState.cursorSample;
            m_viewState.markerBSet = true;
            break;

        case 'c':
        case 'C':
            m_viewState.markerASet = false;
            m_viewState.markerBSet = false;
            break;

        case 'r':
        case 'R':
        case ' ':
            m_captureRequested = true;
            break;

        case 's':
        case 'S':
            m_editSettings = m_settings;
            m_settingsUi.selected = 0;
            m_appMode = AppMode::Settings;
            break;

        case 'e':
        case 'E':
            m_exportUi.selected = 0;
            m_appMode = AppMode::Export;
            break;

        case KEY_CTRL_ENTER:
        case KEY_ESC:
            m_running = false;
            break;

        default:
            break;
    }
}

void LaApp::handleSettingsKey(uint8_t key)
{
    switch(key) {
        case KEY_UP:
            if(m_settingsUi.selected > 0)
                m_settingsUi.selected--;
            break;

        case KEY_DOWN:
            if(m_settingsUi.selected < 2)
                m_settingsUi.selected++;
            break;

        case KEY_LEFT:
            if(m_settingsUi.selected == 0) {
                m_editSettings.sampleRateHz =
                    changeValue(
                        SAMPLE_RATES,
                        m_editSettings.sampleRateHz,
                        -1
                    );
            } else if(m_settingsUi.selected == 1) {
                m_editSettings.postSamples =
                    changeValue(
                        SAMPLE_COUNTS,
                        m_editSettings.postSamples,
                        -1
                    );
            } else if(m_settingsUi.selected == 2) {
                m_editSettings.uartBaud =
                    changeValue(
                        UART_BAUDS,
                        m_editSettings.uartBaud,
                        -1
                    );
            }
            break;

        case KEY_RIGHT:
            if(m_settingsUi.selected == 0) {
                m_editSettings.sampleRateHz =
                    changeValue(
                        SAMPLE_RATES,
                        m_editSettings.sampleRateHz,
                        1
                    );
            } else if(m_settingsUi.selected == 1) {
                m_editSettings.postSamples =
                    changeValue(
                        SAMPLE_COUNTS,
                        m_editSettings.postSamples,
                        1
                    );
            } else if(m_settingsUi.selected == 2) {
                m_editSettings.uartBaud =
                    changeValue(
                        UART_BAUDS,
                        m_editSettings.uartBaud,
                        1
                    );
            }
            break;

        case KEY_ENTER:
            m_settings = m_editSettings;
            m_appMode = AppMode::View;
            m_captureRequested = true;
            break;

        case KEY_ESC:
        case 's':
        case 'S':
            m_appMode = AppMode::View;
            break;

        default:
            break;
    }
}

void LaApp::handleExportKey(uint8_t key)
{
    switch(key) {
        case KEY_LEFT:
            m_exportUi.format =
                changeValue(
                    EXPORT_FORMATS,
                    m_exportUi.format,
                    -1
                );
            break;

        case KEY_RIGHT:
            m_exportUi.format =
                changeValue(
                    EXPORT_FORMATS,
                    m_exportUi.format,
                    1
                );
            break;

        case KEY_ENTER:
        {
            bool ok = performExport(
                m_exportUi.format,
                m_rawCapture,
                m_events,
                m_viewState,
                m_settings
            );

            std::printf(
                "export %s: %s\n",
                exportFormatName(m_exportUi.format),
                ok ? "ok" : "failed"
            );

            m_appMode = AppMode::View;
            break;
        }

        case KEY_ESC:
        case 'e':
        case 'E':
            m_appMode = AppMode::View;
            break;

        default:
            break;
    }
}