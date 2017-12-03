#include <SFFS.h>

#define USE_SPI	// Uncomment this to use I2C

#ifndef USE_SPI
#define USE_I2C
#define beginParam MB85RC_DEFAULT_ADDRESS // I2C hardware address
SFFS_HW_I2C g_hw;
#else // USE_SPI
// IMPORTANT: For SPI you must specify the correct address width of the FRAM (if not 2),
// if not done it is possible that some data on the FRAM could be overwritten, and
// also this the file system will not work.
#define beginParam 2 // SPI address width in bytes
#define CS_PIN 10
SFFS_HW_SPI g_hw(CS_PIN);
#endif

typedef struct {
	const char* name;
	uint16_t size;
}sFILE_ITEM;
sFILE_ITEM g_files[] = {
	"file100", 100,
	"file200", 200,
	"file300", 300,
	"file400", 400,
	"file500", 500,
	"file600", 600
};
#define g_fileCount (sizeof(g_files)/sizeof(sFILE_ITEM))

SFFS_Volume g_ffs(g_hw);
#define BUFFER_SIZE_MAX 350
uint8_t g_buffer[BUFFER_SIZE_MAX];
uint8_t g_readBuffer[BUFFER_SIZE_MAX];

void createFile(char* name, uint32_t maxSize)
{
	SFFS_FP handle = g_ffs.fCreate(name, maxSize);
	if (handle != NULL)
	{
		uint32_t hasWritten = g_ffs.fWrite(handle, g_buffer, sizeof(g_buffer));
		maxSize = (sizeof(g_buffer) < g_ffs.fSizeMax(handle)) ? sizeof(g_buffer) : g_ffs.fSizeMax(handle); 
		if (hasWritten != maxSize)
		{
		  	Serial.print("ERROR: Create file write: maxWrite ");
		  	Serial.print(maxSize);
		  	Serial.print(" wrote ");
		  	Serial.println(hasWritten);
			while (1)
			;
		}
		g_ffs.fClose(handle);
	}
}


bool checkFile(char* name)
{
	bool bRet = false;
	SFFS_FP handle = g_ffs.fOpen(name);
	if (handle != NULL)
	{
		bRet = true;
		
		g_ffs.fSeek(handle, 0);
		uint32_t readBytes = g_ffs.fRead(handle, g_readBuffer, sizeof(g_readBuffer));
		if (readBytes != g_ffs.fSize(handle))
		{
		  	Serial.print("ERROR: Check file read: size ");
		  	Serial.print(g_ffs.fSize(handle));
		  	Serial.print(" read ");
		  	Serial.println(readBytes);
		  	bRet = false;
			while (1)
			;
		}
		else
		{
		    uint32_t error = 0;
			for (uint32_t i=0; i<readBytes; i++)
			  if (g_readBuffer[i] != g_buffer[i])
			    error++;
			if (error > 0)
			{
		  		Serial.print("ERROR: Check file read: comp ");
		  		Serial.print(readBytes);
		  		Serial.print(" bytes, errors ");
		  		Serial.println(error);
			  	bRet = false;
				while (1)
				;
			}
		}
		g_ffs.fClose(handle);
	}
	if (false && bRet)
	{
  		Serial.print("Check file '");
  		Serial.print(name);
  		Serial.println("' OK");
	}
	return bRet;
}

bool checkAll()
{
	bool bRet = true;
	for (int i=0; i<g_fileCount && bRet==true; i++)
		bRet = checkFile(g_files[i].name);
	Serial.print("Check files "); Serial.println((bRet) ? "OK" : "FAIL");
	return bRet;
}

void writeFileAt(char* name, uint32_t offset, uint32_t writeSize)
{
	SFFS_FP handle = g_ffs.fOpen(name);
	if (handle != NULL)
	{
		g_ffs.fSeek(handle, offset);
		g_ffs.fWrite(handle, g_buffer, writeSize);
		g_ffs.fClose(handle);
	}
}


bool doATransfer(uint8_t SrcIdx, uint8_t DstIdx, uint32_t offset, uint32_t len)
{
	bool bRet = false;
	
	Serial.print("Tran: ");
	Serial.print(g_files[SrcIdx].name);
	Serial.print(" @");
	Serial.print(offset);
	Serial.print("[");
	Serial.print(len);
	Serial.print("] to ");
	Serial.println(g_files[DstIdx].name);
	
	SFFS_FP hSrc = g_ffs.fOpen(g_files[SrcIdx].name);
	if (hSrc != NULL)
	{
		uint32_t lenRead = g_ffs.fReadAt(hSrc, offset, g_readBuffer, len);
		g_ffs.fClose(hSrc); // Close here in case we are copying to the same file
		if (lenRead > 0)
		{
			SFFS_FP hDst = g_ffs.fOpen(g_files[DstIdx].name);
			if (hDst != NULL)
			{
				uint32_t lenWritten = g_ffs.fWriteAt(hDst, offset, g_readBuffer, lenRead);
				if (lenWritten > 0)
				{
				}
				g_ffs.fClose(hDst);
				bRet = true;
			}
		}	
	}	
	return bRet;
}

void setup(void)
{
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("Setup...");
  
  for (uint32_t i=0; i<sizeof(g_buffer); i++)
    g_buffer[i] = (uint8_t)i;
  
  if (g_ffs.begin(beginParam))
  {
    //g_ffs.debug(true);
    
	if (g_ffs.VolumeCreate("NewVolume")==false)
	{
		Serial.println("FAILED: Cannot create volume!");
		while (1)
		;
	}
	
	for (int i=0; i<g_fileCount; i++)
	{
		Serial.println("");
		createFile(g_files[i].name, g_files[i].size);
	}
	Serial.println("");
	checkAll();
	Serial.println("");
	Serial.println("******");
	Serial.println("");
  }
}

uint32_t loopCount = 0;

void loop(void)
{
	if ( doATransfer( rand()%g_fileCount,rand()%g_fileCount,10+(rand()%90), 1+(rand()%(sizeof(g_buffer)-1)) ) == false)
	{
		Serial.print("FAIL-TRAN: On loop #"); Serial.println(loopCount);
		while (1)
		;
	}
	if (checkAll()==false)
	{
		Serial.print("FAIL-CHECK: On loop #"); Serial.println(loopCount);
		while (1)
		;
	}
	loopCount++;
}