/*

  MicroMod Asset Tracker Example
  ==============================

  GNSS Request

  Written by: Paul Clark
  Date: December 28th 2021

  This example how to request GNSS data and receive it via a callback.

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

// SARA-R5

#include "AssetTrackerPins.h" // Include the Asset Tracker pin and port definitions

#define SERIAL_PORT Serial // This is the console serial port - change this if required

#include <SparkFun_u-blox_SARA-R5_Arduino_Library.h> //Click here to get the library: http://librarymanager/All#SparkFun_u-blox_SARA-R5_Arduino_Library

// Create a SARA_R5 object to use throughout the sketch. Pass it the power pin number.
SARA_R5 assetTracker(SARA_PWR);

boolean requestingGPS = false;
unsigned long lastRequest = 0;
#define MIN_REQUEST_INTERVAL 60000 // How often we'll get GPS in loop (in ms)

#define GNSS_REQUEST_TIMEOUT 30 // Time to turn on GNSS module and get a fix (in s)
#define GNSS_REQUEST_ACCURACY 5  // Desired accuracy from GPS module (in meters)

// processGpsRead is provided to the SARA-R5 library via a 
// callback setter -- setGpsReadCallback. (See end of setup())
void processGpsRead(ClockData clck, PositionData gps, 
  SpeedData spd, unsigned long uncertainty) {

  Serial.println();
  Serial.println();
  Serial.println(F("GNSS Data Received"));
  Serial.println(F("=================="));
  Serial.println("Date (MM/DD/YYYY): " + String(clck.date.month) + "/" + 
    String(clck.date.day) + "/" + String(clck.date.year));
  Serial.println("Time: " + String(clck.time.hour) + ":" + 
    String(clck.time.minute) + ":" + String(clck.time.second) + "." + String(clck.time.ms));
  Serial.println("Lat: " + String(gps.lat, 7));
  Serial.println("Lon: " + String(gps.lon, 7));
  Serial.println("Alt: " + String(gps.alt));
  Serial.println("Uncertainty: " + String(uncertainty));
  Serial.println("Speed: " + String(spd.speed));
  Serial.println("Course Over Ground: " + String(spd.cog));
  Serial.println();

  requestingGPS = false;
}

void setup()
{
  initializeAssetTrackerPins(); // Initialize the pins and ports (defined in AssetTrackerPins.ino)

  SERIAL_PORT.begin(115200); // Start the serial console
  SERIAL_PORT.println(F("Asset Tracker Example"));
  SERIAL_PORT.println();
  SERIAL_PORT.println(F("Initializing the SARA-R5..."));
  SERIAL_PORT.println(F("...this may take ~25 seconds if the SARA is off."));
  SERIAL_PORT.println(F("...it may take ~5 seconds if it just turned on."));
  SERIAL_PORT.println();

  delay(100);

  while (SERIAL_PORT.available()) // Make sure the serial RX buffer is empty
    SERIAL_PORT.read();

  SERIAL_PORT.println(F("Press any key to continue..."));

  serialWait(); // Wait for the user to press a key (send any serial character)

  //assetTracker.enableDebugging(SERIAL_PORT); // Uncomment this line to enable helpful debug messages

  assetTracker.invertPowerPin(true); // For the Asset Tracker, we need to invert the power pin so it pulls high instead of low

  // Initialize the SARA using SARA_Serial and 9600 Baud
  if (assetTracker.begin(SARA_Serial, 9600) )
  {
    SERIAL_PORT.println(F("Asset Tracker (SARA-R5) connected!"));
  }
  else
  {
    SERIAL_PORT.println(F("Unable to communicate with the SARA."));
    SERIAL_PORT.println(F("Manually power-on (hold the SARA On button for 3 seconds) on and try again."));
    while (1) ; // Loop forever on fail
  }
  SERIAL_PORT.println();

  // Set a callback to process the Ping result
  assetTracker.setGpsReadCallback(&processGpsRead);
}

void loop()
{
  // Poll as often as possible
  assetTracker.poll();

  if (!requestingGPS) {
    if ((lastRequest == 0) || (lastRequest + MIN_REQUEST_INTERVAL < millis())) {
        Serial.println(F("Requesting GNSS data...this can take up to 10 seconds"));
        if (assetTracker.gpsRequest(GNSS_REQUEST_TIMEOUT, GNSS_REQUEST_ACCURACY) == SARA_R5_SUCCESS) {
          Serial.println(F("GNSS data requested."));
          Serial.println("Wait up to " + String(GNSS_REQUEST_TIMEOUT) + " seconds");
          requestingGPS = true;
          lastRequest = millis();
        } else {
          Serial.println(F("Error requesting GNSS"));
        }
      }
  } else {
    // Print a '.' every ~1 second if requesting GPS data
    // (Hopefully this doesn't mess with poll too much)
    if ((millis() % 1000) == 0) {
      Serial.print('.');
      delay(1);
    }
  }
}

void serialWait()
{
  while (SERIAL_PORT.available()) SERIAL_PORT.read();
  while (!SERIAL_PORT.available()) ;
  delay(100);
  while (SERIAL_PORT.available()) SERIAL_PORT.read();
}
