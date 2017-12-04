/**************************************************************************/
/*!
    @file     io_driver_spi.cpp
    @author   Paul Holmes
    @license  BSD (see LICENSE)

    Simple FRam File System

    @section  HISTORY

    v1.0 - First release
*/
/**************************************************************************/
#include "io_driver.h"
#include <SPI.h>

#define SPI_CMD_READ   0x03  // Read
#define SPI_CMD_WRITE  0x02  // Write
#define SPI_CMD_WREN   0x06  // Write Enable
#define SPI_CMD_WRDI   0x04  // Reset write enable


bool
cIO_DRV_SPI::Init(uint8 csPin, uint8 addrWidth)
{
	m_csPin = csPin;
	m_addrWidth = addrWidth;

	pinMode(m_csPin, OUTPUT);
	digitalWrite(m_csPin, HIGH);

	uint8 div = SPI_CLOCK_DIV2;	// 8mhz on AVR
	#if defined(__SAM3X8E__)
    div = 9; // 9.3 MHz
	#elif defined(STM32F2XX)
	// Is seems the photon SPI0 clock runs at 60MHz, but SPI1 runs at
	// 30MHz, so the DIV will need to change if this is ever extended
	// to cover SPI1
	div = SPI_CLOCK_DIV4; // Adafruit WICED/Particle Photon SPI @ 15MHz
	#endif

	SPI.begin();
	SPI.setClockDivider(div);
	SPI.setDataMode(SPI_MODE0);

	return true;
}

void
cIO_DRV_SPI::_writeAddress(uint32 offset)
{
	if (m_addrWidth>3)
		SPI.transfer((uint8)(offset>>24));
	if (m_addrWidth>2)
		SPI.transfer((uint8)(offset>>16));
	SPI.transfer((uint8)(offset>>8));
	SPI.transfer((uint8)offset);
}

void 
cIO_DRV_SPI::_writeEnable(bool bEnable)
{
	digitalWrite(m_csPin, LOW);
	SPI.transfer((bEnable) ? SPI_CMD_WREN : SPI_CMD_WRDI);
	digitalWrite(m_csPin, HIGH);
}

uint32
cIO_DRV_SPI::Read(uint32 offset, uint8* pBuf, uint32 byteCount)
{
	digitalWrite(m_csPin, LOW);
	SPI.transfer(SPI_CMD_READ);
  	_writeAddress(offset);
  	for (uint32 i=0; i<byteCount; i++)
		pBuf[i] = SPI.transfer(0);
  	digitalWrite(m_csPin, HIGH);
	return byteCount;
}

uint32
cIO_DRV_SPI::Write(uint32 offset, const uint8* pBuf, uint32 byteCount)
{
	_writeEnable(true);
	digitalWrite(m_csPin, LOW);
	SPI.transfer(SPI_CMD_WRITE);
  	_writeAddress(offset);
	for (uint32 i=0; i<byteCount; i++)
		SPI.transfer(pBuf[i]);
	digitalWrite(m_csPin, HIGH);
	_writeEnable(false);
	
	return byteCount;
}
