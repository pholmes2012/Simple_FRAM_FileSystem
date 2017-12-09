/**************************************************************************/
/*!
    @file     io_driver_i2c.cpp
    @author   Paul Holmes
    @license  BSD (see LICENSE)

    Simple FRam File System

    @section  HISTORY

    v1.0 - First release
*/
/**************************************************************************/
#include "io_driver.h"
#include <Wire.h>

#define DEV_DBG

// This is the maximum number of bytes that can be received in one go (UNO)
#define MULTIBYTE_BLOCK_RX_LEN 32
// This is the maximum number of bytes that can be sent in one go (UNO)
#define MULTIBYTE_BLOCK_TX_LEN 30
// Page select bit (A16), MSB of 17 bit address
#define I2C_PAGE_BIT 0x01 


bool
cIO_DRV_I2C::Init(uint8 hwAddr)
{
	m_hwAddr = hwAddr;
	Wire.begin();
	return true;
}

void
cIO_DRV_I2C::_writeAddress(uint32 offset)
{
	Wire.write((uint8)(offset>>8));
	Wire.write((uint8)offset);
}


uint32
cIO_DRV_I2C::Read(uint32 offset, void* pBuf, uint32 byteCount)
{
	uint32 hasRead = 0;
	uint32 toRead = byteCount;

	// Read in <= 32 byte blocks
	while (toRead > 0)
	{
		uint32 addr = offset+hasRead;
		uint8 pageBit = (addr & 0x10000) ? I2C_PAGE_BIT : 0;
		uint8 block = (toRead > MULTIBYTE_BLOCK_RX_LEN) ? MULTIBYTE_BLOCK_RX_LEN : toRead;
		Wire.beginTransmission(m_hwAddr | pageBit);
		_writeAddress(addr);
		Wire.endTransmission();
		Wire.requestFrom(m_hwAddr, block);
		while (Wire.available())
		{
			((uint8*)pBuf)[hasRead++] = Wire.read();
			toRead--;
		}
	}
#ifdef DEV_DBG
	if (hasRead != byteCount)
	{
		// Something went wrong...
		Serial.print("I2C FRAM Read failed, ");
		Serial.print(byteCount);
		Serial.print(" requested but got ");
		Serial.println(hasRead);
	}
#endif
	return hasRead;
}

uint32
cIO_DRV_I2C::Write(uint32 offset, const void* pBuf, uint32 byteCount)
{
	uint32 hasWritten = 0;
	uint32 toWrite = byteCount;

	while (toWrite > 0)
	{
		uint32 addr = offset+hasWritten;
		uint8 pageBit = (addr & 0x10000) ? I2C_PAGE_BIT : 0;
		Wire.beginTransmission(m_hwAddr | pageBit);
		_writeAddress(addr);
		uint8 block = (toWrite > MULTIBYTE_BLOCK_TX_LEN) ? MULTIBYTE_BLOCK_TX_LEN : toWrite;
		uint8 done = Wire.write(&((uint8*)pBuf)[hasWritten], block);
		toWrite -= done;
		hasWritten += done;
		Wire.endTransmission();
	}
#ifdef DEV_DBG
	if (hasWritten != byteCount)
	{
		// Something went wrong...
		Serial.print("I2C FRAM Write failed, ");
		Serial.print(byteCount);
		Serial.print(" requested but wrote ");
		Serial.println(hasWritten);
	}
#endif
	return hasWritten;
}
