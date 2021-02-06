/*

  MicroMod Asset Tracker Example
  ==============================

  ThingSpeak (HTTP POST / GET)

  Written by: Paul Clark
  Date: November 17th 2020

  This example uses the SARA's mobile data connection to report the IMU temperature to ThingSpeak using HTTP POST or GET.
  https://thingspeak.com/

  You will need to:
    Create a ThingSpeak User Account – https://thingspeak.com/login
    Create a new Channel by selecting Channels, My Channels, and then New Channel
    Note the Write API Key and copy&paste it into myWriteAPIKey below
  The IMU temperature reading will be added to the channel as "Field 1"

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

#include <IPAddress.h>

// ThingSpeak via HTTP POST / GET

String myWriteAPIKey = "PFIOEXW1VF21T7O6"; // Change this to your API key

String serverName = "api.thingspeak.com"; // Domain Name for HTTP POST Request. ("/update" is added by sendHTTPPOSTdata)

// SARA-R5

#include "AssetTrackerPins.h" // Include the Asset Tracker pin and port definitions

#define SERIAL_PORT Serial // This is the console serial port - change this if required

#include <SparkFun_u-blox_SARA-R5_Arduino_Library.h> //Click here to get the library: http://librarymanager/All#SparkFun_u-blox_SARA-R5_Arduino_Library

// Create a SARA_R5 object to use throughout the sketch. Pass it the power pin number.
SARA_R5 assetTracker(SARA_PWR);

// IMU

#include "ICM_20948.h"  // Click here to get the library: http://librarymanager/All#SparkFun_ICM_20948_IMU

ICM_20948_SPI myICM;  // Create an ICM_20948_SPI object


// processHTTPcommandResult is provided to the SARA-R5 library via a 
// callback setter -- setHTTPCommandCallback. (See the end of setup())
void processHTTPcommandResult(int profile, int command, int result)
{
  Serial.println();
  Serial.print(F("HTTP Command Result:  profile: "));
  Serial.print(profile);
  Serial.print(F("  command: "));
  Serial.print(command);
  Serial.print(F("  result: "));
  Serial.print(result);
  if (result == 0)
    Serial.print(F(" (fail)"));
  if (result == 1)
    Serial.print(F(" (success)"));
  Serial.println();

  // Get and print the most recent HTTP protocol error
  int error_class;
  int error_code;
  assetTracker.getHTTPprotocolError(0, &error_class, &error_code);
  SERIAL_PORT.print(F("Most recent HTTP protocol error:  class: "));
  SERIAL_PORT.print(error_class);
  SERIAL_PORT.print(F("  code: "));
  SERIAL_PORT.print(error_code);
  if (error_code == 0)
    Serial.print(F(" (no error)"));
  SERIAL_PORT.println();

  // Read and print the HTTP POST result
  String postResult = "";
  assetTracker.getFileContents("post_response.txt", &postResult);
  SERIAL_PORT.print(F("HTTP command result was: "));
  SERIAL_PORT.println(postResult);

  SERIAL_PORT.println();
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
    }
  }

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

  // Deactivate the PSD profile - in case one is already active
  if (assetTracker.performPDPaction(0, SARA_R5_PSD_ACTION_DEACTIVATE) != SARA_R5_SUCCESS)
  {
    SERIAL_PORT.println(F("Warning: performPDPaction (deactivate profile) failed. Probably because no profile was active."));
  }

  // Load the PSD profile from NVM - these were saved by a previous example
  if (assetTracker.performPDPaction(0, SARA_R5_PSD_ACTION_LOAD) != SARA_R5_SUCCESS)
  {
    SERIAL_PORT.println(F("performPDPaction (load from NVM) failed! Freezing..."));
    while (1)
      ; // Do nothing more
  }

  // Activate the PSD profile
  if (assetTracker.performPDPaction(0, SARA_R5_PSD_ACTION_ACTIVATE) != SARA_R5_SUCCESS)
  {
    SERIAL_PORT.println(F("performPDPaction (activate profile) failed! Freezing..."));
    while (1)
      ; // Do nothing more
  }

  // Reset HTTP profile 0
  assetTracker.resetHTTPprofile(0);
  
  // Set the server name
  assetTracker.setHTTPserverName(0, serverName);
  
  // Use HTTPS
  assetTracker.setHTTPsecure(0, false); // Setting this to true causes the POST / GET to fail. Not sure why...

  // Set a callback to process the HTTP command result
  assetTracker.setHTTPCommandCallback(&processHTTPcommandResult);
}

void loop()
{
  if( myICM.dataReady() ) // Check that the IMU has data ready
  {
    myICM.getAGMT(); // Read the Accel, Gyr, Mag and Temperature values
    float temperature = myICM.temp(); // Extract the temperature (Degrees C)

    // Send data using HTTP POST
    String httpRequestData = "api_key=" + myWriteAPIKey + "&field1=" + String(temperature);

    SERIAL_PORT.print(F("POSTing a temperature of "));
    SERIAL_PORT.print(String(temperature));
    SERIAL_PORT.println(F(" to ThingSpeak"));
          
    // Send HTTP POST request to /update. The reponse will be written to post_response.txt in the SARA's file system
    assetTracker.sendHTTPPOSTdata(0, "/update", "post_response.txt", httpRequestData, SARA_R5_HTTP_CONTENT_APPLICATION_X_WWW);

//    // Send data using HTTP GET
//    String path = "/update?api_key=" + myWriteAPIKey + "&field1=" + String(temperature);
//
//    SERIAL_PORT.print(F("Send a temperature of "));
//    SERIAL_PORT.print(String(temperature));
//    SERIAL_PORT.println(F(" to ThingSpeak using HTTP GET"));
//          
//    // Send HTTP POST request to /update. The reponse will be written to post_response.txt in the SARA's file system
//    assetTracker.sendHTTPGET(0, path, "post_response.txt");
  }
  
  // Wait for 20 seconds
  for (int i = 0; i < 20000; i++)
  {
    assetTracker.poll(); // Keep processing data from the SARA so we can catch the HTTP command result
    delay(1);
  }
}

void serialWait()
{
  while (SERIAL_PORT.available()) SERIAL_PORT.read();
  while (!SERIAL_PORT.available()) ;
  delay(100);
  while (SERIAL_PORT.available()) SERIAL_PORT.read();
}
