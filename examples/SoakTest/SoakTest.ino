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

SFFS_Volume g_ffs(g_hw);
#define BUFFER_SIZE_MAX 450
uint8_t g_buffer[BUFFER_SIZE_MAX];
uint8_t g_readBuffer[BUFFER_SIZE_MAX];

void createFile(char* name, uint32_t maxSize)
{
	SFFS_FP handle = g_ffs.fCreate(name, maxSize);
	if (handle != NULL)
	{
		uint32_t hasWritten = g_ffs.fWrite(handle, g_buffer, sizeof(g_buffer));
		if (hasWritten != maxSize)
		{
		  	Serial.print("ERROR: Create file write: size ");
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
	if (bRet)
	{
  		Serial.print("Check file '");
  		Serial.print(name);
  		Serial.println("' OK");
	}
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
    g_ffs.debug(true);
    
	if (g_ffs.VolumeCreate("NewVolume")==false)
	{
		Serial.println("FAILED: Cannot create volume!");
		while (1)
		;
	}
	createFile("file100", 100);
	createFile("file200", 200);
	createFile("file300", 300);
	createFile("file400", 400);
	createFile("file500", 500);
	createFile("file600", 600);
	
	checkFile("file100");
	checkFile("file200");
	checkFile("file300");
	checkFile("file400");
	checkFile("file500");
	checkFile("file600");
  }
}

void loop(void)
{
}