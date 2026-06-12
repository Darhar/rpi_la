#pragma once

#include <stdint.h>

class CardKB
{
public:
    CardKB();
    ~CardKB();
    bool init(const char* device = "/dev/i2c-1");
    bool poll(uint8_t& ch);

private:
    int m_fd;
};