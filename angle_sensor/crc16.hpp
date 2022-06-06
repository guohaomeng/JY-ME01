#ifndef _CRC_H_
#define _CRC_H_

#include <stdint.h>
uint16_t Crc16(uint8_t *ptr, uint32_t len);

#define HIG_UINT16(a)   ( ((a)>>8) & 0xFF )     // 取uint16的高8位
#define LOW_UINT16(a)   ( (a) & 0xFF )          // 取uint16的低8位

#define HIG_UINT8(a)    ( ((a)>>4) & 0x0F )     // 取uint8的高4位
#define LOW_UINT8(a)    ( (a) & 0x0F )          // 取uint8的低4位


/* CRC16 余式表 */
static uint16_t crctalbeabs[] = { 
	0x0000, 0xCC01, 0xD801, 0x1400, 0xF001, 0x3C00, 0x2800, 0xE401, 
	0xA001, 0x6C00, 0x7800, 0xB401, 0x5000, 0x9C01, 0x8801, 0x4400 
};


/*!
 *  功  能: CRC16校验
 *  param1: 指向要校验的数据的指针
 *  param2: 要校验的数据的长度
 *  retval: 校验所得到的值，uint16_t 类型
 * 
 *  说  明: 本次CRC校验为查表法，多项式为 x16+x15+x2+1(0x8005)，CRC的初始值为0xFFFF
 */
uint16_t Crc16(uint8_t *ptr, uint32_t len) 
{
	uint16_t crc = 0xffff; 
	uint32_t i;
	uint8_t ch;
 
	for (i = 0; i < len; i++) {
		ch = *ptr++;
		crc = crctalbeabs[(ch ^ crc) & 15] ^ (crc >> 4);
		crc = crctalbeabs[((ch >> 4) ^ crc) & 15] ^ (crc >> 4);
	} 
	
	return crc;
}

#endif  /* crc.hpp */
