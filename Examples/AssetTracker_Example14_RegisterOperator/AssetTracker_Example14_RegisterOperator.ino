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

// MNO_GLOBAL is the factory-programmed default
// If you are in Europe, you may find no operators unless you choose MNO_STD_EUROPE
const mobile_network_operator_t MOBILE_NETWORK_OPERATOR = MNO_STD_EUROPE;

const String MOBILE_NETWORK_STRINGS[] = {"default (Undefined/Regulatory)", "SIM ICCID", "AT&T", "Verizon", 
  "Telstra", "T-Mobile US", "China Telecom", "Sprint", "Vodafone", "NTT DoCoMo", "Telus", "SoftBank",
  "Deutsche Telekom", "US Cellular", "SKT", "global (factory default)", "standard Europe",
  "standard Europe No-ePCO", "NOT RECOGNIZED"};

// Convert the operator number into an index for MOBILE_NETWORK_STRINGS
int convertOperatorNumber( mobile_network_operator_t mno)
{
  switch (mno)
  {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
      return ((int)mno);
      break;
    case 8:
      return 7;
      break;
    case 19:
      return 8;
      break;
    case 20:
      return 9;
      break;
    case 21:
      return 10;
      break;
    case 28:
      return 11;
      break;
    case 31:
      return 12;
      break;
    case 32:
      return 13;
      break;
    case 39:
      return 14;
      break;
    case 90:
      return 15;
      break;
    case 100:
      return 16;
      break;
    case 101:
      return 17;
      break;
    default: // NOT RECOGNIZED
      return 18;
      break;
  }
}

// APN -- Access Point Name. Gateway between GPRS MNO
// and another computer network. E.g. "hologram" or "internet"
//const String APN = "hologram";
const String APN = "internet";

// The APN can be omitted: this is the so-called "blank APN" setting that may be suggested by
// network operators (e.g. to roaming devices); in this case the APN string is not included in
// the message sent to the network.
//const String APN = "";

// This defines the size of the ops struct array. To narrow the operator
// list, set MOBILE_NETWORK_OPERATOR to AT&T, Verizon etc. instead
// of MNO_SW_DEFAULT.
#define MAX_OPERATORS 10

// Uncomment this line if you want to be able to communicate directly with the SARA in the main loop
//#define DEBUG_PASSTHROUGH_ENABLED

void setup()
{
  int opsAvailable;
  struct operator_stats ops[MAX_OPERATORS];
  String currentOperator = "";
  bool newConnection = true;

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

  assetTracker.enableDebugging(SERIAL_PORT); // Uncomment this line to enable helpful debug messages

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

  // First check to see if we're already connected to an operator:
  if (assetTracker.getOperator(&currentOperator) == SARA_R5_SUCCESS) {
    SERIAL_PORT.print(F("Already connected to: "));
    SERIAL_PORT.println(currentOperator);
    // If already connected provide the option to type y to connect to new operator
    SERIAL_PORT.println(F("Press y to connect to a new operator, or any other key to continue.\r\n"));
    while (!SERIAL_PORT.available()) ;
    if (SERIAL_PORT.read() != 'y')
    {
      newConnection = false;
    }
    while (SERIAL_PORT.available()) SERIAL_PORT.read();
  }

  if (newConnection) {
    // Set MNO to either Verizon, T-Mobile, AT&T, Telstra, etc.
    // This will narrow the operator options during our scan later
    SERIAL_PORT.println(F("Setting mobile-network operator"));
    if (assetTracker.setNetwork(MOBILE_NETWORK_OPERATOR))
    {
      SERIAL_PORT.print(F("Set mobile network operator to "));
      SERIAL_PORT.println(MOBILE_NETWORK_STRINGS[convertOperatorNumber(MOBILE_NETWORK_OPERATOR)] + "\r\n");
    }
    else
    {
      SERIAL_PORT.println(F("Error setting MNO. Try cycling the power. Freezing..."));
      while (1) ;
    }
    
    // Set the APN -- Access Point Name -- e.g. "hologram"
    SERIAL_PORT.println(F("Setting APN..."));
    if (assetTracker.setAPN(APN) == SARA_R5_SUCCESS)
    {
      SERIAL_PORT.println(F("APN successfully set.\r\n"));
    }
    else
    {
      SERIAL_PORT.println(F("Error setting APN. Try cycling the power. Freezing..."));
      while (1) ;
    }

    // Wait for user to press button before initiating network scan.
    SERIAL_PORT.println(F("Press any key scan for networks.."));
    serialWait();

    SERIAL_PORT.println(F("Scanning for operators...this may take up to 3 minutes\r\n"));
    // assetTracker.getOperators takes in a operator_stats struct pointer and max number of
    // structs to scan for, then fills up those objects with operator names and numbers
    opsAvailable = assetTracker.getOperators(ops, MAX_OPERATORS); // This will block for up to 3 minutes

    if (opsAvailable > 0)
    {
      // Pretty-print operators we found:
      SERIAL_PORT.println("Found " + String(opsAvailable) + " operators:");
      printOperators(ops, opsAvailable);

      // Wait until the user presses a key to initiate an operator connection
      SERIAL_PORT.println("Press 1-" + String(opsAvailable) + " to select an operator.");
      char c = 0;
      bool selected = false;
      while (!selected) {
        while (!SERIAL_PORT.available()) ;
        c = SERIAL_PORT.read();
        int selection = c - '0';
        if ((selection >= 1) && (selection <= opsAvailable)) {
          selected = true;
          SERIAL_PORT.println("Connecting to option " + String(selection));
          if (assetTracker.registerOperator(ops[selection - 1]) == SARA_R5_SUCCESS)
          {
            SERIAL_PORT.println("Network " + ops[selection - 1].longOp + " registered\r\n");
          }
          else
          {
            SERIAL_PORT.println(F("Error connecting to operator. Reset and try again, or try another network."));
          }
        }
      }
    }
    else
    {
      SERIAL_PORT.println(F("Did not find an operator. Double-check SIM and antenna, reset and try again, or try another network."));
      while (1) ;
    }
  }

  // At the very end print connection information
  printInfo();
}

