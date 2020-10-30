/*

  MicroMod Asset Tracker Example
  ==============================

  Blink

  Written by: Paul Clark
  Date: October 30th 2020

  This example demonstrates how to initialize the pins on the MicroMod Asset Tracker
  and then blink the Processor Board STAT LED. Exciting stuff!

  The pins and ports are defined in AssetTrackerPins.ino.

  Please make sure that you have selected the correct Board using the Tools\Board menu:
  SparkFun Artemis MicroMod: Click here to get the boards: http://boardsmanager/All#SparkFun_Apollo3
  SparkFun SAMD51 MicroMod : Click here to get the boards: http://boardsmanager/All#Arduino_SAMD_Boards plus http://boardsmanager/All#SparkFun_SAMD_Boards
  SparkFun ESP32 MicroMod  : Click here to get the boards: http://boardsmanager/All#SparkFun_ESP32

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

void setup()
{
  initializeAssetTrackerPins(); // Initialize the pins and ports (defined in AssetTrackerPins.ino)
}

void loop()
{
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN)); // Flash the STAT LED at 0.5Hz (Read-Invert-Write)
  delay(1000);
}
