/*

  MicroMod Asset Tracker Example
  ==============================

  u-blox AssistNow Offline

  Written by: Paul Clark
  Date: January 15th 2022

  This example uses the SARA's mobile data connection to:
    * Request AssistNow Offline data from the u-blox server
    * Provide assistance data to an external u-blox GNSS module over I2C (not to the one built-in to the SARA-R510M8S)

  The PDP profile is read from NVM. Please make sure you have run examples 14 & 17 previously to set up the profile.

  This example uses UTC time from the SARA-R5's Real Time Clock to select the AssistNow Offline data for today.

  You will need to have a token to be able to access Thingstream. See the AssistNow README for more details:
  https://github.com/sparkfun/SparkFun_u-blox_GNSS_Arduino_Library/tree/main/examples/AssistNow

  Update secrets.h with your AssistNow token string

  Note: this code does not use the AssistNow or CellLocate features built-in to the SARA-R510M8S.
        Those features are great but the assistance data remains 'hidden' and cannot be read and passed to an external GNSS.
        This code downloads the assistance data to the SARA-R5's internal file system where it can be accessed,
        used and re-used with an external GNSS.

  Note: AssistNow Offline is not supported by the ZED-F9P! "The ZED-F9P supports AssistNow Online only."

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
  SparkFun MicroMod Asset Tracker     : https://www.sparkfun.com/products/17272
  SparkFun MicroMod Artemis Processor : http://www.sparkfun.com/products/16401
  SparkFun MicroMod SAMD51 Processor  : http://www.sparkfun.com/products/16791
  SparkFun MicroMod ESP32 Processor   : http://www.sparkfun.com/products/16781

  Licence: MIT
  Please see LICENSE.md for full details

*/

#include "AssetTrackerPins.h" // Include the Asset Tracker pin and port definitions

#define SERIAL_PORT Serial // This is the console serial port - change this if required

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include <SparkFun_u-blox_SARA-R5_Arduino_Library.h> //Click here to get the library: http://librarymanager/All#SparkFun_u-blox_SARA-R5_Arduino_Library

// Create a SARA_R5 object to use throughout the sketch. Pass it the power pin number.
SARA_R5 assetTracker(SARA_PWR);

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include <SparkFun_u-blox_GNSS_Arduino_Library.h> //http://librarymanager/All#SparkFun_u-blox_GNSS
SFE_UBLOX_GNSS myGNSS;

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

