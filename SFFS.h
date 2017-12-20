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

#include "io_driver.h"

#define SFFS_MAGIC_INT (uint32)('1'<<24 | '0'<<16 | 'S'<<8 | 'F')
#define SFFS_FILE_NAME_LEN 15 // Maximum length of a file or volume name (excluding the trailing 0)
#define SFFS_FILE_NAME_BUFFER_LEN (SFFS_FILE_NAME_LEN+1)

class SFFS_Volume;

class SFFS_Tools
{
private:
public:
	static bool strcpy(char* pDest, const char* pSrc, uint maxLen)
	{
		uint len = 0;
		while (len<maxLen && pSrc[len] != '\0')
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


class SFFS_Stream
{
private:
	cIO_DRV& m_driver;
	uint32 m_offset;
public:
	SFFS_Stream(cIO_DRV& driver) :
				m_driver(driver),
				m_offset(0)
	{
	}
	void Seek(uint32 offset)
	{
		m_offset = offset;
	}
	void Skip(uint32 count)
	{
		m_offset += count;
	}
	uint32 Tell()
	{
		return m_offset;
	}
	uint Read(void* pDest, uint32 count)
	{
		count = m_driver.Read(m_offset, pDest, count);
		m_offset += count;
		return count;
	}
	uint Read(uint32 addr, void* pDest, uint32 count)
	{
		Seek(addr);
		return Read(pDest, count);
	}
	uint Write(const void* pSource, uint32 count)
	{
		count = m_driver.Write(m_offset, pSource, count);
		m_offset += count;
		return count;
	}
	uint Write(uint32 addr, const void* pSource, uint32 count)
	{
		Seek(addr);
		return Write(pSource, count);
	}
};


class SFFS_File
{
private:
	SFFS_Volume& m_volume;
	uint m_index;
	bool m_bInUse;
	uint32 m_streamOffset;
	uint32 m_dataOffset;
	uint32 m_dataWrittenSize;
	uint32 m_dataMaxSize;
	char m_name[SFFS_FILE_NAME_BUFFER_LEN];
public:
	static uint32 m_fileMemStart;
	static uint32 m_headSize;

	SFFS_File(SFFS_Volume& volume) :
			m_volume(volume)
	{
		InUse(false);
	}
	bool fOpen(uint index);
	bool fOpen(const char* fileName);
	bool fCreate(const char* fileName, uint32 maxSize);

	bool InUse()
	{
		return m_bInUse;
	}

	//
	// File operations
	//
	uint32 fSize()
	{
		return m_dataWrittenSize;
	}
	uint32 fSizeMax()
	{
		return m_dataMaxSize;
	}
	const char* fName()
	{
		return m_name;
	}
	uint32 fRead(void* pBuf, uint32 count);
	uint32 fReadAt(uint32 offset, void* pBuf, uint32 count)
	{
		if (fSeek(offset)==offset)
			return fRead(pBuf, count);
		return 0;
	}
	uint32 fWrite(const void* pBuf, uint32 count);
	uint32 fWriteAt(uint32 offset, const void* pBuf, uint32 count)
	{
		if (fSeek(offset)==offset)
			return fWrite(pBuf, count);
		return 0;
	}
	uint32 fSeek(uint32 offset)
	{
		if (checkFP(offset))
			m_streamOffset = offset;
		return fTell();
	}
	uint32 fTell()
	{
		return m_streamOffset;
	}
	uint32 fShrink(uint32 shrinkBy);
	void fClose()
	{
		if (InUse())
		{
			InUse(false);
		}
	}

//protected friend
public:
	bool create(const char* name, uint32 dataOffset, uint32 DataSize, uint index);

private:
	void InUse(bool bOnOff)
	{
		m_bInUse = bOnOff;
	}
	void commit();
	void commitWrite();

	void _showFH();

	uint32 headOffset()
	{
		return SFFS_File::m_fileMemStart + (m_index*SFFS_File::m_headSize);
	}

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
	void _hasRead(uint32 done)
	{
		m_streamOffset += done;
	}
	void _hasWritten(uint32 done)
	{
		_hasRead(done);
		if (m_streamOffset > m_dataWrittenSize)
		{
			m_dataWrittenSize = m_streamOffset;
			commitWrite();
		}
	}
};


class SFFS_Volume
{
private:
	char m_volumeName[SFFS_FILE_NAME_BUFFER_LEN];
	uint32 m_magic;
	uint32 m_volumeSize;
	uint32 m_fileCount;
	uint32 m_dataMemStart;
	SFFS_Stream m_ios;
public:

	SFFS_Volume(cIO_DRV& driver) :
			m_magic(0),
			m_volumeSize(0),
			m_fileCount(0),
			m_dataMemStart(0),
			m_ios(driver)
	{
	}

	void debug(bool bOnOff);

	//
	// Volume operations
	//
	bool VolumeCreate(const char* volumeName);
	uint32 VolumeSize()
	{
		if (VolumeName()==NULL)
			return 0;
		return m_volumeSize;
	}
	uint32 VolumeFree();
	const char* VolumeName()
	{
		return (m_magic == SFFS_MAGIC_INT) ? m_volumeName : NULL;
	}
	//
	// File operations
	//
	SFFS_Stream& Stream()
	{
		return m_ios;
	}
	uint FileCount()
	{
		return m_fileCount;
	}
//protected friend
	bool fileCreate(SFFS_File* pFile, const char* fileName, uint32 maxSize);
	bool fileOpen(SFFS_File* pFile, const char* fileName);
protected:
	bool			init();
private:
	bool 			_start();
	uint8 			_init(uint32 framAddrWidth);
	int 			_findFile(const char* fileName);
	uint32 			_readBack(uint32 addr, uint32 data);
	uint32 			_volumeSize();
	bool 			_volumeOpen();
	void 			_volumeCommit();
	void			_printDbgNum(uint32 num);
};

class SFFS_Volume_SPI : public SFFS_Volume
{
private:
	cIO_DRV_SPI m_drv;
public:
	SFFS_Volume_SPI() : SFFS_Volume(m_drv)
	{
	}
	bool begin(uint8 csPin, uint8 addrWidth=2)
	{
		if (m_drv.Init(csPin, addrWidth))
			return init();
		return false;
	}
};

class SFFS_Volume_I2C : public SFFS_Volume
{
private:
	cIO_DRV_I2C m_drv;
public:
	SFFS_Volume_I2C() : SFFS_Volume(m_drv)
	{
	}
	bool begin(uint8 hwAddr)
	{
		if (m_drv.Init(hwAddr))
			return init();
		return false;
	}
};

#endif //_SFFS_H
