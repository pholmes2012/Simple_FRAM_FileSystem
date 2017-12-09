/**************************************************************************/
/*!
    @file     SFFS.cpp
    @author   Paul Holmes
    @license  BSD (see LICENSE)

    Simple FRam File System

    @section  HISTORY

    v1.0 - First release
*/
/**************************************************************************/
#include "SFFS.h"

//#define DEV_DBG

bool g_bDebug = false;
#ifdef DEV_DBG
#define DEBUG_OUT(s) if (g_bDebug) Serial.s
#define DEBUG_DEV(s) if (g_bDebug) Serial.s
#else
#define DEBUG_OUT(s)
#define DEBUG_DEV(s)
#endif

void 
SFFS_Volume::_printDbgNum(uint32 num)
{
	DEBUG_OUT(print("*** num = 0x"));
	DEBUG_OUT(println(num, HEX));
	//m_files[0]._showFH();

}
/**********************************************************************
//
// CFS_FILE_HEAD
//
***********************************************************************/
//static
uint32 SFFS_File::m_fileMemStart = 0;
//static
uint32 SFFS_File::m_headSize = sizeof(m_name) + sizeof(m_dataOffset) + sizeof(m_dataMaxSize) + sizeof(m_dataWrittenSize);

bool
SFFS_File::create(const char* name, uint32 dataOffset, uint32 dataSize, uint index)
{
	fClose();
	InUse(SFFS_Tools::strcpy(m_name, name, sizeof(m_name)));
	if (InUse())
	{
		m_index = index;
		m_dataOffset = dataOffset;
		m_dataMaxSize = dataSize;
		m_streamOffset = 0;
		m_dataWrittenSize = 0;
	  	DEBUG_OUT(print("Create: ")); DEBUG_OUT(print(m_name)); DEBUG_OUT(print(" size ")); DEBUG_OUT(println(dataSize));
		fSeek(0);
		commit();
	}
	else
	{
		DEBUG_OUT(print("SFFS: File name too long or incorrect!"));
	}
	return InUse();
}
bool
SFFS_File::fCreate(const char* fileName, uint32 maxSize)
{
	return m_volume.fileCreate(this, fileName, maxSize);
}


bool
SFFS_File::fOpen(uint index)
{
	fClose();
	SFFS_Stream& stream = m_volume.Stream();
	DEBUG_OUT(print("SFFS: fOpen = ")); DEBUG_OUT(println(index));
	InUse(true);
	m_index = index;
	stream.Seek(headOffset());
	stream.Read((uint8*)m_name, sizeof(m_name));
	stream.Read((uint8*)&m_dataOffset, sizeof(m_dataOffset));
	stream.Read((uint8*)&m_dataMaxSize, sizeof(m_dataMaxSize));
	stream.Read((uint8*)&m_dataWrittenSize, sizeof(m_dataWrittenSize));
	m_streamOffset = m_dataWrittenSize;
	return InUse();
}
bool 
SFFS_File::fOpen(const char* fileName)
{
	fClose();
	return m_volume.fileOpen(this, fileName);
}


void
SFFS_File::commit()
{
	SFFS_Stream& stream = m_volume.Stream();
	stream.Seek(headOffset());
	stream.Write((uint8*)m_name, sizeof(m_name));
	stream.Write((uint8*)&m_dataOffset, sizeof(m_dataOffset));
	stream.Write((uint8*)&m_dataMaxSize, sizeof(m_dataMaxSize));
	stream.Write((uint8*)&m_dataWrittenSize, sizeof(m_dataWrittenSize));
}
void
SFFS_File::commitWrite()
{
	SFFS_Stream& stream = m_volume.Stream();
	stream.Seek(headOffset());
	stream.Skip(sizeof(m_name));
	stream.Skip(sizeof(m_dataOffset));
	stream.Skip(sizeof(m_dataMaxSize));
	stream.Write((uint8*)&m_dataWrittenSize, sizeof(m_dataWrittenSize));
}


