#ifndef __CRC_16_H
#define __CRC_16_H

#include <stdint.h>

uint16_t Crc16GetFromSeed(uint16_t seed, uint8_t* data, uint32_t len);
uint16_t Crc16Get(uint8_t* data, uint32_t len);

#endif
