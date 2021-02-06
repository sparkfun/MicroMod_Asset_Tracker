/*

  MicroMod Asset Tracker Example
  ==============================

  microSD

  Written by: Paul Clark
  Date: October 30th 2020

  This example enables power for the microSD and then prints a directory of all files on the card.

  TO DO: figure out how to make sd use SD_and_IMU_SPI instead of SPI.

  The pins and ports are defined in AssetTrackerPins.ino.

  Please make sure that you have selected the correct Board using the Tools\Board menu:
  
  Please add these lines to your File\Preferences\Additional Boards Manager URLs:
  https://raw.githubusercontent.com/sparkfun/Arduino_Boards/master/IDE_Board_Manager/package_sparkfun_index.json
  https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
  Then:
  SparkFun Artemis MicroMod: Click here to get the boards: http://boardsmanager/All#SparkFun_Apollo3
  SparkFun SAMD51 MicroMod : Click here to get the boards: http://boardsmanager/All#Arduino_SAMD_Boards plus http://boardsmanager/All#SparkFun_SAMD_Boards
  SparkFun ESP32 MicroMod  : Click here to get the boards: http://boardsmanager/All#ESP32 (Please install the Espressif ESP32 boards _and_ the SparkFun ESP32 boards)

  Feel like supporting open source hardware?
  Buy a board from SparkFun!
  SparkFun MicroMod Artemis Processor : http://www.sparkfun.com/products/16401
  SparkFun MicroMod SAMD51 Processor  : http://www.sparkfun.com/products/16791
  SparkFun MicroMod ESP32 Processor   : http://www.sparkfun.com/products/16781
  SparkFun MicroMod ATP Carrier Board          : https://www.sparkfun.com/products/16885
  SparkFun MicroMod Data Logging Carrier Board : https://www.sparkfun.com/products/16829

  Licence: MIT
  Please see LICENSE.md for full details

*/

#include "AssetTrackerPins.h" // Include the Asset Tracker pin and port definitions

#define SERIAL_PORT Serial // This is the console serial port - change this if required

#include <SparkFun_u-blox_SARA-R5_Arduino_Library.h> //Click here to get the library: http://librarymanager/All#SparkFun_u-blox_SARA-R5_Arduino_Library

// Create a SARA_R5 object to use throughout the sketch. Pass it the power pin number.
SARA_R5 assetTracker(SARA_PWR);

//microSD Interface
#include <SPI.h>
#include <SdFat.h> //SdFat (FAT32) by Bill Greiman: http://librarymanager/All#SdFat
SdFat sd;

void setup()
{
  initializeAssetTrackerPins(); // Initialize the pins and ports (defined in AssetTrackerPins.ino)

  SERIAL_PORT.begin(115200); // Start the serial console
  SERIAL_PORT.println(F("Asset Tracker Example"));

  delay(100);

  while (SERIAL_PORT.available()) // Make sure the serial RX buffer is empty
    SERIAL_PORT.read();

  SERIAL_PORT.println(F("microSD power is OFF"));
  SERIAL_PORT.println(F("Press any key to continue..."));

  while (!SERIAL_PORT.available()) // Wait for the user to press a key (send any serial character)
    ;

  digitalWrite(LED_BUILTIN, HIGH);

  enableMicroSDPower(); // Enable power for the microSD card

  SD_and_IMU_SPI.begin(); // Begin SPI comms

  delay(1000); // Give the IMU time to start up

  SERIAL_PORT.println(F("microSD power is ON"));

  if (sd.begin(MICROSD_CS, SD_SCK_MHZ(24)) == false) //Standard SdFat
  {
    SERIAL_PORT.println(F("SD init failed (first attempt). Trying again...\r\n"));
    delay(250); //Give SD more time to power up, then try again
    if (sd.begin(MICROSD_CS, SD_SCK_MHZ(24)) == false) //Standard SdFat
    {
      SERIAL_PORT.println(F("SD init failed (second attempt). Is card present? Formatted? Freezing..."));
      digitalWrite(MICROSD_CS, HIGH); //Be sure SD is deselected
      while(1)
        ; // Do nothing more
    }
  }

  //Change to root directory.
  if (sd.chdir() == false)
  {
    Serial.println(F("SD change directory failed!"));
  }

  SERIAL_PORT.println(F("\r\nRoot Directory Listing:"));

  sd.ls("/", LS_DATE | LS_SIZE | LS_R); // Do a recursive LS of the root directory showing file modification dates and sizes

  SERIAL_PORT.println(F("End of Directory"));
}

void loop()
{
  // Nothing to do here...
}