uint32
SFFS_File::fRead(void* pDest, uint32 count)
{
  	DEBUG_OUT(print("Read: ")); DEBUG_OUT(print(m_name)); DEBUG_OUT(print(" bytes: ")); DEBUG_OUT(println(count));
#ifdef DEV_DBG
	_showFH();
#endif
	uint32 done = m_volume.Stream().Read(m_dataOffset+m_streamOffset, pDest, boundRead(m_streamOffset, count));
	DEBUG_OUT(print("ReadDone: "));	DEBUG_OUT(println(done));
	_hasRead(done);
#ifdef DEV_DBG
	_showFH();
#endif
	return done;
}

#ifdef DEV_DBG
void
SFFS_File::_showFH()
{
	DEBUG_OUT(print(" idx "));
	DEBUG_OUT(print(m_index));
	DEBUG_OUT(print(" DOff "));
	DEBUG_OUT(print(m_dataOffset));
	DEBUG_OUT(print(", fp "));
	DEBUG_OUT(print(m_streamOffset));
	DEBUG_OUT(print(", size "));
	DEBUG_OUT(print(m_dataWrittenSize));
	DEBUG_OUT(print("/"));
	DEBUG_OUT(println(m_dataMaxSize));
}
#endif


uint32
SFFS_File::fWrite(void* pSource, uint32 count)
{
  	DEBUG_OUT(print("Write: ")); DEBUG_OUT(print(m_name)); DEBUG_OUT(print(" bytes: ")); DEBUG_OUT(println(count));
#ifdef DEV_DBG
	_showFH();
#endif
	uint32 done = m_volume.Stream().Write(m_dataOffset+m_streamOffset, pSource, boundWrite(m_streamOffset, count));
	_hasWritten(done);
	DEBUG_OUT(print("WriteDone: ")); DEBUG_OUT(println(done));
#ifdef DEV_DBG
	_showFH();
#endif
	return done;
}

/**********************************************************************
//
// CFS_VOLUME
//
***********************************************************************/

bool
SFFS_Volume::init()
{
	bool bRet = false;

	if (_readBack(4, 0xABADDEED)==0xABADDEED)
	{
		m_volumeSize = _volumeSize();
		if (_volumeOpen()==false)
		{
			DEBUG_OUT(println("SFFS: No volume found."));
		}
		bRet = true;
	}
	else
	{
		DEBUG_OUT(println("SFFS: Can not read or write FRAM."));
	}
	return bRet;
}

void
SFFS_Volume::debug(bool bOnOff)
{
	g_bDebug = bOnOff;
}

bool
SFFS_Volume::VolumeCreate(const char* volumeName)
{
	m_magic = SFFS_MAGIC_INT;
	SFFS_Tools::strcpy(m_volumeName, volumeName, sizeof(m_volumeName));
	m_fileCount = 0;
	m_dataMemStart = m_volumeSize;
	
	_volumeCommit();
	
	DEBUG_OUT(print("Volume '")); DEBUG_OUT(print(m_volumeName)); DEBUG_OUT(println("' created."));

	return _volumeOpen();
}

bool
SFFS_Volume::_volumeOpen()
{
	bool bRet = false;

	m_ios.Seek(0);
	m_ios.Read((uint8*)&m_magic, sizeof(m_magic));
	if (m_magic == SFFS_MAGIC_INT)
	{
		m_ios.Read((uint8*)&m_volumeName, sizeof(m_volumeName));
		m_ios.Read((uint8*)&m_fileCount, sizeof(m_fileCount));
		m_ios.Read((uint8*)&m_dataMemStart, sizeof(m_dataMemStart));
		m_ios.Read((uint8*)&m_magic, sizeof(m_magic));
		if (m_magic == SFFS_MAGIC_INT)
		{
			DEBUG_OUT(print("SFFS: Volume '")); 
			DEBUG_OUT(print(m_volumeName)); 
			DEBUG_OUT(println("' mounted.")); 
			SFFS_File::m_fileMemStart = m_ios.Tell();
			bRet = true;
		}
	}
	if (!bRet)
	{
		DEBUG_OUT(println("SFFS: No volume mounted."));
	}
	return bRet;
}

