#include <SFFS.h>

// SFFS_Volume API
//
// begin(uint8 csPin, uint8 addressWidth); // Initialise the SFFS and the SPI FRAM device
// VolumeName();                           // Return the volume name if one exists, or NULL if not
// VolumeCreate(char* volumeName)          // Create a new volume, overwrite if one already exists
//
// SFFS_File API
//
// fCreate(char* fileName, uint32 maxSize) // Create a file with a name and a maximum size it can grow to
// fOpen(char* fileName);                  // Open an existing file, or return false if the file does not exist 
// fOpen(uint idx);                        // Open a file at idx, or return false if fewer than idx+1 files exists
// fClose();                               // Close an open file
// fSeek(uint32 fileOffset);               // Seek to a position in a file, fail if out of bounds, return current position either way
// fTell();                                // Return the current read/write position in a file  
// fRead(uin8* buffer, uint32 count);      // Read in data from the current file position  					
// fWrite(uin8* buffer, uint32 count);     // Write out data starting at the current file position 
// fReadAt(uint32 fileOffset, uin8* buffer, uint32 count); // Read in data after seeking to a file position
// fWriteAt(uint32 fileOffset, uin8* buffer, uint32 count); // Write our data after seeking to a file position  
//

// Set this to true if you want to force a new volume creation
#define FORCE_NEW_VOLUME false

SFFS_Volume_SPI g_ffs;   // SPI FRAM FileSystem instance
SFFS_File g_file(g_ffs); // The file instance we will use

// The structure we are going to save as a file.
typedef struct {
  int32_t a;
  int16_t b;
  int8_t c;
  int8_t d;
}MY_STRUCT;

MY_STRUCT my_struct;
void show();


void setup() {
  Serial.begin(115200);
    while(!Serial)
      ;

  // Initialise the SPI FRAM, use pin 10 for CS, address width of 2 bytes (16bit)
  if (g_ffs.begin(10, 2)==false)
  {
      Serial.println("FAILED: Cannot initialise FRAM");
      while (1)
        ;
  }

  // Check for existing FRAM filesystem, or create one
  if (FORCE_NEW_VOLUME || g_ffs.VolumeName()==NULL)
  {
    // Creat a new volume...
    Serial.println("Create new FRAM volume");
    if (g_ffs.VolumeCreate("Volume_1")==false)
    {
      Serial.println("FAILED: Cannot create FRAM volume");
      while (1)
        ;
    }
  }

  Serial.print("FRAM volume '"); Serial.print(g_ffs.VolumeName()); Serial.println("'");

  // Open or create our structure file
  if (g_file.fOpen("MyStruct")==false)
  {
     // Create our file with its maximum size being the size of our structure
     if (g_file.fCreate("MyStruct", sizeof(my_struct)))
     {
       // Zero then write out our structure
       memset(&my_struct, 0, sizeof(my_struct));
       // At this point the file size is 0, so we write all our data to it then
       // the file size will be the size of our structure (also this file's maximum size).
       g_file.fWrite(&my_struct, sizeof(my_struct));
       Serial.println("Created MyStruct file");
     }
     else
     {
       Serial.println("FAILED: Cannot create the file");
       while (1)
         ;
     }
  }
  else
  {
    Serial.println("Opened MyStruct file");
  }

  // Load in our structure
  g_file.fReadAt(0, &my_struct, sizeof(my_struct));
  // Show our initial structure
  show();
  Serial.println("Change structure variables with..");
  Serial.println("a=123");
  Serial.println("c=56");
  Serial.println("etc...");
}


void show()
{
  Serial.print("a = "); Serial.println(my_struct.a); 
  Serial.print("b = "); Serial.println(my_struct.b); 
  Serial.print("c = "); Serial.println(my_struct.c); 
  Serial.print("d = "); Serial.println(my_struct.d); 
  Serial.println();
}

int idx = 0;
char input[32];

void loop() {

  if (Serial.available())
  {
    char C = (char)Serial.read();
	input[idx++] = C;
	if (idx==sizeof(input) || C=='\n' || C=='\r' || C=='\0')
    {
      int num;	  
	  if (sscanf(input, "%c=%d", &C, &num)==2)
	  {
	    if (C=='a') my_struct.a = (int32_t)num;
	    if (C=='b') my_struct.b = (int16_t)num;
	    if (C=='c') my_struct.c = (int8_t)num;
	    if (C=='d') my_struct.d = (int8_t)num;
	    // Write the current structure to the file...
	    g_file.fWriteAt(0, &my_struct, sizeof(my_struct));
	    // Display the new values
	    show();    
	  }
	  idx = 0;
	 }
  }
}