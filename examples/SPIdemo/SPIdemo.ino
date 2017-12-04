#include <SFFS.h>

#define FORCE_NEW_VOLUME false // Set this to true if you want it to create a new volume regarless

SFFS_Volume_SPI g_ffs;  // SPI FRAM FileSystem instance

void createFile(const char* name, uint32_t maxSizeInBytes);
void writeToFile(const char* name, uint32_t nBytesToWrite);
void showFile(uint fileIndex);
void listFiles();


void setup() {
  Serial.begin(115200);
  while(!Serial)
   ;

  // Use pin 10 for CS and a 2 byte address for an 8Kbyte FRAM, make this 3 if your FRAM is over 64Kbytes
  if (g_ffs.begin(10, 2)==false)
  {
     Serial.println("FRAM begin failed, check your connections.");
     while (true) 
       ;
  }

  Serial.print("SPI FRAM Init OK, capacity is ");
  Serial.print(g_ffs.VolumeSize());
  Serial.println(" bytes.");
  Serial.println("");

  if (FORCE_NEW_VOLUME || g_ffs.VolumeName()==NULL)
  {
    // Creat a new volume...
    if (g_ffs.VolumeCreate("SPIVolume_1")==false)
    {
      Serial.println("FAILED: Cannot create FRAM volume!");
      while (1)
        ;
	}
	else
	{
	  Serial.print("Created volume: '");
	  Serial.print(g_ffs.VolumeName());
	  Serial.println("'");

	  // Create some files with 1000 bytes maximum size... 
	  createFile("File One", 1000);	
	  createFile("File Two", 1000);	
	  createFile("File Three", 1000);	
	  // Write some data to files One and Three, but not Two... 
      writeToFile("File One", 10);
      writeToFile("File Three", 10);
	}
  }
  else
  {
    Serial.print("Found volume: '");
    Serial.print(g_ffs.VolumeName());
    Serial.println("'");
  }
  
  // List all files...
  listFiles();
  
  // Now write an additional 8 bytes to file Two...
  writeToFile("File Two", 8);

  // List all files again...
  listFiles();
}

void loop() {

}

void listFiles()
{
  Serial.println("");
  Serial.println("List Files:");
  // List all files...
  uint FileCount = g_ffs.fCount();
  for (uint i=0; i<FileCount; i++)
  {
    showFile(i);
  }	
  Serial.println("");
}

void showFile(uint fileIndex)
{
  SFFS_HANDLE handle = g_ffs.fOpen(fileIndex);
  if (handle != NULL)
  {
    Serial.print(" '");
    Serial.print(g_ffs.fName(handle));
    Serial.print("', size ");
    Serial.println(g_ffs.fSize(handle));
    
    g_ffs.fClose(handle);
  }
}

void writeToFile(const char* name, uint32_t nBytesToWrite)
{
  SFFS_HANDLE handle = g_ffs.fOpen(name);
  if (handle != NULL) 
  {
    Serial.print("Write ");
    Serial.print(nBytesToWrite);
    Serial.print(" bytes to '");
    Serial.print(name);
    Serial.println("'...");
  
    for (uint32_t i=0; i<nBytesToWrite; i++)
      g_ffs.fWrite(handle, (uint8_t*)&i, 1);
    g_ffs.fClose(handle);
  }
}

void createFile(const char* name, uint32_t maxSizeInBytes)
{
  SFFS_HANDLE handle = g_ffs.fCreate(name, maxSizeInBytes);
  if (handle != NULL) 
  {
    Serial.print("Create file '");
    Serial.print(name);
    Serial.println("'");

    // Success, now close it
    g_ffs.fClose(handle);
  }
}
