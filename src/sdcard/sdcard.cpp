#include "sdcard.h"
#include <SPI.h>
#include <SD.h>

// change this to match your SD shield or module;
const int chipSelect = 5;

void initsdcard(void)
{
  Serial.print("Initializing SD card...");

  while (!SD.begin(chipSelect))
  {
    Serial.println("initialization failed!");
    delay(1000);
  }

  Serial.println("initialization done.");
  
  // if (!SD.begin(chipSelect))
  // {
  //   Serial.println("initialization failed!");
  //   return;
  // }
  // Serial.println("initialization done.");
}

void writeLog( const char *log)
{
  File myFile;
  myFile = SD.open("/directory/datalog.csv", FILE_APPEND);

  if (!myFile) // if the file didn't open, print an error:
  {
    Serial.println("error opening datalog.txt");
    return;
  }
  
  myFile.println(log);
  Serial.print("Writing to test.txt...");
  myFile.close();
  Serial.println("done log");
}