#pragma once

#include "app_settings.h"
#include "../../terminal/screenbuffer.h"

#include <cstdint>
#include <cstddef>

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

void drawSettingsOverlay(
    ScreenBuffer& screen,
    const CaptureSettings& settings,
    const SettingsUi& ui
);