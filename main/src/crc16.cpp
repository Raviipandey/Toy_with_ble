#include "crc16.h"

uint16_t Crc16GetFromSeed(uint16_t seed, uint8_t* data, uint32_t len)
{
  uint16_t crc;
  uint8_t i;
  crc = seed;
  while (len != 0)
  {
    crc ^= *data++;
    for (i = 0; i < 8; ++i)
    {
      if (crc & 1)
        crc = (crc >> 1) ^ 0xA001;
      else
        crc = (crc >> 1);
    }
    i = 0;
    len--;
  }
  return crc;
}

/**
    brief : Use this function to calculate 16 bit CRC

    param [in] data: data pointer
    param [in] len: no of bytes
    return: 16 bit CRC (modbus)

    details Details
*/
uint16_t Crc16Get(uint8_t* data, uint32_t len)
{
  return Crc16GetFromSeed(0xFFFF, data, len);
}
