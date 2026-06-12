#pragma once

#include <cstdint>
#include <vector>
#include <string>

enum class GusmanCaptureMode : uint8_t {
    Mode8  = 0,
    Mode16 = 1,
    Mode24 = 2
};

struct GusmanCaptureRequest {
    uint8_t triggerType = 3;     // 0=edge, 1=complex, 2=fast, 3=blast
    uint8_t trigger = 0;
    uint8_t inverted = 0;
    uint16_t triggerValue = 0;

    uint8_t channels[24] = {};
    uint8_t channelCount = 8;

    uint32_t frequency = 1000000;
    uint32_t preSamples = 0;
    uint32_t postSamples = 32768;

    uint8_t loopCount = 0;
    uint8_t measure = 0;
    GusmanCaptureMode captureMode = GusmanCaptureMode::Mode8;
};

struct GusmanCapture {
    uint32_t sampleCount = 0;
    uint8_t channelCount = 0;
    GusmanCaptureMode mode = GusmanCaptureMode::Mode8;

    // Normalised: one uint32_t per sample.
    std::vector<uint32_t> samples;
    std::vector<uint32_t> timestamps;
};

class GusmanLogicAnalyzer {
public:
    GusmanLogicAnalyzer() = default;
    ~GusmanLogicAnalyzer();

    bool openDevice(const char* path);
    void closeDevice();

    bool requestCapture(const GusmanCaptureRequest& req);
    bool readCapture(GusmanCapture& out, GusmanCaptureMode mode, uint8_t channelCount);
    bool readU32LengthSkippingText(uint32_t& outCount);
    bool readLine(std::string& line, int timeoutMs);
    const std::string& lastError() const { return m_error; }

private:
    int m_fd = -1;
    std::string m_error;

    bool writeAll(const uint8_t* data, size_t len);
    bool readExact(uint8_t* data, size_t len, int timeoutMs);

    static uint16_t readU16LE(const uint8_t* p);
    static uint32_t readU32LE(const uint8_t* p);

    void setError(const std::string& e);
};