void loop()
{
  // Loop won't do much besides provide a debugging interface.
  // Pass serial data from Arduino to shield and vice-versa
  if (SARA_Serial.available()) {
    SERIAL_PORT.write((char) SARA_Serial.read());
  }
#ifdef DEBUG_PASSTHROUGH_ENABLED
  if (SERIAL_PORT.available()) {
    SARA_Serial.write((char) SERIAL_PORT.read());
  }
#endif
}

void printInfo(void) {
  String currentApn = "";
  IPAddress ip(0, 0, 0, 0);
  String currentOperator = "";

  SERIAL_PORT.println(F("Connection info:"));
  // APN Connection info: APN name and IP
  if (assetTracker.getAPN(&currentApn, &ip) == SARA_R5_SUCCESS) {
    SERIAL_PORT.println("APN: " + String(currentApn));
    SERIAL_PORT.print("IP: ");
    SERIAL_PORT.println(ip);
  }

  // Operator name or number
  if (assetTracker.getOperator(&currentOperator) == SARA_R5_SUCCESS)
  {
    SERIAL_PORT.print(F("Operator: "));
    SERIAL_PORT.println(currentOperator);
  }

  // Received signal strength
  SERIAL_PORT.println("RSSI: " + String(assetTracker.rssi()));
  SERIAL_PORT.println();
}

void printOperators(struct operator_stats * ops, int operatorsAvailable)
{
  for (int i = 0; i < operatorsAvailable; i++)
  {
    SERIAL_PORT.print(String(i + 1) + ": ");
    SERIAL_PORT.print(ops[i].longOp + " (" + String(ops[i].numOp) + ") - ");
    switch (ops[i].stat)
    {
    case 0:
      SERIAL_PORT.print(F("UNKNOWN"));
      break;
    case 1:
      SERIAL_PORT.print(F("AVAILABLE"));
      break;
    case 2:
      SERIAL_PORT.print(F("CURRENT"));
      break;
    case 3:
      SERIAL_PORT.print(F("FORBIDDEN"));
      break;
    }
    switch (ops[i].act)
    {
    // SARA-R5 only supports LTE
    case 7:
      SERIAL_PORT.print(F(" - LTE"));
      break;
    }
  }
  SERIAL_PORT.println();
}

void serialWait()
{
  while (SERIAL_PORT.available()) SERIAL_PORT.read();
  while (!SERIAL_PORT.available()) ;
  delay(100);
  while (SERIAL_PORT.available()) SERIAL_PORT.read();
}
