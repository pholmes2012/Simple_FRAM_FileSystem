/**************************************************************************/
/*!
    @file     SFFS.h
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
#ifndef _SFFS_H
#define _SFFS_H

#include "Arduino.h"
#include "Adafruit_FRAM_I2C.h"
#include "Adafruit_FRAM_SPI.h"

#define SFFS_MAGIC "SF01"
#define SFFS_MAGIC_INT (uint32)('S'<<24 | 'F'<<16 | '0'<<8 | '1')
#define SFFS_MAGIC_SIZE 4
#define SFFS_MAX_OPEN_FILES 4
#define SFFS_FILE_NAME_LEN 15

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
class SFFS_Volume;

typedef void* SFFS_FP;

class SFFS_Tools
{
private:
public:
	static bool strcpy(char* pDest, const char* pSrc, uint maxLen)
	{
		uint len = 0;
		while (len<maxLen-1 && pSrc[len] != '\0')
		{
			pDest[len] = pSrc[len];
			len++;
		}
		if (len<maxLen)
		{
			pDest[len] = '\0';
			return true;
		}
		return false;
	}
	static bool strcmp(char* pDest, const char* pSrc)
	{
		uint i=0;
		while (pDest[i]==pSrc[i])
		{
			if (pDest[i]=='\0')
			{
				return (i>0) ? true : false;
			}
			i++;
		}
		return false;
	}
};

//
// Base class for system I2C or SPI access
//
class SFFS_HW
{
public:
	typedef enum {
		eFRAM_HW_SPI=0,
		eFRAM_HW_I2C,
	}eFRAM_HW;
	virtual eFRAM_HW ID() = 0;
	virtual bool begin(uint8 beginParam) = 0;
	virtual bool read(uint32 addr, uint8* pBuf, uint32 count) = 0;

	virtual bool write(uint32 addr, uint8* pBuf, uint32 count) = 0;
};
//
// System I2C access
//
class SFFS_HW_I2C : public SFFS_HW
{
private:
	Adafruit_FRAM_I2C m_fram;
public:
	virtual eFRAM_HW ID() { return eFRAM_HW_I2C; };
	virtual bool begin(uint8 i2cAddr)
	{
		return m_fram.begin(i2cAddr);
	}
	virtual bool read(uint32 addr, uint8* pBuf, uint32 count)
	{
		return m_fram.read(addr, pBuf, count);
	}
	virtual bool write(uint32 addr, uint8* pBuf, uint32 count)
	{
		return m_fram.write(addr, pBuf, count);
	}
};
//
// System SPI access
//
class SFFS_HW_SPI : public SFFS_HW
{
private:
	Adafruit_FRAM_SPI m_fram;
	int8_t m_clk;
	int8_t m_miso;
	int8_t m_mosi;
	int8_t m_cs;
public:
	// HW-SPI constructor
	SFFS_HW_SPI(int8_t cs=0)
	{
		m_cs = cs;
		m_clk = -1;
	}
	// SW-SPI constructor
	SFFS_HW_SPI(int8_t clk, int8_t miso, int8_t mosi, int8_t cs)
	{
		m_clk=clk; m_miso=miso; m_mosi=mosi; m_cs=cs;
	}
	virtual eFRAM_HW ID() { return eFRAM_HW_SPI; };
	virtual bool begin(uint8 addressWidth)
	{
		return m_fram.begin(m_cs, addressWidth);
	}
	virtual bool read(uint32 addr, uint8* pBuf, uint32 count)
	{
		m_fram.read(addr, pBuf, count);
		return true;
	}
	virtual bool write(uint32 addr, uint8* pBuf, uint32 count)
	{
		m_fram.writeEnable(true);
		m_fram.write(addr, pBuf, count);
		bool bResult = true;
		m_fram.writeEnable(false);
		return bResult;
	}
};


class SFFS_Stream
{
private:
	uint32 m_offset;
	SFFS_HW* m_pFram;
public:
	SFFS_Stream() : m_offset(0)
	{
	}
	void Init(SFFS_HW* pFram)
	{
		m_pFram = pFram;
	}
	void Seek(uint32 offset)
	{
		m_offset = offset;
	}
	uint32 Tell()
	{
		return m_offset;
	}
	uint Read(uint8* pDest, uint32 count)
	{
		m_pFram->read(m_offset, pDest, count);
		m_offset += count;
		return count;
	}
	uint Read(uint32 addr, uint8* pDest, uint32 count)
	{
		Seek(addr);
		return Read(pDest, count);
	}
	uint Write(uint8* pSource, uint32 count)
	{
		m_pFram->write(m_offset, pSource, count);
		m_offset += count;
		return count;
	}
	uint Write(uint32 addr, uint8* pSource, uint32 count)
	{
		Seek(addr);
		return Write(pSource, count);
	}
};


class SFFS_FileHead
{
public:
	static SFFS_Volume* m_pVol;
private:
	bool m_bInUse;
	bool m_bChanged;
	uint m_index;
	uint32 m_streamOffset;
	uint32 m_dataOffset;
	uint32 m_dataWrittenSize;
	uint32 m_dataMaxSize;
	char m_name[SFFS_FILE_NAME_LEN+1];
public:
	SFFS_FileHead() : m_bInUse(false)
	{
	}

	//
	// File header operations
	//
	bool Create(const char* name, uint32 dataOffset, uint32 DataSize, uint index);
	void Open(uint index);
	void Commit();
	void Close();
	bool InUse()
	{
		return m_bInUse;
	}
	//
	// File data operations
	//
	static uint HeadSize();

	uint32 Size()
	{
		return m_dataWrittenSize;
	}
	const char* Name()
	{
		return m_name;
	}
	uint32 SizeMax()
	{
		return m_dataMaxSize;
	}
	uint32 Seek(uint32 offset)
	{
		if (checkFP(offset))
			m_streamOffset = offset;
		return Tell();
	}
	uint32 Tell()
	{
		return m_streamOffset;
	}
	uint32 Read(uint8* pDest, uint32 count);
	uint32 Write(uint8* pSource, uint32 count);
	uint32 Load(uint8* pDest);
	uint32 Save(uint8* pSource, uint32 count);
private:
	void seekHead();

	void seek(uint32 offset)
	{
		m_streamOffset = offset;
	}
	bool checkFP(uint32 offset)
	{
		return (offset<m_dataWrittenSize);
	}
	uint32 boundRead(uint32 offset, uint32 count)
	{
		if ((offset+count) > m_dataWrittenSize)
			count = m_dataWrittenSize-offset;
		return count;
	}
	uint32 boundWrite(uint32 offset, uint32 count)
	{
		if ((offset+count) > m_dataMaxSize)
			count = m_dataMaxSize-offset;
		return count;
	}
	uint32 _hasRead(uint32 done)
	{
		m_streamOffset += done;
		return done;
	}
	uint32 _hasWritten(uint32 done)
	{
		done = _hasRead(done);
		if (m_streamOffset > m_dataWrittenSize)
			m_dataWrittenSize = m_streamOffset;
		return done;
	}
	void _showFH();
};

typedef struct {
	uint32 value;
	uint32 chars[4];
}sSFFS_MAGIC;


class SFFS_Volume
{
private:
	SFFS_HW* m_pFram;
	SFFS_FileHead m_files[SFFS_MAX_OPEN_FILES];
	sSFFS_MAGIC m_magic;
	char m_volumeName[12];
	uint32 m_volumeSize;
	uint32 m_fileCount;
	uint32 m_fileMemStart;
	uint32 m_dataMemStart;
public:
	SFFS_Stream m_ios;

	SFFS_Volume(SFFS_HW& fram)
	{
		m_pFram = &fram;
		m_magic.value = 0;
		m_volumeName[0] = '\0';
	}
	bool begin(uint8 beginParam);

	void debug(bool bOnOff);

	uint32 HeadOffset()
	{
		return 0;
	}
	uint32 FileHeadOffset()
	{
		return m_fileMemStart;
	}
	uint32 DataStartOffset()
	{
		return m_volumeSize;
	}

	const char* VolumeName()
	{
		return (m_magic.value == SFFS_MAGIC_INT) ? m_volumeName : NULL;
	}
	bool VolumeCreate(const char* volumeName);

	//
	// File operations
	//
	uint fCount()
	{
		return m_fileCount;
	}
	SFFS_FP fCreate(const char* fileName, uint32 maxSize);
	SFFS_FP fOpen(uint index);
	SFFS_FP fOpen(const char* fileName);
	uint32 fSize(SFFS_FP handle)
	{
		SFFS_FileHead* pFile = (SFFS_FileHead*)handle;
		return pFile->Size();
	}
	uint32 fSizeMax(SFFS_FP handle)
	{
		SFFS_FileHead* pFile = (SFFS_FileHead*)handle;
		return pFile->SizeMax();
	}
	const char* fName(SFFS_FP handle)
	{
		SFFS_FileHead* pFile = (SFFS_FileHead*)handle;
		return pFile->Name();
	}
	uint32 fRead(SFFS_FP handle, uint8* pBuf, uint32 count)
	{
		SFFS_FileHead* pFile = (SFFS_FileHead*)handle;
		return pFile->Read(pBuf, count);
	}
	uint32 fReadAt(SFFS_FP handle, uint32 offset, uint8* pBuf, uint32 count)
	{
		SFFS_FileHead* pFile = (SFFS_FileHead*)handle;
		if (pFile->Seek(offset)==offset)
			return fRead(handle, pBuf, count);
		return 0;
	}
	uint32 fWrite(SFFS_FP handle, uint8* pBuf, uint32 count)
	{
		SFFS_FileHead* pFile = (SFFS_FileHead*)handle;
		return pFile->Write(pBuf, count);
	}
	uint32 fWriteAt(SFFS_FP handle, uint32 offset, uint8* pBuf, uint32 count)
	{
		SFFS_FileHead* pFile = (SFFS_FileHead*)handle;
		if (pFile->Seek(offset)==offset)
			return fWrite(handle, pBuf, count);
		return 0;
	}
	uint32 fSeek(SFFS_FP handle, uint32 offset)
	{
		SFFS_FileHead* pFile = (SFFS_FileHead*)handle;
		return pFile->Seek(offset);
	}
	void fClose(SFFS_FP handle)
	{
		SFFS_FileHead* pFile = (SFFS_FileHead*)handle;
		pFile->Close();
	}
private:
	bool 			_start();
	uint8 			_init(uint32 framAddrWidth);
	int 			_findFile(const char* fileName);
	SFFS_FileHead* 	_getFreeFile();
	int32 			_readBack(uint32 addr, int32 data);
	uint32 			_volumeSize();
	bool 			_volumeOpen();
	void 			_volumeCommit();

};


#endif //_SFFS_H