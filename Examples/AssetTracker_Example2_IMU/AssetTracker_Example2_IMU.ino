/*

  MicroMod Asset Tracker Example
  ==============================

  IMU

  Written by: Paul Clark
  Date: October 30th 2020

  This example enables power for the IMU and then does a basic read of the Accelerometer, Magnetometer and Gyro readings.

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

#include "ICM_20948.h"  // Click here to get the library: http://librarymanager/All#SparkFun_ICM_20948_IMU

ICM_20948_SPI myICM;  // Create an ICM_20948_SPI object

#define SERIAL_PORT Serial // This is the console serial port - change this if required

#include <SparkFun_u-blox_SARA-R5_Arduino_Library.h> //Click here to get the library: http://librarymanager/All#SparkFun_u-blox_SARA-R5_Arduino_Library

// Create a SARA_R5 object to use throughout the sketch. Pass it the power pin number.
SARA_R5 assetTracker(SARA_PWR);

void setup()
{
  initializeAssetTrackerPins(); // Initialize the pins and ports (defined in AssetTrackerPins.ino)

  SERIAL_PORT.begin(115200); // Start the serial console
  SERIAL_PORT.println(F("Asset Tracker Example"));

  delay(100);

  while (SERIAL_PORT.available()) // Make sure the serial RX buffer is empty
    SERIAL_PORT.read();

  SERIAL_PORT.println(F("IMU power is OFF"));
  SERIAL_PORT.println(F("Press any key to continue..."));

  while (!SERIAL_PORT.available()) // Wait for the user to press a key (send any serial character)
    ;

  digitalWrite(LED_BUILTIN, HIGH);

  enableIMUPower(); // Enable power for the IMU
  enableMicroSDPower(); // Enable power for the microSD card too - otherwise we have SPI communication problems

  SD_and_IMU_SPI.begin(); // Begin SPI comms

  delay(1000); // Give the IMU time to start up

  SERIAL_PORT.println(F("IMU power is ON"));
  
  bool initialized = false;
  while( !initialized )
  {
    myICM.begin(IMU_CS, SD_and_IMU_SPI); // Start the IMU using SPI

    SERIAL_PORT.print( F("Initialization of the IMU returned: ") );
    SERIAL_PORT.println( myICM.statusString() );
    if( myICM.status != ICM_20948_Stat_Ok )
    {
      SERIAL_PORT.println( "Trying again..." );
      delay(500);
    }
    else
    {
      initialized = true;
      digitalWrite(LED_BUILTIN, HIGH);
    }
  }
}

void loop()
{
  if( myICM.dataReady() )
  {
    myICM.getAGMT();
    printScaledAGMT( myICM.agmt);
    delay(30);
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN)); // Flash the STAT LED
  }
  else
  {
    SERIAL_PORT.println("Waiting for data");
    delay(500);
  }
}

void printFormattedFloat(float val, uint8_t leading, uint8_t decimals){
  float aval = abs(val);
  if(val < 0){
    SERIAL_PORT.print("-");
  }else{
    SERIAL_PORT.print(" ");
  }
  for( uint8_t indi = 0; indi < leading; indi++ ){
    uint32_t tenpow = 0;
    if( indi < (leading-1) ){
      tenpow = 1;
    }
    for(uint8_t c = 0; c < (leading-1-indi); c++){
      tenpow *= 10;
    }
    if( aval < tenpow){
      SERIAL_PORT.print("0");
    }else{
      break;
    }
  }
  if(val < 0){
    SERIAL_PORT.print(-val, decimals);
  }else{
    SERIAL_PORT.print(val, decimals);
  }
}

void printScaledAGMT( ICM_20948_AGMT_t agmt){
  SERIAL_PORT.print("Scaled. Acc (mg) [ ");
  printFormattedFloat( myICM.accX(), 5, 2 );
  SERIAL_PORT.print(", ");
  printFormattedFloat( myICM.accY(), 5, 2 );
  SERIAL_PORT.print(", ");
  printFormattedFloat( myICM.accZ(), 5, 2 );
  SERIAL_PORT.print(" ], Gyr (DPS) [ ");
  printFormattedFloat( myICM.gyrX(), 5, 2 );
  SERIAL_PORT.print(", ");
  printFormattedFloat( myICM.gyrY(), 5, 2 );
  SERIAL_PORT.print(", ");
  printFormattedFloat( myICM.gyrZ(), 5, 2 );
  SERIAL_PORT.print(" ], Mag (uT) [ ");
  printFormattedFloat( myICM.magX(), 5, 2 );
  SERIAL_PORT.print(", ");
  printFormattedFloat( myICM.magY(), 5, 2 );
  SERIAL_PORT.print(", ");
  printFormattedFloat( myICM.magZ(), 5, 2 );
  SERIAL_PORT.print(" ], Tmp (C) [ ");
  printFormattedFloat( myICM.temp(), 5, 2 );
  SERIAL_PORT.print(" ]");
  SERIAL_PORT.println();
}
