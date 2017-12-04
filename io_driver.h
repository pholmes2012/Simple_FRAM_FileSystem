/**************************************************************************/
/*!
    @file     io_driver.h
    @author   Paul Holmes

    @section LICENSE

	BSD 3-Clause License

	Copyright (c) 2017, Paul Holmes
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:

	* Redistributions of source code must retain the above copyright notice, this
	  list of conditions and the following disclaimer.

	* Redistributions in binary form must reproduce the above copyright notice,
	  this list of conditions and the following disclaimer in the documentation
	  and/or other materials provided with the distribution.

	* Neither the name of the copyright holder nor the names of its
	  contributors may be used to endorse or promote products derived from
	  this software without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
	AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
	IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
	DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
	FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
	DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
	SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
	CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
	OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
	OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/**************************************************************************/
#ifndef _io_driver_h
#define _io_driver_h

#include "Arduino.h"

#ifndef uint
#define uint unsigned int
#endif
#ifndef uint8
#define uint8 uint8_t
#endif
#ifndef uint16
#define uint16 uint16_t
#endif
#ifndef uint32
#define uint32 uint32_t
#endif
#ifndef int8
#define int8 int8_t
#endif
#ifndef int16
#define int16 int16_t
#endif
#ifndef int32
#define int32 int32_t
#endif

typedef union {
	uint32 Int32;
	uint8 Bytes[4];
}uAddress;


class cIO_DRV
{
private:
public:
	cIO_DRV()
	{
	}
	virtual uint32 Read(uint32 offset, uint8* pBuf, uint32 count) = 0;
	virtual uint32 Write(uint32 offset, const uint8* pBuf, uint32 count) = 0;
};


class cIO_DRV_SPI : public cIO_DRV
{
public:
	cIO_DRV_SPI() : cIO_DRV()
	{
	}
	bool Init(uint8 csPin, uint8 addrWidth);
	
	virtual uint32 Read(uint32 offset, uint8* pBuf, uint32 count);
	virtual uint32 Write(uint32 offset, const uint8* pBuf, uint32 count);
private:
	uint8 m_csPin;
	uint8 m_addrWidth;

	void _writeAddress(uint32 offset);
	void _writeEnable(bool bEnable);
};

#endif //_io_driver_h