#include "spi.h"

#include <cstdio>
#include <cstring>

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <linux/spi/spidev.h>

SPI::SPI() : fd(-1) {
}

SPI::~SPI() {
    if (fd >= 0)
        close(fd);
}

bool SPI::openDevice(const char* device, uint32_t spd) {

    speed = spd;
    fd = ::open(device, O_WRONLY);

    if (fd < 0) {
        perror("SPI open");
        return false;
    }

    uint8_t mode = SPI_MODE_0;
    uint8_t bits = 8;

    ioctl(fd, SPI_IOC_WR_MODE, &mode);
    ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
    ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);

    return true;
}

bool SPI::write(const uint8_t* data, size_t len) {
	return ::write(fd, data, len) == (ssize_t)len;
    //ssize_t result = ::write(fd, data, len);
    //return result == (ssize_t)len;
}

bool SPI::transfer(const uint8_t* data, size_t len)
{
    const size_t MAX_CHUNK = 4096;   // safe spidev limit

    size_t offset = 0;

    while (offset < len) {

        spi_ioc_transfer tr = {};
        size_t chunk = MAX_CHUNK;

        if (offset + chunk > len)
            chunk = len - offset;

        tr.tx_buf = (unsigned long)(data + offset);
        tr.rx_buf = 0;
        tr.len = chunk;
        tr.speed_hz = speed;
        tr.bits_per_word = 8;

        int ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
        if (ret < 1) {
            perror("SPI transfer");
            return false;
        }

        offset += chunk;
    }

    return true;
}