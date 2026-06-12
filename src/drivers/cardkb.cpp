#include "cardkb.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <cstdio>
#include <linux/i2c-dev.h>

#define CARDKB_ADDR 0x5F

CardKB::CardKB()
    : m_fd(-1)
{
}

CardKB::~CardKB()
{
    // Close I2C

    // Example:
    // close(i2cFd);
}

bool CardKB::init(const char* device)
{
    m_fd = open(device, O_RDWR);
    if(m_fd < 0)
    {
        return false;
    }
    if(ioctl(m_fd, I2C_SLAVE, CARDKB_ADDR) < 0)
    {
        return false;
    }
    uint8_t data;
    int n = ::read(m_fd, &data, 1);    
    return true;
}

bool CardKB::poll(uint8_t& ch)
{
    uint8_t data;
    int n = ::read(m_fd, &data, 1);
    if(n != 1)
    {
        return false;
    }

    if(data == 0)
    {
        return false;
    }

    ch = data;

    return true;
}