void setup()
{
  initializeAssetTrackerPins(); // Initialize the pins and ports (defined in AssetTrackerPins.ino)

  SERIAL_PORT.begin(115200); // Start the serial console

  // Wait for user to press key to begin
  SERIAL_PORT.println(F("SARA-R5 Example"));
  SERIAL_PORT.println(F("Wait for the SARA NI LED to light up - then press any key to begin"));
  
  while (!SERIAL_PORT.available()) // Wait for the user to press a key (send any serial character)
    ;
  while (SERIAL_PORT.available()) // Empty the serial RX buffer
    SERIAL_PORT.read();

  //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  //assetTracker.enableDebugging(); // Uncomment this line to enable helpful debug messages on Serial

  // For the MicroMod Asset Tracker, we need to invert the power pin so it pulls high instead of low
  // Comment the next line if required
  assetTracker.invertPowerPin(true); 

  // Disable the automatic time zone so we can use UTC. We need to do this _before_ .begin
  assetTracker.autoTimeZoneForBegin(false);

  // Initialize the SARA
  if (assetTracker.begin(SARA_Serial, 115200) )
  {
    SERIAL_PORT.println(F("SARA-R5 connected!"));
  }
  else
  {
    SERIAL_PORT.println(F("Unable to communicate with the SARA."));
    SERIAL_PORT.println(F("Manually power-on (hold the SARA On button for 3 seconds) on and try again."));
    while (1) ; // Loop forever on fail
  }
  SERIAL_PORT.println();

  // First check to see if we're connected to an operator:
  String currentOperator = "";
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

  // Activate the profile
  if (assetTracker.performPDPaction(0, SARA_R5_PSD_ACTION_ACTIVATE) != SARA_R5_SUCCESS)
  {
    SERIAL_PORT.println(F("performPDPaction (activate profile) failed! Freezing..."));
    while (1)
      ; // Do nothing more
  }

  //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  //Get the time from an NTP server and use it to set the clock. See SARA-R5_NTP.ino
  uint8_t y, mo, d, h, min, s;
  bool success = getNTPTime(&y, &mo, &d, &h, &min, &s);
  if (!success)
  {
    SERIAL_PORT.println(F("getNTPTime failed! Freezing..."));
    while (1)
      ; // Do nothing more
  }

  //Set the SARA's RTC. Set the time zone to zero so the clock uses UTC
  if (assetTracker.setClock(y, mo, d, h, min, s, 0) != SARA_R5_SUCCESS)
  {
    SERIAL_PORT.println(F("setClock failed! Freezing..."));
    while (1)
      ; // Do nothing more
  }
  
  //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  // Read and print the clock as a String
  SERIAL_PORT.print(F("The UTC time is: "));
  String theTime = assetTracker.clock();
  SERIAL_PORT.println(theTime);

  //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  // Start I2C. Connect to the GNSS.

  Wire.begin(); //Start I2C

  // Uncomment the next line to enable the 'major' GNSS debug messages on Serial so you can see what AssistNow data is being sent
  //myGNSS.enableDebugging(Serial, true);

  if (myGNSS.begin() == false) //Connect to the Ublox module using Wire port
  {
    SERIAL_PORT.println(F("u-blox GPS not detected at default I2C address. Please check wiring. Freezing."));
    while (1);
  }
  SERIAL_PORT.println(F("u-blox module connected"));

  myGNSS.setI2COutput(COM_TYPE_UBX); //Turn off NMEA noise

  //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  // Request the AssistNow data from the server. Data is stored in the SARA's file system.
  
  String theFilename = "assistnow_offline.ubx"; // The file that will contain the AssistNow Offline data

///* Comment from here ===>
  if (getAssistNowOfflineData(theFilename) == false) // See SARA-R5_AssistNow_Offline.ino
  {
    SERIAL_PORT.println(F("getAssistNowOfflineData failed! Freezing..."));
    while (1)
      ; // Do nothing more    
  }
//*/ // <=== to here if you want to re-use the existing AssistNow Offline data

  //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  // Read the AssistNow data from file

  // Read the data from file
  String theAssistData = "";
  if (assetTracker.getFileContents(theFilename, &theAssistData) != SARA_R5_SUCCESS)
  {
    SERIAL_PORT.println(F("getFileContents failed! Freezing..."));
    while (1)
      ; // Do nothing more    
  }

  //prettyPrintString(theAssistData); // Uncomment this line to see the whole file contents (including the HTTP header)

  //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  // Read UTC time from the SARA-R5's RTC

  uint8_t year, month, day, hours, minutes, seconds;
  int8_t timeZone; // Should be zero for UTC
  if (assetTracker.clock(&year, &month, &day, &hours, &minutes, &seconds, &timeZone) != SARA_R5_SUCCESS)
  {
    SERIAL_PORT.println(F("clock failed! Freezing..."));
    while (1)
      ; // Do nothing more        
  }

  if (year < 22)
  {
    SERIAL_PORT.print(F("WARNING: the SARA-R5's clock time is: "));
    SERIAL_PORT.print(assetTracker.clock());
    SERIAL_PORT.println(F(". Did you forget to set the clock to UTC?"));       
  }

  if (timeZone != 0)
  {
    SERIAL_PORT.print(F("WARNING: the SARA-R5's time zone is: "));
    if (timeZone >= 0) SERIAL_PORT.println(F("+"));
    SERIAL_PORT.print(timeZone);
    SERIAL_PORT.println(F(". Did you forget to set the clock to UTC?"));
  }

  //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  
  // Find where the AssistNow data for today starts and ends

  size_t todayStart = 0; // Default to sending all the data
  size_t tomorrowStart = (size_t)theAssistData.length();
  
  if (theAssistData.length() > 0)
  {
    // Find the start of today's data
    todayStart = myGNSS.findMGAANOForDate(theAssistData, (size_t)theAssistData.length(), year + 2000, month, day);
    if (todayStart < (size_t)theAssistData.length())
    {
      SERIAL_PORT.print(F("Found the data for today starting at location "));
      SERIAL_PORT.println(todayStart);
    }
    else
    {
      SERIAL_PORT.println("Could not find the data for today. This will not work well. The GNSS needs help to start up quickly.");
    }
    
    // Find the start of tomorrow's data
    tomorrowStart = myGNSS.findMGAANOForDate(theAssistData, (size_t)theAssistData.length(), year + 2000, month, day, 1);
    if (tomorrowStart < (size_t)theAssistData.length())
    {
      SERIAL_PORT.print(F("Found the data for tomorrow starting at location "));
      SERIAL_PORT.println(tomorrowStart);
    }
    else
    {
      SERIAL_PORT.println("Could not find the data for tomorrow. (Today's data may be the last?)");
    }
  }

  //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  
  // Push the RTC time to the module

  assetTracker.clock(&year, &month, &day, &hours, &minutes, &seconds, &timeZone); // Refresh the time

  // setUTCTimeAssistance uses a default time accuracy of 2 seconds which should be OK here.
  // Have a look at the library source code for more details.
  myGNSS.setUTCTimeAssistance(year + 2000, month, day, hours, minutes, seconds); // Push it to the GNSS module

  //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  
  // Push the AssistNow data for today to the module - without the time

  if ((tomorrowStart - todayStart) > 0)
  {
    // Tell the module to return UBX_MGA_ACK_DATA0 messages when we push the AssistNow data
    myGNSS.setAckAiding(1);

    // Speed things up by setting setI2CpollingWait to 1ms
    myGNSS.setI2CpollingWait(1);

    // Push the AssistNow data for today - without the time
    //
    // pushAssistNowData is clever and will only push valid UBX-format data.
    // It will ignore the HTTP header at the start of the AssistNow file.
    myGNSS.pushAssistNowData(todayStart, true, theAssistData, tomorrowStart - todayStart, SFE_UBLOX_MGA_ASSIST_ACK_YES, 100);

    // Set setI2CpollingWait to 125ms to avoid pounding the I2C bus
    myGNSS.setI2CpollingWait(125);
  }

  //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  
  // Delete the file after use. This is optional as the SARA will automatically overwrite the file.
  // And you might want to reuse it? AssistNow Offline data is valid for up to 35 days.

  //if (assetTracker.deleteFile(theFilename) != SARA_R5_SUCCESS)
  //{
  //  SERIAL_PORT.println(F("Warning: deleteFile failed!"));
  //}  
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

void loop()
{
  // Print the UBX-NAV-PVT data so we can see how quickly the fixType goes to 3D
  
  long latitude = myGNSS.getLatitude();
  SERIAL_PORT.print(F("Lat: "));
  SERIAL_PORT.print(latitude);

  long longitude = myGNSS.getLongitude();
  SERIAL_PORT.print(F(" Long: "));
  SERIAL_PORT.print(longitude);
  SERIAL_PORT.print(F(" (degrees * 10^-7)"));

  long altitude = myGNSS.getAltitude();
  SERIAL_PORT.print(F(" Alt: "));
  SERIAL_PORT.print(altitude);
  SERIAL_PORT.print(F(" (mm)"));

  byte SIV = myGNSS.getSIV();
  SERIAL_PORT.print(F(" SIV: "));
  SERIAL_PORT.print(SIV);

  byte fixType = myGNSS.getFixType();
  SERIAL_PORT.print(F(" Fix: "));
  if(fixType == 0) SERIAL_PORT.print(F("No fix"));
  else if(fixType == 1) SERIAL_PORT.print(F("Dead reckoning"));
  else if(fixType == 2) SERIAL_PORT.print(F("2D"));
  else if(fixType == 3) SERIAL_PORT.print(F("3D"));
  else if(fixType == 4) SERIAL_PORT.print(F("GNSS + Dead reckoning"));
  else if(fixType == 5) SERIAL_PORT.print(F("Time only"));

  SERIAL_PORT.println();
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
