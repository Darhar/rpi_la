#include "gusman_logic.h"

#include <array>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <termios.h>
#include <unistd.h>

static void putU16LE(uint8_t* p, uint16_t v)
{
    p[0] = uint8_t(v & 0xff);
    p[1] = uint8_t((v >> 8) & 0xff);
}

static void putU32LE(uint8_t* p, uint32_t v)
{
    p[0] = uint8_t(v & 0xff);
    p[1] = uint8_t((v >> 8) & 0xff);
    p[2] = uint8_t((v >> 16) & 0xff);
    p[3] = uint8_t((v >> 24) & 0xff);
}

static void appendEscaped(std::vector<uint8_t>& out, uint8_t b)
{
    if(b == 0xAA) {
        out.push_back(0xF0);
        out.push_back(0x5A);
    } else if(b == 0x55) {
        out.push_back(0xF0);
        out.push_back(0xA5);
    } else if(b == 0xF0) {
        out.push_back(0xF0);
        out.push_back(0x00);
    } else {
        out.push_back(b);
    }
}

static std::array<uint8_t, 48> buildCaptureRequestBytes(
    const GusmanCaptureRequest& c
) {
    std::array<uint8_t, 48> r{};

    r[0] = c.triggerType;
    r[1] = c.trigger;
    r[2] = c.inverted;
    r[3] = 0;

    putU16LE(&r[4], c.triggerValue);

    for(int i = 0; i < 24; ++i)
        r[6 + i] = c.channels[i];

    r[30] = c.channelCount;
    r[31] = 0;

    putU32LE(&r[32], c.frequency);
    putU32LE(&r[36], c.preSamples);
    putU32LE(&r[40], c.postSamples);

    r[44] = c.loopCount;
    r[45] = c.measure;
    r[46] = static_cast<uint8_t>(c.captureMode);
    r[47] = 0;

    return r;
}
bool GusmanLogicAnalyzer::readU32LengthSkippingText(uint32_t& outCount)
{
    uint8_t window[4] = {};

    while(true) {
        uint8_t b;

        if(!readExact(&b, 1, 10000))
            return false;

        window[0] = window[1];
        window[1] = window[2];
        window[2] = window[3];
        window[3] = b;

        uint32_t v = readU32LE(window);

        if(v > 0 && v <= 1024 * 1024) {
            outCount = v;
            return true;
        }

        // Debug useful while testing
        std::printf("skip byte: 0x%02X '%c'\n",
                    b,
                    (b >= 32 && b < 127) ? b : '.');
    }
}
static std::vector<uint8_t> buildCaptureFrame(const GusmanCaptureRequest& req)
{
    auto body = buildCaptureRequestBytes(req);

    std::vector<uint8_t> frame;

    frame.push_back(0x55);
    frame.push_back(0xAA);

    appendEscaped(frame, 0x01); // capture command

    for(uint8_t b : body)
        appendEscaped(frame, b);

    frame.push_back(0xAA);
    frame.push_back(0x55);

    return frame;
}

GusmanLogicAnalyzer::~GusmanLogicAnalyzer()
{
    closeDevice();
}

void GusmanLogicAnalyzer::setError(const std::string& e)
{
    m_error = e;
}

bool GusmanLogicAnalyzer::openDevice(const char* path)
{
    closeDevice();

    m_fd = ::open(path, O_RDWR | O_NOCTTY | O_SYNC);
    if(m_fd < 0) {
        setError(std::string("open failed: ") + std::strerror(errno));
        return false;
    }

    termios tty{};
    if(tcgetattr(m_fd, &tty) != 0) {
        setError(std::string("tcgetattr failed: ") + std::strerror(errno));
        closeDevice();
        return false;
    }

    cfmakeraw(&tty);

    // CDC usually ignores this, but set something sane.
    cfsetispeed(&tty, B115200);
    cfsetospeed(&tty, B115200);

    tty.c_cflag |= CLOCAL | CREAD;
    tty.c_cflag &= ~CRTSCTS;
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;

    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 0;

    if(tcsetattr(m_fd, TCSANOW, &tty) != 0) {
        setError(std::string("tcsetattr failed: ") + std::strerror(errno));
        closeDevice();
        return false;
    }

    tcflush(m_fd, TCIOFLUSH);
    return true;
}

void GusmanLogicAnalyzer::closeDevice()
{
    if(m_fd >= 0) {
        ::close(m_fd);
        m_fd = -1;
    }
}

bool GusmanLogicAnalyzer::writeAll(const uint8_t* data, size_t len)
{
    size_t done = 0;

    while(done < len) {
        ssize_t n = ::write(m_fd, data + done, len - done);
        if(n < 0) {
            if(errno == EINTR)
                continue;

            setError(std::string("write failed: ") + std::strerror(errno));
            return false;
        }

        done += size_t(n);
    }

    return true;
}

