/*

  MicroMod Asset Tracker Example
  ==============================

  Qwiic

  Written by: Paul Clark
  Date: October 30th 2020

  This example tests the Qwiic I2C bus by communicating with an external Qwiic Power Switch:
  https://www.sparkfun.com/products/16740

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

#include <Wire.h> // Needed for I2C

#include <SparkFun_Qwiic_Power_Switch_Arduino_Library.h> // Click here to get the library: http://librarymanager/All#SparkFun_Qwiic_Power_Switch

QWIIC_POWER mySwitch;

void setup()
{
  initializeAssetTrackerPins(); // Initialize the pins and ports (defined in AssetTrackerPins.ino)

  SERIAL_PORT.begin(115200); // Start the serial console
  SERIAL_PORT.println(F("Asset Tracker Example"));

  delay(100);

  while (SERIAL_PORT.available()) // Make sure the serial RX buffer is empty
    SERIAL_PORT.read();

  SERIAL_PORT.println(F("Press any key to continue..."));

  while (!SERIAL_PORT.available()) // Wait for the user to press a key (send any serial character)
    ;

  digitalWrite(LED_BUILTIN, HIGH);

  Wire.begin();

  if (mySwitch.begin() == false) //Connect to the power switch using Wire port
  {
    SERIAL_PORT.println(F("Qwiic Power Switch not detected at default I2C address. Please check wiring. Freezing."));
    while (1)
      ;
  }

  // Configure GPIO1 and GPIO2 as INPUT
  mySwitch.pinMode(1, INPUT);
  mySwitch.pinMode(2, INPUT);

  SERIAL_PORT.println(F("1) Enable power and I2C"));
  SERIAL_PORT.println(F("2) Disable power and I2C"));
  SERIAL_PORT.println(F("3) Enable I2C isolation"));
  SERIAL_PORT.println(F("4) Disable I2C isolation"));
}

void loop()
{
  if (SERIAL_PORT.available())
  {
    byte incoming = SERIAL_PORT.read();

    if (incoming == '1')
    {
      // Switch the power on
      mySwitch.powerOn();
      SERIAL_PORT.println(F("Power is ON. I2C isolation is disabled."));
    }
    else if (incoming == '2')
    {
      // Switch the power off
      mySwitch.powerOff();
      SERIAL_PORT.println(F("Power is OFF. I2C isolation is enabled."));
    }
    else if (incoming == '3')
    {
      // Enable I2C isolation = I2C bus _is_ isolated
      mySwitch.isolationOn();
      SERIAL_PORT.println(F("I2C isolation enabled. I2C is isolated."));
    }
    else if (incoming == '4')
    {
      // Disable I2C isolation = I2C bus _is not_ isolated
      mySwitch.isolationOff();
      SERIAL_PORT.println(F("I2C isolation disabled. I2C is not isolated."));
    }

    // Read and print the GPIO1/GPIO2 state
    SERIAL_PORT.print(F("GPIO1 is: "));
    SERIAL_PORT.println(mySwitch.digitalRead(1));
    SERIAL_PORT.print(F("GPIO2 is: "));
    SERIAL_PORT.println(mySwitch.digitalRead(2));

    // Read any extra Serial bytes (e.g. CR or LF)
    while (SERIAL_PORT.available() > 0)
    {
      SERIAL_PORT.read();
    }
  }
}
