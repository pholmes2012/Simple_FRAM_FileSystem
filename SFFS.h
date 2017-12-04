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
#include "io_driver.h"

#define SFFS_MAGIC_INT (uint32)('1'<<24 | '0'<<16 | 'S'<<8 | 'F')
#define SFFS_MAX_OPEN_FILES 4
#define SFFS_FILE_NAME_LEN 15
#define SFFS_FILE_NAME_BUFFER_LEN (SFFS_FILE_NAME_LEN+1)

class SFFS_Volume;

typedef void* SFFS_HANDLE;

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


class SFFS_Stream
{
private:
	uint32 m_offset;
	cIO_DRV* m_pFram;
public:
	SFFS_Stream() : m_offset(0)
	{
	}
	void Init(cIO_DRV* pFram)
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
		count = m_pFram->Read(m_offset, pDest, count);
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
		count = m_pFram->Write(m_offset, pSource, count);
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
	cIO_DRV* m_pFram;
	SFFS_FileHead m_files[SFFS_MAX_OPEN_FILES];
	sSFFS_MAGIC m_magic;
	char m_volumeName[SFFS_FILE_NAME_LEN+1];
	uint32 m_volumeSize;
	uint32 m_fileCount;
	uint32 m_fileMemStart;
	uint32 m_dataMemStart;
public:
	SFFS_Stream m_ios;

	SFFS_Volume(cIO_DRV& driver)
	{
		m_pFram = &driver;
		m_magic.value = 0;
	}

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

	/**************************************************************************/
	/*!
	@brief  Get the current FRAM capacity.
	
    @returns    The FRAM memory capacity in bytes
	*/
	/**************************************************************************/
	uint32 VolumeSize()
	{
		return m_volumeSize;
	}
	/**************************************************************************/
	/*!
	@brief  Get the current FRAM volume name, if one exists.
	
    @returns    The current volume name, NULL if none exists
	*/
	/**************************************************************************/
	const char* VolumeName()
	{
		return (m_magic.value == SFFS_MAGIC_INT) ? m_volumeName : NULL;
	}
	/**************************************************************************/
	/*!
	@brief  Create a new volume on the FRAM, this will effectively erase
	any volume and its files already on there.

	@params[in] volumeName
                The new volume name, must be no longer than
				SFFS_FILE_NAME_LEN, not including the trailing 0.

    @returns    true if success false if failed
	*/
	/**************************************************************************/
	bool VolumeCreate(const char* volumeName);

	//
	// Directory operations
	//

	/**************************************************************************/
	/*!
	@brief  Get the current file count.

    @returns    Current file count
	*/
	/**************************************************************************/
	uint fCount()
	{
		return m_fileCount;
	}
	
	/**************************************************************************/
	/*!
	@brief  Get the filename of a file.

    @params[in] index
                The index of the file
    @params[in] pBuffer
                A buffer to receive the filename
    @params[in] bufferSize
                Size of the buffer in bytes, must be at least SFFS_FILE_NAME_BUFFER_LEN

    @returns    true if success false if failed
	*/
	/**************************************************************************/
	bool fList(uint index, char* pBuffer, uint bufferSize);

	//
	// File operations
	//

	/**************************************************************************/
	/*!
	@brief  Create a new file.

    @params[in] fileName
                The filename for the new file, must not already exist.
    @params[in] maxSize
                The maximum possible size for the new file. The file will
				be created with 0 size, this is the maximum size it can grow to.

    @returns    A handle to the new (open) file, NULL if failed.
	*/
	/**************************************************************************/
	SFFS_HANDLE fCreate(const char* fileName, uint32 maxSize);
	
	/**************************************************************************/
	/*!
	@brief  Open a file via its index in the filesystem. This
	corresponds with the filenames output by fList().
	Note: A file can not be opened more than once simultaniusly.

    @params[in] index
                An 0 based index to a file in the filesystem, must be less than fCount()

    @returns    A handle to the opened file, NULL if failed.
	*/
	/**************************************************************************/
	SFFS_HANDLE fOpen(uint index);

	/**************************************************************************/
	/*!
	@brief  Open a file via its filename. The file must exist.
	Note: A file can not be opened more than once simultaniusly.

    @params[in] fileName
                Filename of the file to be opened

    @returns    A handle to the opened file, NULL if failed.
	*/
	/**************************************************************************/
	SFFS_HANDLE fOpen(const char* fileName);

	/**************************************************************************/
	/*!
	@brief  Get the current size of an open file, this is the already
	written to size and not the maximum possible size.

    @params[in] handle
                The handle of a currently open file

    @returns    The current size of the file
	*/
	/**************************************************************************/
	uint32 fSize(SFFS_HANDLE handle)
	{
		SFFS_FileHead* pFile = (SFFS_FileHead*)handle;
		return pFile->Size();
	}

	/**************************************************************************/
	/*!
	@brief  Get the maximum possible size of an open file.

    @params[in] handle
                The handle of a currently open file

    @returns    The current maximum possible size of the file
	*/
	/**************************************************************************/
	uint32 fSizeMax(SFFS_HANDLE handle)
	{
		SFFS_FileHead* pFile = (SFFS_FileHead*)handle;
		return pFile->SizeMax();
	}