void
SFFS_Volume::_volumeCommit()
{
	m_ios.Seek(0);
	m_ios.Write((uint8*)&m_magic, sizeof(m_magic));
	m_ios.Write((uint8*)&m_volumeName, sizeof(m_volumeName));
	m_ios.Write((uint8*)&m_fileCount, sizeof(m_fileCount));
	m_ios.Write((uint8*)&m_dataMemStart, sizeof(m_dataMemStart));
	m_ios.Write((uint8*)&m_magic, sizeof(m_magic));
	SFFS_File::m_fileMemStart = m_ios.Tell();
}

uint32
SFFS_Volume::VolumeFree()
{
	uint32 memStart = SFFS_File::m_fileMemStart + ((m_fileCount+1)*SFFS_File::m_headSize);
	return m_dataMemStart-memStart;
}

bool
SFFS_Volume::fileOpen(SFFS_File* pFile, const char* fileName)
{
	int index = _findFile(fileName);
	DEBUG_OUT(print("SFFS: fileOpen = ")); DEBUG_OUT(println(index));
	if (index == -1)
		return false;

	return pFile->fOpen(index);
}

bool
SFFS_Volume::fileCreate(SFFS_File* pFile, const char* fileName, uint32 maxSize)
{
	if (_findFile(fileName) == -1)
	{
		if (VolumeFree() >= maxSize)
		{
			uint32 dataOffset = m_dataMemStart-maxSize;
			if (pFile->create(fileName, dataOffset, maxSize, m_fileCount))
			{
				m_dataMemStart -= maxSize;
				m_fileCount++;
				// Save to disk
				_volumeCommit();
			}
			else
			{
				// Failed
			}
		}
		else
		{
			DEBUG_OUT(println("SFFS: Not enough space!"));
		}
	}
	else
	{
		DEBUG_OUT(println("SFFS: File already exists!"));
	}
	return pFile->InUse();
}

//**************************************************
// Backup, write, read-back then restore, then compare
//**************************************************
uint32
SFFS_Volume::_readBack(uint32 addr, uint32 data)
{
  uint32 check = !data;
  uint32 wrapCheck, backup;
  m_ios.Read(addr, (uint8*)&backup, sizeof(uint32));
  m_ios.Write(addr, (uint8*)&data, sizeof(uint32));
  m_ios.Read(addr, (uint8*)&check, sizeof(uint32));
  m_ios.Read(0, (uint8_t*)&wrapCheck, sizeof(uint32));
  m_ios.Write(addr, (uint8*)&backup, sizeof(uint32));
  // Check for warparound, address 0 will work anyway
  if (wrapCheck==check)
    check = 0;
  return check;
}
uint32
SFFS_Volume::_volumeSize()
{
	uint32 memSize = 0;
	// Step through FRAM until we hit a wraparound to get the size
	while (_readBack(memSize, memSize) == memSize)
		memSize += 256;
	return memSize;
}
// Locate a file by name on the disk, if it exists.
int
SFFS_Volume::_findFile(const char* fileName)
{
	char name[SFFS_FILE_NAME_BUFFER_LEN];
	uint32 offset = SFFS_File::m_fileMemStart;

	for (uint i=0; i<m_fileCount; i++)
	{
		m_ios.Seek(offset);
		m_ios.Read((uint8*)name, sizeof(name));
		if (SFFS_Tools::strcmp(name, fileName))
		{
			// Found it
			return (int)i;
		}
		offset += SFFS_File::m_headSize;
	}
	return -1;
}

