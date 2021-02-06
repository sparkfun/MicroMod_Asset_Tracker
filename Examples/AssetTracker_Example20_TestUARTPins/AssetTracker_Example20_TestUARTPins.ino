/*

  MicroMod Asset Tracker Example
  ==============================

  Test UART Pins

  Written by: Paul Clark
  Date: November 20th 2020

  This example tests the UART pins (DSR/RI/DCD/DTR/RTS/CTS) using the AT+UTEST command.
  
  ** Note: this example currently only runs on the MicroMod nRF52840 Processor Board as it supports all of the UART pins **

  ** Note: you will need to manually solder the G4_RI and G7_DSR split pads closed to connect the RI and DSR signals through to the nRF52840 pins **

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
  // begin calls init. init sets: SARA GPIO1 to NETWORK_STATUS, and SARA GPIO6 to TIME_PULSE_OUTPUT
  if (assetTracker.begin(SARA_Serial, 9600) )
  {
    SERIAL_PORT.println(F("Asset Tracker (SARA-R5) connected!"));
  }

  digitalWrite(LED_BUILTIN, HIGH);

  // In this example, we want to configure: DSR, RI, DCD and CTS as inputs; DTR and RTS as outputs
  pinMode(SARA_DSR, INPUT);
  pinMode(SARA_RI, INPUT);
  pinMode(SARA_DCD, INPUT);
  pinMode(SARA_CTS, INPUT);

  pinMode(SARA_RTS, OUTPUT);
  pinMode(SARA_DTR, OUTPUT);

  // Deactivate the protocol stack before entering test mode ("+CFUN=0")
  SARA_R5_error_t result = assetTracker.functionality(MINIMUM_FUNCTIONALITY);
  if (result != SARA_R5_SUCCESS)
  {
    SERIAL_PORT.println(F("Could not enter minimum functionality mode! Freezing..."));
    while (1)
      ; // Do nothing more
  }

  // Disable DTE flow control so we can test the RTS pin without stalling serial communication
  result = assetTracker.setFlowControl(SARA_R5_DISABLE_FLOW_CONTROL);
  if (result != SARA_R5_SUCCESS)
  {
    SERIAL_PORT.println(F("Could not disable flow control! Freezing..."));
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

  // Define the DSR, RI, DCD, DTR, RTS and CTS pins as test pins
  // The pin numbers are 6,7,8,9,10 and 11 (big endian)
  const char test_pins[] = "+UTEST=10,2,\"0000000000000000000007E0\""; // Define pins 6 to 11 as test pins
  result = assetTracker.sendCustomCommandWithResponse(test_pins, SARA_R5_RESPONSE_OK, response_ptr);
  if (result != SARA_R5_SUCCESS)
  {
    SERIAL_PORT.println(F("Could not configure the UART pins for testing! Freezing..."));
    while (1)
      ; // Do nothing more
  }

  // Define the DSR, RI, DCD and CTS pins as outputs. Define DTR and RTS as inputs.
  const char set_io[] = "+UTEST=10,3,\"000000000000000000000300\""; // Define pins 9 and 10 as inputs, the remainder will be outputs
  result = assetTracker.sendCustomCommandWithResponse(set_io, SARA_R5_RESPONSE_OK, response_ptr);
  if (result != SARA_R5_SUCCESS)
  {
    SERIAL_PORT.println(F("Could not configure the UART pins for I/O! Freezing..."));
    while (1)
      ; // Do nothing more
  }

  // Set the DSR, RI, DCD and CTS pins HIGH
  const char set_high[] = "+UTEST=10,4,\"0000000000000000000004E0\""; // Set selected pins high
  result = assetTracker.sendCustomCommandWithResponse(set_high, SARA_R5_RESPONSE_OK, response_ptr);
  if (result != SARA_R5_SUCCESS)
  {
    SERIAL_PORT.println(F("Could not set the UART pins high! Freezing..."));
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

  // Read the state of each input pin - check it is high
  if (digitalRead(SARA_DSR) != HIGH) SERIAL_PORT.println(F("ERROR! SARA DSR pin is not high!"));
  if (digitalRead(SARA_RI) != HIGH) SERIAL_PORT.println(F("ERROR! SARA RI pin is not high!"));
  if (digitalRead(SARA_DCD) != HIGH) SERIAL_PORT.println(F("ERROR! SARA DCD pin is not high!"));
  if (digitalRead(SARA_CTS) != HIGH) SERIAL_PORT.println(F("ERROR! SARA CTS pin is not high!"));

  // Set the DSR, RI, DCD and CTS pins LOW
  const char set_low[] = "+UTEST=10,4,\"000000000000000000000000\""; // Set selected pins low
  result = assetTracker.sendCustomCommandWithResponse(set_low, SARA_R5_RESPONSE_OK, response_ptr);
  if (result != SARA_R5_SUCCESS)
  {
    SERIAL_PORT.println(F("Could not set the UART pins low! Freezing..."));
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

  // Read the state of each pin - check it is low
  if (digitalRead(SARA_DSR) != LOW) SERIAL_PORT.println(F("ERROR! SARA DSR pin is not low!"));
  if (digitalRead(SARA_RI) != LOW) SERIAL_PORT.println(F("ERROR! SARA RI pin is not low!"));
  if (digitalRead(SARA_DCD) != LOW) SERIAL_PORT.println(F("ERROR! SARA DCD pin is not low!"));
  if (digitalRead(SARA_CTS) != LOW) SERIAL_PORT.println(F("ERROR! SARA CTS pin is not low!"));

  // Set the DTR and RTS pins low
  digitalWrite(SARA_DTR, LOW);
  digitalWrite(SARA_RTS, LOW);

  // Read the SARA pins
  const char read_pins[] = "+UTEST=10,6"; // Read the pins
  result = assetTracker.sendCustomCommandWithResponse(read_pins, SARA_R5_RESPONSE_OK, response_ptr);
  if (result != SARA_R5_SUCCESS)
  {
    SERIAL_PORT.println(F("Could not read the UART pins! Freezing..."));
    while (1)
      ; // Do nothing more
  }

  // Check the response
  if (strstr(response,"000000000000000000000000") == NULL)
  {
    SERIAL_PORT.println(F("ERROR! SARA DTR and RTS pins are not low!"));
    SERIAL_PORT.print(F("response is: \""));
    SERIAL_PORT.print(response);
    SERIAL_PORT.println(F("\""));
  }
  
  // Set the DTR and RTS pins high
  digitalWrite(SARA_DTR, HIGH);
  digitalWrite(SARA_RTS, HIGH);

  delay(1);

  // Read the SARA pins
  result = assetTracker.sendCustomCommandWithResponse(read_pins, SARA_R5_RESPONSE_OK, response_ptr);
  if (result != SARA_R5_SUCCESS)
  {
    SERIAL_PORT.println(F("Could not read the UART pins! Freezing..."));
    while (1)
      ; // Do nothing more
  }

  // Check the response
  if (strstr(response,"000000000000000000000300") == NULL)
  {
    SERIAL_PORT.println(F("ERROR! SARA DTR and RTS pins are not high!"));
    SERIAL_PORT.print(F("response is: \""));
    SERIAL_PORT.print(response);
    SERIAL_PORT.println(F("\""));
  }
  
  // Restore the pins to their original state
  const char restore_pins[] = "+UTEST=10,0"; // Restore the pins
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
