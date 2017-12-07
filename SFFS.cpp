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
	inUse(SFFS_Tools::strcpy(m_name, name, sizeof(m_name)));
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


void
SFFS_File::open(uint index)
{
	m_index = index;
	seekHead();
	m_pStream->Read((uint8*)m_name, sizeof(m_name));
	m_pStream->Read((uint8*)&m_dataOffset, sizeof(m_dataOffset));
	m_pStream->Read((uint8*)&m_dataMaxSize, sizeof(m_dataMaxSize));
	m_pStream->Read((uint8*)&m_dataWrittenSize, sizeof(m_dataWrittenSize));
	m_streamOffset = m_dataWrittenSize;
	inUse(true);
}

void
SFFS_File::commit()
{
	seekHead();
	m_pStream->Write((uint8*)m_name, sizeof(m_name));
	m_pStream->Write((uint8*)&m_dataOffset, sizeof(m_dataOffset));
	m_pStream->Write((uint8*)&m_dataMaxSize, sizeof(m_dataMaxSize));
	m_pStream->Write((uint8*)&m_dataWrittenSize, sizeof(m_dataWrittenSize));
}
void
SFFS_File::commitWrite()
{
	seekHead();
	m_pStream->Skip(sizeof(m_name));
	m_pStream->Skip(sizeof(m_dataOffset));
	m_pStream->Skip(sizeof(m_dataMaxSize));
	m_pStream->Write((uint8*)&m_dataWrittenSize, sizeof(m_dataWrittenSize));
}

void
SFFS_File::seekHead()
{
	uint32 offset = SFFS_File::m_fileMemStart + (m_index*SFFS_File::m_headSize);
	m_pStream->Seek(offset);
}


uint32
SFFS_File::fRead(uint8* pDest, uint32 count)
{
  	DEBUG_OUT(print("Read: ")); DEBUG_OUT(print(m_name)); DEBUG_OUT(print(" bytes: ")); DEBUG_OUT(println(count));
#ifdef DEV_DBG
	_showFH();
#endif
	uint32 done = m_pStream->Read(m_dataOffset+m_streamOffset, pDest, boundRead(m_streamOffset, count));
	DEBUG_OUT(print("ReadDone: "));	DEBUG_OUT(println(done));
	done = _hasRead(done);
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
SFFS_File::fWrite(uint8* pSource, uint32 count)
{
  	DEBUG_OUT(print("Write: ")); DEBUG_OUT(print(m_name)); DEBUG_OUT(print(" bytes: ")); DEBUG_OUT(println(count));
#ifdef DEV_DBG
	_showFH();
#endif
	uint32 done = m_pStream->Write(m_dataOffset+m_streamOffset, pSource, boundWrite(m_streamOffset, count));
	bool bChanged = _hasWritten(done);
	DEBUG_OUT(print("WriteDone: ")); DEBUG_OUT(println(done));
#ifdef DEV_DBG
	_showFH();
#endif
	hasChanged(bChanged);
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

bool
SFFS_Volume::FileList(uint index, char* pBuffer, uint bufferSize)
{
	bool bRet = false;
	if (bufferSize > SFFS_FILE_NAME_LEN)
	{
		if (index < m_fileCount)
		{
			SFFS_File head;
			head.open(index);
			if (SFFS_Tools::strcpy(pBuffer, head.fName(), bufferSize))
			{
				bRet = true;
			}
			head.fClose();
		}
	}
	if (bRet==false)
	{
		DEBUG_OUT(println("SFFS:fList: Bad param."));
	}
	return bRet;
}

SFFS_File*
SFFS_Volume::FileCreate(const char* fileName, uint32 maxSize)
{
	SFFS_File* pFile = NULL;
	if (_findFile(fileName) == -1)
	{
		pFile = m_files.New();
		if (pFile != NULL)
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
				pFile = NULL;
			}
		}
	}
	else
	{
		DEBUG_OUT(println("SFFS: File alread exists!"));
	}
	return pFile;
}

SFFS_File*
SFFS_Volume::FileOpen(uint index)
{
	SFFS_File* pHead = m_files.New();
	if (pHead != NULL)
	{
		pHead->open(index);
	}
	return pHead;
}

SFFS_File*
SFFS_Volume::FileOpen(const char* fileName)
{
	int index = _findFile(fileName);
	if (index != -1)
		return FileOpen(index);
	return NULL;
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

