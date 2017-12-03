#include <SFFS.h>

//#define USE_SPI	// Uncomment this to use I2C


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
uint8_t g_buffer[128];

void createFile(char* name, uint32_t maxSize)
{
	SFFS_FP handle = g_ffs.fCreate(name, maxSize);
	if (handle != NULL)
	{
	  	Serial.print("Create: "); Serial.println(name);
		g_ffs.fClose(handle);
	}
}
void writeFile(char* name, uint32_t writeSize)
{
	SFFS_FP handle = g_ffs.fOpen(name);
	if (handle != NULL)
	{
		g_ffs.fWrite(handle, g_buffer, writeSize);
		g_ffs.fClose(handle);
	}
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
  while (!Serial);
  
  for (uint16_t i=0; i<sizeof(g_buffer); i++)
    g_buffer[i] = i;
  
  if (g_ffs.begin(beginParam))
  {
    g_ffs.debug(true);
    
  	if (true || g_ffs.VolumeName()==NULL)
  	{
  		if (g_ffs.VolumeCreate("NewVolume")==false)
  		{
  			Serial.println("FAILED: Cannot create volume!");
  			while (1)
  			;
  		}
  		createFile("Paul01.bin", 1000);
  		createFile("Paul02.bin", 1000);
  		createFile("Paul03.bin", 1000);
  		createFile("Paul04.bin", 1000);
     }
     
	//writeFile("Paul01.bin", 10);
	writeFile("Paul02.bin", 20);
	writeFileAt("Paul02.bin", 10, 5);
	writeFile("Paul02.bin", 5);
	writeFileAt("Paul02.bin", 0, 5);
	writeFile("Paul02.bin", 8);
	writeFileAt("Paul02.bin", 25, 10);
	//writeFile("Paul03.bin", 30);
	//writeFile("Paul04.bin", 40);

	uint32_t count = g_ffs.fCount();
	for (uint32_t i=0; i<count; i++)
	{
	  SFFS_FP handle = g_ffs.fOpen(i);
	  if (handle != NULL)
	  {
	  	Serial.print("File: '");
	  	Serial.print(g_ffs.fName(handle)); 	
	  	Serial.print("' size "); 	
	  	Serial.print(g_ffs.fSize(handle)); 	
	  	Serial.print(" max "); 	
	  	Serial.println(g_ffs.fSizeMax(handle)); 	
	  	g_ffs.fClose(handle);
	  }
  	}
  }
}

void loop(void)
{
}