/*

  MicroMod Asset Tracker Example
  ==============================

  Ping

  Written by: Paul Clark
  Date: November 17th 2020

  This example uses the SARA's mobile data connection to ping a server.

  The pins and ports are defined in AssetTrackerPins.ino.

  Please make sure that you have selected the correct Board using the Tools\Board menu:
  
  Please add these lines to your File\Preferences\Additional Boards Manager URLs:
  https://raw.githubusercontent.com/sparkfun/Arduino_Boards/master/IDE_Board_Manager/package_sparkfun_index.json
  https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
  Then:
  SparkFun Artemis MicroMod: Click here to get the boards: http://boardsmanager/All#SparkFun_Apollo3
  SparkFun SAMD51 MicroMod : Click here to get the boards: http://boardsmanager/All#Arduino_SAMD_Boards plus http://boardsmanager/All#SparkFun_SAMD_Boards
  SparkFun ESP32 MicroMod  : Click here to get the boards: http://boardsmanager/All#ESP32 (Please install the Espressif ESP32 boards _and_ the SparkFun ESP32 boards)

  Special note for the ESP32:
    If you are using the ESP32 Processor Board, you must open the G3/IMU_PWR and G4/RI split pads on the rear of the PCB
    otherwise the PB will not be able to communicate with the SARA via serial.

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

#include <IPAddress.h>

// SARA-R5

#include "AssetTrackerPins.h" // Include the Asset Tracker pin and port definitions

#define SERIAL_PORT Serial // This is the console serial port - change this if required

#include <SparkFun_u-blox_SARA-R5_Arduino_Library.h> //Click here to get the library: http://librarymanager/All#SparkFun_u-blox_SARA-R5_Arduino_Library

// Create a SARA_R5 object to use throughout the sketch. Pass it the power pin number.
SARA_R5 assetTracker(SARA_PWR);

String pingMe = ""; // The name of the server we are going to ping

// processPingResult is provided to the SARA-R5 library via a 
// callback setter -- setPingCallback. (See the end of setup())
void processPingResult(int retry, int p_size, String remote_hostname, IPAddress ip, int ttl, long rtt)
{
  Serial.println();
  Serial.print(F("Ping Result:  Retry #:"));
  Serial.print(retry);
  Serial.print(F("  Ping Size (Bytes):"));
  Serial.print(p_size);
  Serial.print(F("  Remote Host:\""));
  Serial.print(remote_hostname);
  Serial.print(F("\"  IP Address:\""));
  Serial.print(String(ip[0]));
  Serial.print(F("."));
  Serial.print(String(ip[1]));
  Serial.print(F("."));
  Serial.print(String(ip[2]));
  Serial.print(F("."));
  Serial.print(String(ip[3]));
  Serial.print(F("\"  Time To Live (hops):"));
  Serial.print(ttl);
  Serial.print(F("  Round Trip (ms):"));
  Serial.print(rtt);
  Serial.println();
}

void setup()
{
  String currentOperator = "";

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

  // First check to see if we're connected to an operator:
  if (assetTracker.getOperator(&currentOperator) == SARA_R5_SUCCESS)
  {
    SERIAL_PORT.print(F("Connected to: "));
    SERIAL_PORT.println(currentOperator);
  }
  else
  {
    SERIAL_PORT.print(F("The SARA is not yet connected to an operator. Please use the previous examples to connect. Or wait and retry. Freezing..."));
    while (1)
      ; // Do nothing more
  }

  // Deactivate the profile - in case one is already active
  if (assetTracker.performPDPaction(0, SARA_R5_PSD_ACTION_DEACTIVATE) != SARA_R5_SUCCESS)
  {
    SERIAL_PORT.println(F("Warning: performPDPaction (deactivate profile) failed. Probably because no profile was active."));
  }

  // Load the profile from NVM
  if (assetTracker.performPDPaction(0, SARA_R5_PSD_ACTION_LOAD) != SARA_R5_SUCCESS)
  {
    SERIAL_PORT.println(F("performPDPaction (load from NVM) failed! Freezing..."));
    while (1)
      ; // Do nothing more
  }

  // Activate the profile
  if (assetTracker.performPDPaction(0, SARA_R5_PSD_ACTION_ACTIVATE) != SARA_R5_SUCCESS)
  {
    SERIAL_PORT.println(F("performPDPaction (activate profile) failed! Freezing..."));
    while (1)
      ; // Do nothing more
  }

  SERIAL_PORT.println();
  SERIAL_PORT.println(F("*** Set the Serial Monitor line ending to Newline ***"));

  SERIAL_PORT.println();
  SERIAL_PORT.println(F("Enter the name of the server you want to ping (followed by LF / Newline): "));

  // Set a callback to process the Ping result
  assetTracker.setPingCallback(&processPingResult);
}

void loop()
{
  if (SERIAL_PORT.available())
  {
    char c = SERIAL_PORT.read();
    if (c == '\n')
    {
      // Newline received so let's do that ping!
      assetTracker.ping(pingMe); // Use the default parameters
      
      // Use custom parameters
      //int retries = 4; // number of retries
      //int p_size = 32; // packet size (bytes)
      //unsigned long timeout = 5000; // timeout (ms)
      //int ttl = 32; // Time To Live
      //assetTracker.ping(pingMe, retries, p_size, timeout, ttl);
      
      pingMe = ""; // Clear the server name for the next try
    }
    else
    {
      // Add serial characters to the server address
      pingMe += c;
    }
  }
  
  assetTracker.poll(); // Keep processing data from the SARA so we can catch the Ping result
}

void serialWait()
{
  while (SERIAL_PORT.available()) SERIAL_PORT.read();
  while (!SERIAL_PORT.available()) ;
  delay(100);
  while (SERIAL_PORT.available()) SERIAL_PORT.read();
}
