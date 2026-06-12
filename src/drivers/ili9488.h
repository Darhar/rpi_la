#pragma once

#include "../hal/spi.h"
#include "../hal/gpio.h"

#include <cstdint>
#include <cstddef>

static constexpr int WIDTH  = 480;
static constexpr int HEIGHT = 320;


class ILI9488 {
public:

    ~ILI9488();
    bool init();

    void fillColor(uint8_t r, uint8_t g, uint8_t b);

    void setWindow(
        uint16_t x0,
        uint16_t y0,
        uint16_t x1,
        uint16_t y1
    );

    void writePixels(
        const uint8_t* data,
        size_t len
    );

    void setScrollPosition(uint16_t scroll){
        command(0x37); // VERTICAL SCROLL START ADDRESS
        data(scroll >> 8);
        data(scroll & 0xFF);
    }   

    void vScrollDef(uint16_t tfa, uint16_t vsa, uint16_t bfa){
        command(0x33);
        data(tfa >> 8);
        data(tfa & 0xFF);
        data(vsa >> 8);
        data(vsa & 0xFF);
        data(bfa >> 8);
        data(bfa & 0xFF);
    }
    
private:

    SPI spi;

    GPIO dc{25};
    GPIO rst{17};

    void reset();
    void command(uint8_t cmd);
    void data(uint8_t value);
};