	/**************************************************************************/
	/*!
	@brief  Get the filename of an open file.

    @params[in] handle
                The handle of a currently open file

    @returns    The filename of the file
	*/
	/**************************************************************************/
	const char* fName(SFFS_HANDLE handle)
	{
		SFFS_FileHead* pFile = (SFFS_FileHead*)handle;
		return pFile->Name();
	}

	/**************************************************************************/
	/*!
	@brief  Read bytes from a file into a buffer, this reads
	from the current file pointer of this open file.

    @params[in] handle
                The handle of a currently open file

	@params[in] pBuf
                A buffer to receive the data, it is the callers responsibility
				to ensure this buffer is large enough to receive the data

	@params[in] count
                The number of the bytes to read from the file

    @returns    The actual number of bytes read, this could be less than 'count'
				as the file pointer may have reached the end of the file/fSize()
	*/
	/**************************************************************************/
	uint32 fRead(SFFS_HANDLE handle, uint8* pBuf, uint32 count)
	{
		SFFS_FileHead* pFile = (SFFS_FileHead*)handle;
		return pFile->Read(pBuf, count);
	}

	/**************************************************************************/
	/*!
	@brief  Read bytes from a file at a specified location into a buffer, this sets
	the current file pointer to 'offset' before reading. This will not restore the
	original file pointer after use.

    @params[in] handle
                The handle of a currently open file

	@params[in] offset
                An offset into the file relative to 0 (same as fSeek(offset))

	@params[in] pBuf
                A buffer to receive the data, it is the callers responsibility
				to ensure this buffer is large enough to receive the data

	@params[in] count
                The number of the bytes to read from the file

    @returns    The actual number of bytes read, this could be less than 'count'
				as the file pointer may have reached the end of the file (fp == fSize()),
				it could also be 0 if 'offset' is greater than the current file size (fSize())
	*/
	/**************************************************************************/
	uint32 fReadAt(SFFS_HANDLE handle, uint32 offset, uint8* pBuf, uint32 count)
	{
		SFFS_FileHead* pFile = (SFFS_FileHead*)handle;
		if (pFile->Seek(offset)==offset)
			return fRead(handle, pBuf, count);
		return 0;
	}

	/**************************************************************************/
	/*!
	@brief  Write bytes from a buffer into a file, this writes
	from the current file pointer of this open file.

    @params[in] handle
                The handle of a currently open file

	@params[in] pBuf
                A buffer to constaining the data to be written.

	@params[in] count
                The number of the bytes to write to the file

    @returns    The actual number of bytes written, this could be less than 'count'
				as the file pointer may have reached the maximum file size (fp == fSizeMax())
	*/
	/**************************************************************************/
	uint32 fWrite(SFFS_HANDLE handle, uint8* pBuf, uint32 count)
	{
		SFFS_FileHead* pFile = (SFFS_FileHead*)handle;
		return pFile->Write(pBuf, count);
	}

	/**************************************************************************/
	/*!
	@brief  Write bytes to a file at a specified file location from a buffer, this sets
	the current file pointer to 'offset' before writing. This will not restore the
	original file pointer after use.

    @params[in] handle
                The handle of a currently open file

	@params[in] offset
                An offset into the file relative to 0 (same as fSeek(offset))

	@params[in] pBuf
                A buffer cotaining the data to send.

	@params[in] count
                The number of the bytes to write to the file

    @returns    The actual number of bytes written, this could be less than 'count'
				as the file pointer may have reached the maximum file size (fp == fSizeMax()),
				it could also be 0 if 'offset' is greater than the current file size (fSize())
	*/
	/**************************************************************************/
	uint32 fWriteAt(SFFS_HANDLE handle, uint32 offset, uint8* pBuf, uint32 count)
	{
		SFFS_FileHead* pFile = (SFFS_FileHead*)handle;
		if (pFile->Seek(offset)==offset)
			return fWrite(handle, pBuf, count);
		return 0;
	}

	/**************************************************************************/
	/*!
	@brief  Set the current file pointer to a specified location.

    @params[in] handle
                The handle of a currently open file

	@params[in] offset
                An offset relative to 0 to set the current file poiter to.

    @returns    The current file pointer, this will either be the same as
				'offset' if it succeeded or it will be unchanged indicating
				failure, failure could happen if 'offset' is greater than the 
				current file size (fSize())
	*/
	/**************************************************************************/
	uint32 fSeek(SFFS_HANDLE handle, uint32 offset)
	{
		SFFS_FileHead* pFile = (SFFS_FileHead*)handle;
		return pFile->Seek(offset);
	}

	/**************************************************************************/
	/*!
	@brief  Close the currently open file.

    @params[in] handle
                The handle of a currently open file
	*/
	/**************************************************************************/
	void fClose(SFFS_HANDLE handle)
	{
		SFFS_FileHead* pFile = (SFFS_FileHead*)handle;
		pFile->Close();
	}
protected:
	bool			init();
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
			return SFFS_Volume::init();
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
			return SFFS_Volume::init();
		return false;
	}
};

#endif //_SFFS_H