bool GusmanLogicAnalyzer::readExact(uint8_t* data, size_t len, int timeoutMs)
{
    size_t done = 0;

    while(done < len) {
        pollfd pfd{};
        pfd.fd = m_fd;
        pfd.events = POLLIN;

        int pr = ::poll(&pfd, 1, timeoutMs);
        if(pr == 0) {
            setError("read timeout");
            return false;
        }

        if(pr < 0) {
            if(errno == EINTR)
                continue;

            setError(std::string("poll failed: ") + std::strerror(errno));
            return false;
        }

        ssize_t n = ::read(m_fd, data + done, len - done);
        if(n < 0) {
            if(errno == EINTR)
                continue;

            setError(std::string("read failed: ") + std::strerror(errno));
            return false;
        }

        if(n == 0)
            continue;

        done += size_t(n);
    }

    return true;
}

uint16_t GusmanLogicAnalyzer::readU16LE(const uint8_t* p)
{
    return uint16_t(p[0]) | (uint16_t(p[1]) << 8);
}

uint32_t GusmanLogicAnalyzer::readU32LE(const uint8_t* p)
{
    return uint32_t(p[0])
         | (uint32_t(p[1]) << 8)
         | (uint32_t(p[2]) << 16)
         | (uint32_t(p[3]) << 24);
}

bool GusmanLogicAnalyzer::requestCapture(const GusmanCaptureRequest& req)
{
    if(m_fd < 0) {
        setError("device not open");
        return false;
    }

    auto frame = buildCaptureFrame(req);

    tcflush(m_fd, TCIOFLUSH);

    return writeAll(frame.data(), frame.size());
}

bool GusmanLogicAnalyzer::readLine(std::string& line, int timeoutMs)
{
    line.clear();

    while(true) {
        uint8_t b;

        if(!readExact(&b, 1, timeoutMs))
            return false;

        if(b == '\n')
            return true;

        if(b != '\r')
            line.push_back(char(b));
    }
}

bool GusmanLogicAnalyzer::readCapture(
    GusmanCapture& out,
    GusmanCaptureMode mode,
    uint8_t channelCount
) {
    if(m_fd < 0) {
        setError("device not open");
        return false;
    }

    std::string line;

    if(!readLine(line, 10000))
        return false;

    std::printf("status: %s\n", line.c_str());

    if(line != "CAPTURE_STARTED") {
        setError("unexpected status: " + line);
        return false;
    }

    uint8_t lenBytes[4];

    if(!readExact(lenBytes, 4, 120000))
        return false;

    uint32_t count = readU32LE(lenBytes);

    std::printf("sample count: %u\n", count);

    constexpr uint32_t MAX_REASONABLE_SAMPLES = 1024 * 1024;

    if(count == 0 || count > MAX_REASONABLE_SAMPLES) {
        setError("invalid sample count");
        return false;
    }

    out.sampleCount = count;
    out.channelCount = channelCount;
    out.mode = mode;
    out.samples.clear();
    out.timestamps.clear();
    out.samples.resize(count);

    switch(mode) {
        case GusmanCaptureMode::Mode8:
        {
            std::vector<uint8_t> buf(count);
            if(!readExact(buf.data(), buf.size(), 30000)) return false;
            for(uint32_t i = 0; i < count; ++i) out.samples[i] = buf[i];
            break;
        }

        case GusmanCaptureMode::Mode16:
        {
            std::vector<uint8_t> buf(size_t(count) * 2);
            if(!readExact(buf.data(), buf.size(), 30000)) return false;
            for(uint32_t i = 0; i < count; ++i) out.samples[i] = readU16LE(&buf[size_t(i) * 2]);
            break;
        }

        case GusmanCaptureMode::Mode24:
        {
            std::vector<uint8_t> buf(size_t(count) * 4);
            if(!readExact(buf.data(), buf.size(), 30000)) return false;
            for(uint32_t i = 0; i < count; ++i) out.samples[i] = readU32LE(&buf[size_t(i) * 4]) & 0x00ffffffu;
            break;
        }
    }

    uint8_t stampsLength = 0;

    if(!readExact(&stampsLength, 1, 5000))
        return false;

    if(stampsLength > 1) {
        std::vector<uint8_t> stampBytes(size_t(stampsLength) * 4);
        if(!readExact(stampBytes.data(), stampBytes.size(), 5000))  return false;
        out.timestamps.resize(stampsLength);
        for(uint8_t i = 0; i < stampsLength; ++i) out.timestamps[i] = readU32LE(&stampBytes[size_t(i) * 4]);
    }

    return true;
}