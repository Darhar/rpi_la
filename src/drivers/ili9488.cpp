#include "ili9488.h"

#include <unistd.h>
#include <cstdio>

//static constexpr int WIDTH  = 480;
//static constexpr int HEIGHT = 320;

static constexpr int FRAME_PIXELS = WIDTH * HEIGHT;
static constexpr int FRAME_BYTES  = FRAME_PIXELS * 2; // RGB565

//if the CHUNK_SIZE = 16 * 512 we get SPI transfer: Message too long
static constexpr int CHUNK_SIZE = 16 * 256;

// RGB565 framebuffer (MUCH faster than RGB888)
//static uint16_t framebuffer[WIDTH * HEIGHT];
static uint8_t framebuffer[WIDTH * HEIGHT * 3];

ILI9488::~ILI9488()
{
    // Release GPIO lines
    // Close SPI

    // Example only
    // gpiod_line_release(...);
    // close(spiFd);
}

void ILI9488::command(uint8_t cmd) {

    dc.set(false);
    spi.write(&cmd, 1);
}

void ILI9488::data(uint8_t value) {

    dc.set(true);
    spi.write(&value, 1);
}

void ILI9488::reset() {

    rst.set(true);
    usleep(10000);
    rst.set(false);
    usleep(10000);
    rst.set(true);
    usleep(200000);
}

bool ILI9488::init() {

    dc.init();
    rst.init();
    if (!spi.openDevice("/dev/spidev0.0", 10000000))
        return false;
    reset();

    // POSITIVE GAMMA
    command(0xE0);

    uint8_t e0[] = {
        0x00,0x03,0x09,0x08,0x16,
        0x0A,0x3F,0x78,0x4C,0x09,
        0x0A,0x08,0x16,0x1A,0x0F
    };

    for (auto v : e0) data(v);

    // NEGATIVE GAMMA
    command(0xE1);

    uint8_t e1[] = {
        0x00,0x16,0x19,0x03,0x0F,
        0x05,0x32,0x45,0x46,0x04,
        0x0E,0x0D,0x35,0x37,0x0F
    };

    for (auto v : e1) data(v);

    command(0xC0);// Power Control 1
    data(0x17);
    data(0x15);

    command(0xC1);// Power Control 2
    data(0x41);

    command(0xC5);// VCOM Control
    data(0x00);
    data(0x12);
    data(0x80);

    command(0x36);//TFT_MADCTL
	data(0x28);
	
    command(0x3A);// Pixel Interface Format
    #ifdef USERGB111
        data(0x01);//3 bits/pixel
    #else
        data(0x66);//18 bits/pixel
    #endif

    command(0xB0);// Interface Mode Control
    data(0x00);

    command(0xB1);// Frame Rate Control
    data(0xA0);

    command(0x20);//TFT_INVOFF

    command(0xB4);// Display Inversion Control
    data(0x02);

    command(0xB6);// Display Function Control
    data(0x02);
    data(0x02);
    data(0x3B);

    command(0xB7);// Entry Mode Set
    data(0xC6);

    command(0xE9);// Set Image Function, disable 24-bits Data Bus
    data(0x00);

    command(0xF7);// Adjust Control 3
    data(0xA9);
    data(0x51);
    data(0x2C);
    data(0x82);

    command(0x11);//TFT_SLPOUT

    usleep(120000);

    command(0x29);//TFT_DISPON

    usleep(120000);

    return true;
}

void ILI9488::fillColor(uint8_t r, uint8_t g, uint8_t b) {

    // COLUMN ADDRESS
    command(0x2A);//Column Address Set

    data(0x00);
    data(0x00);
    data((HEIGHT - 1) >> 8);
    data((HEIGHT - 1) & 0xFF);

    // PAGE ADDRESS
    command(0x2B);//Page Address Set

    data(0x00);
    data(0x00);
    data((WIDTH - 1) >> 8);
    data((WIDTH - 1) & 0xFF);

    // MEMORY WRITE
    command(0x2C);//Memory Write

    // ---------------- RGB565 conversion ----------------
    uint16_t color =
        ((r & 0xF8) << 8) |
        ((g & 0xFC) << 3) |
        (b >> 3);

    dc.set(true);

    for (int y = 0; y < HEIGHT; y++) {

        for (int x = 0; x < WIDTH; x++) {

            int idx = (y * WIDTH + x) * 3;

            framebuffer[idx + 0] = r;
            framebuffer[idx + 1] = g;
            framebuffer[idx + 2] = b;
        }
    }

    // Chunked SPI transfer (Linux safe)
    int offset = 0;
	int total = HEIGHT * WIDTH * 3;

	while (offset < total) {

		int chunk = CHUNK_SIZE;
		if (offset + chunk > total)
			chunk = total - offset;

		spi.transfer(framebuffer + offset, chunk);
		offset += chunk;
	}

}

void ILI9488::setWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    command(0x2A);

    // FORCE FULL 16-bit correctness
    data(x0 >> 8);
    data(x0 & 0xFF);
    data(x1 >> 8);
    data(x1 & 0xFF);

    command(0x2B);

    data(y0 >> 8);
    data(y0 & 0xFF);
    data(y1 >> 8);
    data(y1 & 0xFF);

    command(0x2C); // IMPORTANT: move GRAM write here temporarily
}


void ILI9488::writePixels(const uint8_t* data, size_t len)
{
    dc.set(true);
    spi.transfer(data, len);
}




