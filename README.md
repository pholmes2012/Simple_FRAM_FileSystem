# Simple_FRAM_FileSystem
An Arduino library that is a small footprint embedded style file system for FRAM breakouts, supporting both SPI and I2C.

Features:

 Create, Open, Read and Write to files.
 
 Small footprint, currently 4K of program flash and 500 bytes of RAM
 
 Efficient, file reads and writes are direct to FRAM, no caching or background process is used.
 
 No fragmentation is possible.

 All 32bit file operations.
 
 The SPI driver is capable of supporting 24bit and 32bit FRAM chips, when they are made available.
 
 The I2C driver is capable of supporting 1mbit (17bit) I2C FRAM chips.
 
 
Limitations:
 
 Files are created with a maximum possible size, the initial file size (fSize) will be 0 then the
 file can be written to until it grows to its maximum size. At any time data can be read/written from any
 offset within the file, where the offset is < fSize().
 
 Once a file is created in a file system it can not be deleted from the file system (but a new
 file system can be created deleting all existing files).
 
 No directories or folders.
 
 
Uses:
   
  This file system was designed primarily for frequent backing up of runtime structures to non-volatile
  storage, this could be done in the following way:
  ```
  if (file.fOpen("Structure1")==false)
  {
    file.fCreate("Structure1", sizeof(my_structure));
    file.fWrite(&my_structure, sizeof(my_structure));
  }
  file.fReadAt(0, &my_structure, sizeof(my_structure));
  ``` 
  See the example sketches for full working examples.
 
 
SFFS_Volume API:
```
// begin(uint8 deviceAddress);             // Initialise the SFFS with an I2C FRAM device
// begin(uint8 csPin, uint8 addressWidth); // Initialise the SFFS with an SPI FRAM device
// VolumeName();                           // Return the volume name if one exists, or NULL if not
// VolumeCreate(char* volumeName);         // Create a new volume, overwrite if one already exists
// VolumeSize();                           // Return the total size of the FRAM
// VolumeFree();                           // Return the size of free storage available for files
```

SFFS_File API:
```
// fCreate(char* fileName, uint32 maxSize) // Create a file with a name and a maximum size it can grow to
// fOpen(char* fileName);                  // Open an existing file, or return false if the file does not exist 
// fOpen(uint idx);                        // Open a file at idx, or return false if fewer than idx+1 files exists
// fClose();                               // Close an open file
// fSeek(uint32 fileOffset);               // Seek to a position in a file, fail if out of bounds, return current position either way
// fTell();                                // Return the current read/write position in a file  
// fRead(uin8* buffer, uint32 count);      // Read in data from the current file position  	
// fWrite(uin8* buffer, uint32 count);     // Write out data starting at the current file position 
// fReadAt(uint32 fileOffset, uin8* buffer, uint32 count); // Read in data after seeking to a file position
// fWriteAt(uint32 fileOffset, uin8* buffer, uint32 count); // Write out data after seeking to a file position 
```
