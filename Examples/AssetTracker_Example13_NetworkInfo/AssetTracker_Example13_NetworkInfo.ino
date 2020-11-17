/*

  MicroMod Asset Tracker Example
  ==============================

  Network Info

  Written by: Paul Clark
  Date: October 30th 2020

  This example demonstrates how to register the SARA on the selected network.

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

#include "AssetTrackerPins.h" // Include the Asset Tracker pin and port definitions

#define SERIAL_PORT Serial // This is the console serial port - change this if required

#include <SparkFun_u-blox_SARA-R5_Arduino_Library.h> //Click here to get the library: http://librarymanager/All#SparkFun_u-blox_SARA-R5_Arduino_Library

// Create a SARA_R5 object to use throughout the sketch. Pass it the power pin number.
SARA_R5 assetTracker(SARA_PWR);

// Map registration status messages to more readable strings
String registrationString[] =
{
  "Not registered",                         // 0
  "Registered, home",                       // 1
  "Searching for operator",                 // 2
  "Registration denied",                    // 3
  "Registration unknown",                   // 4
  "Registered, roaming",                    // 5
  "Registered, home (SMS only)",            // 6
  "Registered, roaming (SMS only)",         // 7
  "Registered, emergency service only",     // 8
  "Registered, home, CSFB not preferred",   // 9
  "Registered, roaming, CSFB not prefered"  // 10
};

// Network operator can be set to e.g.:
// MNO_SW_DEFAULT -- DEFAULT (Undefined / regulatory)
// MNO_SIM_ICCID -- SIM DEFAULT
// MNO_ATT -- AT&T 
// MNO_VERIZON -- Verizon
// MNO_TELSTRA -- Telstra
// MNO_TMO -- T-Mobile US
// MNO_CT -- China Telecom
// MNO_SPRINT
// MNO_VODAFONE
// MNO_NTT_DOCOMO
// MNO_TELUS
// MNO_SOFTBANK
// MNO_DT -- Deutsche Telekom
// MNO_US_CELLULAR
// MNO_SKT
// MNO_GLOBAL -- SARA factory-programmed value
// MNO_STD_EUROPE
// MNO_STD_EU_NOEPCO

// If you are based in Europe, you will (probably) need to select MNO_STD_EUROPE

const mobile_network_operator_t MOBILE_NETWORK_OPERATOR = MNO_GLOBAL;

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

  while (!SERIAL_PORT.available()) // Wait for the user to press a key (send any serial character)
    ;

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

  if (!assetTracker.setNetworkProfile(MOBILE_NETWORK_OPERATOR))
  {
    SERIAL_PORT.println(F("Error setting network. Try cycling the power."));
    while (1) ;
  }
  
  SERIAL_PORT.println(F("Network profile set. Ready to go!"));
  
  // RSSI: Received signal strength:
  SERIAL_PORT.println("RSSI: " + String(assetTracker.rssi()));
  // Registration Status
  int regStatus = assetTracker.registration();
  if ((regStatus >= 0) && (regStatus <= 10))
  {
    SERIAL_PORT.println("Network registration: " + registrationString[regStatus]);
  }

  // Print the Context IDs, Access Point Names and IP Addresses
  SERIAL_PORT.println(F("Available PDP (Packet Data Protocol) APNs (Access Point Names) and IP Addresses:"));
  SERIAL_PORT.println(F("Context ID:\tAPN Name:\tIP Address:"));
  for (int cid = 0; cid < SARA_R5_NUM_PDP_CONTEXT_IDENTIFIERS; cid++)
  {
    String apn = "";
    IPAddress ip(0, 0, 0, 0);
    assetTracker.getAPN(cid, &apn, &ip);
    if (apn.length() > 0)
    {
      SERIAL_PORT.print(cid);
      SERIAL_PORT.print(F("\t"));
      SERIAL_PORT.print(apn);
      SERIAL_PORT.print(F("\t"));
      SERIAL_PORT.println(ip);
    }
  }

  SERIAL_PORT.println();
  
  if (regStatus > 0)
  {
    SERIAL_PORT.println(F("All set. Go to the next example!"));
  }
}

void loop()
{
  // Do nothing. Now that we're registered move on to the next example.
}
