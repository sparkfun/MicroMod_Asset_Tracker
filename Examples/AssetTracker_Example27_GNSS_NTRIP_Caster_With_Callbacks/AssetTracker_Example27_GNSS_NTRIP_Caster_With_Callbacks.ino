/*

  MicroMod Asset Tracker Example
  ==============================

  u-blox GNSS NTRIP Caster Client

  Written by: Paul Clark
  Date: January 15th 2022

  This example uses the SARA's mobile data connection to:
    * Request RTK RTCM data from a NTRIP Caster service
    * Push the RTCM data to an external u-blox GNSS module over I2C (not to the one built-in to the SARA-R510M8S)

  The PDP profile is read from NVM. Please make sure you have run examples 14 & 17 previously to set up the profile.

  Update secrets.h with your NTRIP Caster username and password

  **************************************************************************************************
  * Important Note:                                                                                *
  *                                                                                                *
  * This example pulls kBytes of correction data from the NTRIP Caster.                            *
  * Depending on your location and service provider, the data rate may exceed the allowable        *
  * rates for LTE-M or NB-IoT.                                                                     *
  * Worst case, your service provider may throttle or block the connection - now or in the future. *
  * We are looking for a long-term solution to this - almost certainly using LTE Cat 1 instead.    *
  **************************************************************************************************
  
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

// The ESP32 core has a built in base64 library but not every platform does
// We'll use an external lib if necessary.

#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_APOLLO3) || defined(ARDUINO_ARDUINO_NANO33BLE)
#include "base64.h" //Built-in ESP32 library
#else
#include <Base64.h> //nfriendly library from https://github.com/adamvr/arduino-base64, will work with any platform
#endif

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include <SparkFun_u-blox_SARA-R5_Arduino_Library.h> //Click here to get the library: http://librarymanager/All#SparkFun_u-blox_SARA-R5_Arduino_Library

// Create a SARA_R5 object to use throughout the sketch. Pass it the power pin number.
SARA_R5 assetTracker(SARA_PWR);

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include <SparkFun_u-blox_GNSS_Arduino_Library.h> //http://librarymanager/All#SparkFun_u-blox_GNSS
SFE_UBLOX_GNSS myGNSS;

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// Globals

volatile int socketNum = -1; // The TCP socket number. -1 indicates invalid/closed socket
volatile bool connectionOpen = false; // Flag to indicate if the connection to the NTRIP Caster is open
volatile unsigned long lastReceivedRTCM_ms; // Record when data last arrived - so we can time out if required

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

  // Start communication with the SARA-R5. Load and activate the Packet Switched Data profile.

  //assetTracker.enableDebugging(); // Uncomment this line to enable helpful debug messages on Serial

  // For the MicroMod Asset Tracker, we need to invert the power pin so it pulls high instead of low
  // Comment the next line if required
  assetTracker.invertPowerPin(true); 

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

  // Set a callback to process the results of the PSD Action - OPTIONAL
  assetTracker.setPSDActionCallback(&processPSDAction);

  // Activate the profile
  if (assetTracker.performPDPaction(0, SARA_R5_PSD_ACTION_ACTIVATE) != SARA_R5_SUCCESS)
  {
    SERIAL_PORT.println(F("performPDPaction (activate profile) failed! Freezing..."));
    while (1)
      ; // Do nothing more
  }

  //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  //Print the dynamic IP Address (for profile 0)
  IPAddress myAddress;
  assetTracker.getNetworkAssignedIPAddress(0, &myAddress);
  SERIAL_PORT.print(F("\r\nMy IP Address is: "));
  SERIAL_PORT.print(myAddress[0]);
  SERIAL_PORT.print(F("."));
  SERIAL_PORT.print(myAddress[1]);
  SERIAL_PORT.print(F("."));
  SERIAL_PORT.print(myAddress[2]);
  SERIAL_PORT.print(F("."));
  SERIAL_PORT.println(myAddress[3]);

  //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  // Set a callback to process the socket data
  // This will push the RTCM data to the GNSS
  assetTracker.setSocketReadCallbackPlus(&processSocketData);
  
  //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  // Set a callback to process the socket close
  // 
  // Note: the SARA-R5 only sends a +UUSOCL URC when the socket os closed by the remote
  assetTracker.setSocketCloseCallback(&processSocketClose);
  
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

  myGNSS.setI2COutput(COM_TYPE_UBX | COM_TYPE_NMEA);                                //Set the I2C port to output both NMEA and UBX messages
  myGNSS.setPortInput(COM_PORT_I2C, COM_TYPE_UBX | COM_TYPE_NMEA | COM_TYPE_RTCM3); //Be sure RTCM3 input is enabled. UBX + RTCM3 is not a valid state.

  myGNSS.setDGNSSConfiguration(SFE_UBLOX_DGNSS_MODE_FIXED); // Set the differential mode - ambiguities are fixed whenever possible

  myGNSS.setNavigationFrequency(1); //Set output in Hz.

  // Set the Main Talker ID to "GP". The NMEA GGA messages will be GPGGA instead of GNGGA
  myGNSS.setMainTalkerID(SFE_UBLOX_MAIN_TALKER_ID_GP);

  myGNSS.setNMEAGPGGAcallback(&pushGPGGA); // Set up the callback for GPGGA

  myGNSS.enableNMEAMessage(UBX_NMEA_GGA, COM_PORT_I2C, 10); // Tell the module to output GGA every 10 seconds

  myGNSS.setAutoPVTcallback(&printPVTdata); // Enable automatic NAV PVT messages with callback to printPVTdata so we can watch the carrier solution go to fixed

  //myGNSS.saveConfiguration(VAL_CFG_SUBSEC_IOPORT | VAL_CFG_SUBSEC_MSGCONF); //Optional: Save the ioPort and message settings to NVM

}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

void loop()
{
  myGNSS.checkUblox(); // Check for the arrival of new GNSS data and process it.
  myGNSS.checkCallbacks(); // Check if any GNSS callbacks are waiting to be processed.

  assetTracker.bufferedPoll(); // Process the SARA-R5 URC's. This pushes the incoming RTCM data to the GNSS.

  enum states // Use a 'state machine' to open and close the connection
  {
    open_connection,
    check_connection_and_wait_for_keypress,
    close_connection,
    waiting_for_keypress
  };
  static states state = open_connection;

  //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  switch (state)
  {
    case open_connection:
      SERIAL_PORT.println(F("Connecting to the NTRIP caster..."));
      if (beginClient((int *)&socketNum, (bool *)&connectionOpen)) // Try to open the connection to the caster
      {
        SERIAL_PORT.println(F("Connected to the NTRIP caster! Press any key to disconnect..."));
        state = check_connection_and_wait_for_keypress; // Move on
      }
      else
      {
        SERIAL_PORT.print(F("Could not connect to the caster. Trying again in 5 seconds."));
        for (int i = 0; i < 5; i++)
        {
          delay(1000);
          SERIAL_PORT.print(F("."));
        }
        SERIAL_PORT.println();
      }
      break;

    //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    case check_connection_and_wait_for_keypress:
      // If the connection has dropped or timed out, or if the user has pressed a key
      if ((checkConnection((int)socketNum, (bool)connectionOpen) == false) || (keyPressed()))
      {
        state = close_connection; // Move on
      }
      break;
    
    //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    case close_connection:
      SERIAL_PORT.println(F("Closing the connection to the NTRIP caster..."));
      closeConnection((int *)&socketNum, (bool *)&connectionOpen);
      SERIAL_PORT.println(F("Press any key to reconnect..."));
      state = waiting_for_keypress; // Move on
      break;
    
    //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    case waiting_for_keypress:
      // Wait for the user to press a key
      if (keyPressed())
        state = open_connection; // Move on
      break; 
  }
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
