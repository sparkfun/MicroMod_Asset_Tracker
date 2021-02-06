/*

  MicroMod Asset Tracker Example
  ==============================

  Test SARA EXT_INT Pin

  Written by: Paul Clark
  Date: November 20th 2020

  This example tests the SARA I2C pins using AT+UTEST.
  The test is bi-directional; EXT_INT is tested both as an input and an output.

  ** Note: you will need to manually solder the G5_SARA_INT split pad closed to connect the SARA_INT signal through to the processor pin **  

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

  //assetTracker.enableDebugging(SERIAL_PORT); // Uncomment this line to enable helpful debug messages

  assetTracker.invertPowerPin(true); // For the Asset Tracker, we need to invert the power pin so it pulls high instead of low

  // Initialize the SARA using SARA_Serial and 9600 Baud
  if (assetTracker.begin(SARA_Serial, 9600) )
  {
    SERIAL_PORT.println(F("Asset Tracker (SARA-R5) connected!"));
  }

  digitalWrite(LED_BUILTIN, HIGH);

  // In this example, we want to configure SARA_INT (G5) as an input
  pinMode(SARA_INT, INPUT);

  // Deactivate the protocol stack before entering test mode ("+CFUN=0")
  SARA_R5_error_t result = assetTracker.functionality(MINIMUM_FUNCTIONALITY);
  if (result != SARA_R5_SUCCESS)
  {
    SERIAL_PORT.println(F("Could not enter minimum functionality mode! Freezing..."));
    while (1)
      ; // Do nothing more
  }

  // Put the SARA-R5 into OEM production test mode
  // Send AT+UTEST=1 expecting SARA_R5_RESPONSE_OK ("OK\r\n") in return
  const char enter_utest[] = "+UTEST=1"; // Enter OEM production test mode
  char response[128]; // Storage for the command response
  char *response_ptr = response; // A pointer to the response storage
  result = assetTracker.sendCustomCommandWithResponse(enter_utest, SARA_R5_RESPONSE_OK, response_ptr);
  if (result != SARA_R5_SUCCESS)
  {
    SERIAL_PORT.println(F("Could not enter OEM production test mode! Freezing..."));
    while (1)
      ; // Do nothing more
  }

  // Define EXT_INT (pin 33) as a test pin
  // The pin numbers are big endian
  const char test_pins[] = "+UTEST=10,2,\"000000000000000100000000\""; // Define pin 33 as a test pin
  result = assetTracker.sendCustomCommandWithResponse(test_pins, SARA_R5_RESPONSE_OK, response_ptr);
  if (result != SARA_R5_SUCCESS)
  {
    SERIAL_PORT.println(F("Could not configure the EXT_INT pin for testing! Freezing..."));
    while (1)
      ; // Do nothing more
  }

  // Define the EXT_INT pin as an output
  const char set_io[] = "+UTEST=10,3,\"000000000000000000000000\""; // Define pin 33 as an output
  result = assetTracker.sendCustomCommandWithResponse(set_io, SARA_R5_RESPONSE_OK, response_ptr);
  if (result != SARA_R5_SUCCESS)
  {
    SERIAL_PORT.println(F("Could not configure the EXT_INT pin for I/O! Freezing..."));
    while (1)
      ; // Do nothing more
  }

  // Set the EXT_INT pin HIGH
  const char set_high[] = "+UTEST=10,4,\"000000000000000100000000\""; // Set selected pins high
  result = assetTracker.sendCustomCommandWithResponse(set_high, SARA_R5_RESPONSE_OK, response_ptr);
  if (result != SARA_R5_SUCCESS)
  {
    SERIAL_PORT.println(F("Could not set the EXT_INT pin high! Freezing..."));
    while (1)
      ; // Do nothing more
  }

  // Apply the change
  const char apply_change[] = "+UTEST=10,5"; // Apply the change
  result = assetTracker.sendCustomCommandWithResponse(apply_change, SARA_R5_RESPONSE_OK, response_ptr);
  if (result != SARA_R5_SUCCESS)
  {
    SERIAL_PORT.println(F("Could not apply the pin setting change! Freezing..."));
    while (1)
      ; // Do nothing more
  }

  delay(1);

  // Read the state of the pin - check it is high
  if (digitalRead(SARA_INT) != HIGH) SERIAL_PORT.println(F("ERROR! SARA EXT_INT pin is not high!"));

  // Set the EXT_INT pin LOW
  const char set_low[] = "+UTEST=10,4,\"000000000000000000000000\""; // Set selected pins low
  result = assetTracker.sendCustomCommandWithResponse(set_low, SARA_R5_RESPONSE_OK, response_ptr);
  if (result != SARA_R5_SUCCESS)
  {
    SERIAL_PORT.println(F("Could not set the EXT_INT pin low! Freezing..."));
    while (1)
      ; // Do nothing more
  }

  // Apply the change
  result = assetTracker.sendCustomCommandWithResponse(apply_change, SARA_R5_RESPONSE_OK, response_ptr);
  if (result != SARA_R5_SUCCESS)
  {
    SERIAL_PORT.println(F("Could not apply the pin setting change! Freezing..."));
    while (1)
      ; // Do nothing more
  }

  delay(1);

  // Read the state of the pin - check it is low
  if (digitalRead(SARA_INT) != LOW) SERIAL_PORT.println(F("ERROR! SARA EXT_INT pin is not low!"));

  // Restore the pins to their original state
  const char restore_pins[] = "+UTEST=10,0"; // Restore the pins
  result = assetTracker.sendCustomCommandWithResponse(restore_pins, SARA_R5_RESPONSE_OK, response_ptr);
  if (result != SARA_R5_SUCCESS)
  {
    SERIAL_PORT.println(F("Could not restore the pins! Freezing..."));
    while (1)
      ; // Do nothing more
  }

  // (Re)define EXT_INT (pin 33) as a test pin
  result = assetTracker.sendCustomCommandWithResponse(test_pins, SARA_R5_RESPONSE_OK, response_ptr);
  if (result != SARA_R5_SUCCESS)
  {
    SERIAL_PORT.println(F("Could not configure the EXT_INT pin for testing! Freezing..."));
    while (1)
      ; // Do nothing more
  }

  // Define the EXT_INT pin as an input
  const char set_io_in[] = "+UTEST=10,3,\"000000000000000100000000\""; // Define pin 33 as an input
  result = assetTracker.sendCustomCommandWithResponse(set_io_in, SARA_R5_RESPONSE_OK, response_ptr);
  if (result != SARA_R5_SUCCESS)
  {
    SERIAL_PORT.println(F("Could not configure the EXT_INT pin for I/O! Freezing..."));
    while (1)
      ; // Do nothing more
  }

  // Apply the change
  result = assetTracker.sendCustomCommandWithResponse(apply_change, SARA_R5_RESPONSE_OK, response_ptr);
  if (result != SARA_R5_SUCCESS)
  {
    SERIAL_PORT.println(F("Could not apply the pin setting change! Freezing..."));
    while (1)
      ; // Do nothing more
  }

  // Turn the SARA_INT pin into an output
  pinMode(SARA_INT, OUTPUT);
  digitalWrite(SARA_INT, LOW);

  delay(1);

  // Read the SARA pins
  const char read_pins[] = "+UTEST=10,6"; // Read the pins
  result = assetTracker.sendCustomCommandWithResponse(read_pins, SARA_R5_RESPONSE_OK, response_ptr);
  if (result != SARA_R5_SUCCESS)
  {
    SERIAL_PORT.println(F("Could not read the pins! Freezing..."));
    while (1)
      ; // Do nothing more
  }

  // Check the response
  if (strstr(response,"000000000000000000000000") == NULL)
  {
    SERIAL_PORT.println(F("ERROR! SARA EXT_INT pin is not low!"));
    SERIAL_PORT.print(F("response is: \""));
    SERIAL_PORT.print(response);
    SERIAL_PORT.println(F("\""));
  }
  
  // Set the SARA_INT pin high
  digitalWrite(SARA_INT, HIGH);


  delay(1);

  // Read the SARA pins
  result = assetTracker.sendCustomCommandWithResponse(read_pins, SARA_R5_RESPONSE_OK, response_ptr);
  if (result != SARA_R5_SUCCESS)
  {
    SERIAL_PORT.println(F("Could not read the pins! Freezing..."));
    while (1)
      ; // Do nothing more
  }

  // Check the response
  if (strstr(response,"000000000000000100000000") == NULL)
  {
    SERIAL_PORT.println(F("ERROR! SARA EXT_INT pin is not high!"));
    SERIAL_PORT.print(F("response is: \""));
    SERIAL_PORT.print(response);
    SERIAL_PORT.println(F("\""));
  }
  
  // Restore the pins to their original state
  result = assetTracker.sendCustomCommandWithResponse(restore_pins, SARA_R5_RESPONSE_OK, response_ptr);
  if (result != SARA_R5_SUCCESS)
  {
    SERIAL_PORT.println(F("Could not restore the pins! Freezing..."));
    while (1)
      ; // Do nothing more
  }

  // Exit OEM production test mode
  // Send AT+UTEST=0 expecting SARA_R5_RESPONSE_OK ("OK\r\n") in return
  const char exit_utest[] = "+UTEST=0"; // Exit OEM production test mode
  result = assetTracker.sendCustomCommandWithResponse(exit_utest, SARA_R5_RESPONSE_OK, response_ptr);
  if (result != SARA_R5_SUCCESS)
  {
    SERIAL_PORT.println(F("Could not exit OEM production test mode! Freezing..."));
    while (1)
      ; // Do nothing more
  }

  // Restore SARA to full functionality ("+CFUN=1")
  result = assetTracker.functionality(FULL_FUNCTIONALITY);
  if (result != SARA_R5_SUCCESS)
  {
    SERIAL_PORT.println(F("Could not restore full functionality! Freezing..."));
    while (1)
      ; // Do nothing more
  }

  SERIAL_PORT.println(F("Pin test is complete! Power cycle the SARA..."));
}

void loop()
{
}
