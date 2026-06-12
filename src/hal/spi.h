#pragma once

#include <cstdint>
#include <cstddef>

class SPI {
public:
    SPI();
    ~SPI();

    bool openDevice(const char* device, uint32_t speed);
    bool write(const uint8_t* data, size_t len);
	bool transfer(const uint8_t* data, size_t len);
private:
    int fd;
	uint32_t speed;
};