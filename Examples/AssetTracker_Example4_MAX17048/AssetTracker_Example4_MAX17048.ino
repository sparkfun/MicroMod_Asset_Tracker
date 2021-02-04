/*

  MicroMod Asset Tracker Example
  ==============================

  MAX17048

  Written by: Paul Clark
  Date: October 30th 2020

  This example configures and monitors the MAX17048 battery fuel gauge.

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

#include <Wire.h> // Needed for I2C

#include <SparkFun_MAX1704x_Fuel_Gauge_Arduino_Library.h> // Click here to get the library: http://librarymanager/All#SparkFun_MAX1704x_Fuel_Gauge_Arduino_Library

SFE_MAX1704X lipo(MAX1704X_MAX17048); // Create a MAX17048

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

  digitalWrite(LED_BUILTIN, HIGH);

  Wire.begin();

  lipo.enableDebugging(); // Uncomment this line to enable helpful debug messages on Serial

  // Set up the MAX17048 LiPo fuel gauge:
  if (lipo.begin() == false) // Connect to the MAX17048 using the default wire port
  {
    SERIAL_PORT.println(F("MAX17048 not detected. Please check wiring. Freezing."));
    while (1)
      ;
  }

  // Just because we can, let's reset the MAX17048
  SERIAL_PORT.println(F("Resetting the MAX17048..."));
  delay(1000); // Give it time to get its act back together

  // Read and print the reset indicator
  SERIAL_PORT.print(F("Reset Indicator was: "));
  bool RI = lipo.isReset(true); // Read the RI flag and clear it automatically if it is set
  SERIAL_PORT.println(RI); // Print the RI
  // If RI was set, check it is now clear
  if (RI)
  {
    SERIAL_PORT.print(F("Reset Indicator is now: "));
    RI = lipo.isReset(); // Read the RI flag
    SERIAL_PORT.println(RI); // Print the RI    
  }

  // To quick-start or not to quick-start? That is the question!
  // Read the following and then decide if you do want to quick-start the fuel gauge.
  // "Most systems should not use quick-start because the ICs handle most startup problems transparently,
  //  such as intermittent battery-terminal connection during insertion. If battery voltage stabilizes
  //  faster than 17ms then do not use quick-start. The quick-start command restarts fuel-gauge calculations
  //  in the same manner as initial power-up of the IC. If the system power-up sequence is so noisy that the
  //  initial estimate of SOC has unacceptable error, the system microcontroller might be able to reduce the
  //  error by using quick-start."
  // If you still want to try a quick-start then uncomment the next line:
  //lipo.quickStart();

  // Read and print the device ID
  SERIAL_PORT.print(F("Device ID: 0x"));
  uint8_t id = lipo.getID(); // Read the device ID
  if (id < 0x10) SERIAL_PORT.print(F("0")); // Print the leading zero if required
  SERIAL_PORT.println(id, HEX); // Print the ID as hexadecimal

  // Read and print the device version
  SERIAL_PORT.print(F("Device version: 0x"));
  uint8_t ver = lipo.getVersion(); // Read the device version
  if (ver < 0x10) SERIAL_PORT.print(F("0")); // Print the leading zero if required
  SERIAL_PORT.println(ver, HEX); // Print the version as hexadecimal

  // Read and print the battery threshold
  SERIAL_PORT.print(F("Battery empty threshold is currently: "));
  SERIAL_PORT.print(lipo.getThreshold());
  SERIAL_PORT.println(F("%"));

  // We can set an interrupt to alert when the battery SoC gets too low.
  // We can alert at anywhere between 1% and 32%:
  lipo.setThreshold(20); // Set alert threshold to 20%.

  // Read and print the battery empty threshold
  SERIAL_PORT.print(F("Battery empty threshold is now: "));
  SERIAL_PORT.print(lipo.getThreshold());
  SERIAL_PORT.println(F("%"));

  // Read and print the high voltage threshold
  SERIAL_PORT.print(F("High voltage threshold is currently: "));
  float highVoltage = ((float)lipo.getVALRTMax()) * 0.02; // 1 LSb is 20mV. Convert to Volts.
  SERIAL_PORT.print(highVoltage, 2);
  SERIAL_PORT.println(F("V"));

  // Set the high voltage threshold
  lipo.setVALRTMax((float)4.1); // Set high voltage threshold (Volts)

  // Read and print the high voltage threshold
  SERIAL_PORT.print(F("High voltage threshold is now: "));
  highVoltage = ((float)lipo.getVALRTMax()) * 0.02; // 1 LSb is 20mV. Convert to Volts.
  SERIAL_PORT.print(highVoltage, 2);
  SERIAL_PORT.println(F("V"));

  // Read and print the low voltage threshold
  SERIAL_PORT.print(F("Low voltage threshold is currently: "));
  float lowVoltage = ((float)lipo.getVALRTMin()) * 0.02; // 1 LSb is 20mV. Convert to Volts.
  SERIAL_PORT.print(lowVoltage, 2);
  SERIAL_PORT.println(F("V"));

  // Set the low voltage threshold
  lipo.setVALRTMin((float)3.9); // Set low voltage threshold (Volts)

  // Read and print the low voltage threshold
  SERIAL_PORT.print(F("Low voltage threshold is now: "));
  lowVoltage = ((float)lipo.getVALRTMin()) * 0.02; // 1 LSb is 20mV. Convert to Volts.
  SERIAL_PORT.print(lowVoltage, 2);
  SERIAL_PORT.println(F("V"));

  // Enable the State Of Change alert
  SERIAL_PORT.print(F("Enabling the 1% State Of Change alert: "));
  if (lipo.enableSOCAlert())
  {
    SERIAL_PORT.println(F("success."));
  }
  else
  {
    SERIAL_PORT.println(F("FAILED!"));
  }
  
  // Read and print the HIBRT Active Threshold
  SERIAL_PORT.print(F("Hibernate active threshold is: "));
  float actThr = ((float)lipo.getHIBRTActThr()) * 0.00125; // 1 LSb is 1.25mV. Convert to Volts.
  SERIAL_PORT.print(actThr, 5);
  SERIAL_PORT.println(F("V"));

  // Read and print the HIBRT Hibernate Threshold
  SERIAL_PORT.print(F("Hibernate hibernate threshold is: "));
  float hibThr = ((float)lipo.getHIBRTHibThr()) * 0.208; // 1 LSb is 0.208%/hr. Convert to %/hr.
  SERIAL_PORT.print(hibThr, 3);
  SERIAL_PORT.println(F("%/h"));
}

void loop()
{
  // Print the variables:
  SERIAL_PORT.print("Voltage: ");
  SERIAL_PORT.print(lipo.getVoltage());  // Print the battery voltage
  SERIAL_PORT.print("V");

  SERIAL_PORT.print(" Percentage: ");
  SERIAL_PORT.print(lipo.getSOC(), 2); // Print the battery state of charge with 2 decimal places
  SERIAL_PORT.print("%");

  SERIAL_PORT.print(" Change Rate: ");
  SERIAL_PORT.print(lipo.getChangeRate(), 2); // Print the battery change rate with 2 decimal places
  SERIAL_PORT.print("%/hr");

  SERIAL_PORT.print(" Alert: ");
  SERIAL_PORT.print(lipo.getAlert()); // Print the generic alert flag

  SERIAL_PORT.print(" Voltage High Alert: ");
  SERIAL_PORT.print(lipo.isVoltageHigh()); // Print the alert flag

  SERIAL_PORT.print(" Voltage Low Alert: ");
  SERIAL_PORT.print(lipo.isVoltageLow()); // Print the alert flag

  SERIAL_PORT.print(" Empty Alert: ");
  SERIAL_PORT.print(lipo.isLow()); // Print the alert flag

  SERIAL_PORT.print(" SOC 1% Change Alert: ");
  SERIAL_PORT.print(lipo.isChange()); // Print the alert flag
  
  SERIAL_PORT.print(" Hibernating: ");
  SERIAL_PORT.print(lipo.isHibernating()); // Print the alert flag
  
  SERIAL_PORT.println();

  delay(500);
}
