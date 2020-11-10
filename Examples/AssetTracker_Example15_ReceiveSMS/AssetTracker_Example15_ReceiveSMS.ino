/*

  MicroMod Asset Tracker Example
  ==============================

  Receive SMS

  Written by: Paul Clark
  Date: October 30th 2020

  This example demonstrates how to receive SMS messages using the SARA.

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

int previousUsed = 0; // Store the previous number of used memory locations

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
}

void loop()
{
  // Read the number of used and total messages
  int used;
  int total;
  if (assetTracker.getPreferredMessageStorage(&used, &total) != SARA_R5_SUCCESS)
  {
    SERIAL_PORT.println(F("An error occurred when trying to read ME memory!"));
  }
  else
  {
    if (used > previousUsed) // Has a new message arrived?
    {
      SERIAL_PORT.print(F("Number of used memory locations: "));
      SERIAL_PORT.println(used);
      SERIAL_PORT.print(F("Total number of memory locations: "));
      SERIAL_PORT.println(total);
      SERIAL_PORT.println();

      int memoryLocation = 1;
      int foundMessages = 0;
      // Keep reading until we find all the messages or we reach the end of the memory
      while ((foundMessages < used) && (memoryLocation <= total))
      {
        String unread = "";
        String from = "";
        String dateTime = "";
        String message = "";
        // Read the message from this location. Reading from empty message locations returns an ERROR
        if (assetTracker.readSMSmessage(memoryLocation, &unread, &from, &dateTime, &message) == SARA_R5_SUCCESS)
        {
          if (unread == "REC UNREAD") // Only print new (previously-unread) messages. Comment this line to display all messages.
          {
            SERIAL_PORT.print(F("Message index: "));
            SERIAL_PORT.println(memoryLocation);
            SERIAL_PORT.print(F("Status: "));
            SERIAL_PORT.println(unread);
            SERIAL_PORT.print(F("Originator: "));
            SERIAL_PORT.println(from);
            SERIAL_PORT.print(F("Date and time: "));
            SERIAL_PORT.println(dateTime);
            SERIAL_PORT.println(message);
            SERIAL_PORT.println();
          }
          foundMessages++; // We found a message
        }
        memoryLocation++; // Move on to the next memory location
      }

      previousUsed = used; // Update previousUsed
    }
  }

  delay(5000); // Check for new messages every 5 seconds
}

void serialWait()
{
  while (SERIAL_PORT.available()) SERIAL_PORT.read();
  while (!SERIAL_PORT.available()) ;
  delay(100);
  while (SERIAL_PORT.available()) SERIAL_PORT.read();
}
