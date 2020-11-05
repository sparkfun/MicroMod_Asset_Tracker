/*

  MicroMod Asset Tracker Pin & Port Support Functions
  ===================================================

  Written by: Paul Clark
  Date: October 30th 2020

  The file defines the pins and ports for the MicroMod Asset Tracker.

  The pin and ports are defined using the MicroMod naming scheme.
  This allows the code to run on different MicroMod Processor Boards
  without needing to change the pin numbers.

  Licence: MIT
  Please see the LICENSE.md for full details

*/

// Initialize the I/O pins.
// Default to having everything turned off.
// See below for the enable/disable functions.
void initializeAssetTrackerPins()
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  
  disableMicroSDPower();

  disableIMUPower();

  disableSPIPins();

  disableGNSSAntennaPower();

  digitalWrite(SARA_PWR, LOW); // Make sure SARA_PWR is low before making the pin an output
  pinMode(SARA_PWR, OUTPUT);
  digitalWrite(SARA_PWR, LOW);

  pinMode(SARA_RI, INPUT);
  
  pinMode(SARA_INT, INPUT);

  if (SARA_DSR >= 0) pinMode(SARA_DSR, INPUT);

  if (SARA_ON >= 0) pinMode(SARA_ON, INPUT);

  if (SARA_ON_ALT >= 0) pinMode(SARA_ON_ALT, INPUT);

  pinMode(IMU_INT, INPUT);

  pinMode(VIN_DIV_3, INPUT);

}

// Disable SPI pins - to avoid parasitic power leaking throug the SD card
void disableSPIPins()
{
  pinMode(MICROSD_CS, OUTPUT);
  digitalWrite(MICROSD_CS, LOW); // Pull CS low to reduce current leak through the SD card
  pinMode(IMU_CS, OUTPUT);
  digitalWrite(IMU_CS, LOW); // Pull CS low to reduce sleep current
  pinMode(MISO, OUTPUT);
  digitalWrite(MISO, LOW); // Pull MISO low to reduce current leak through the SD card
  pinMode(MOSI, OUTPUT);
  digitalWrite(MOSI, LOW); // Pull MOSI low to reduce current leak through the SD card
  pinMode(SCK, OUTPUT);
  digitalWrite(SCK, LOW); // Pull SCK low to reduce current leak through the SD card
}
// Disable power for the micro SD card
void disableMicroSDPower()
{
  pinMode(MICROSD_PWR_EN, OUTPUT); // Define the pinMode here in case a sleep function has disabled it
  digitalWrite(MICROSD_PWR_EN, HIGH);
}
// Enable power for the micro SD card
void enableMicroSDPower()
{
  pinMode(MICROSD_PWR_EN, OUTPUT); // Define the pinMode here in case a sleep function has disabled it
  digitalWrite(MICROSD_PWR_EN, LOW);
  pinMode(MICROSD_CS, OUTPUT);
  digitalWrite(MICROSD_CS, HIGH); // Pull CS high to disable the SD card
}

// Disable power for the IMU
void disableIMUPower()
{
  pinMode(IMU_PWR_EN, OUTPUT); // Define the pinMode here in case a sleep function has disabled it
  digitalWrite(IMU_PWR_EN, LOW);
}
// Enable power for the IMU
void enableIMUPower()
{
  pinMode(IMU_PWR_EN, OUTPUT); // Define the pinMode here in case a sleep function has disabled it
  digitalWrite(IMU_PWR_EN, HIGH);
  pinMode(IMU_CS, OUTPUT);
  digitalWrite(IMU_CS, HIGH); // Pull CS high to disable the IMU
}

// Disable power for the GNSS active antenna
void disableGNSSAntennaPower()
{
  pinMode(ANT_PWR_EN, OUTPUT); // Define the pinMode here in case a sleep function has disabled it
  digitalWrite(ANT_PWR_EN, LOW);
}
// Enable power for the GNSS active antenna
void enableGNSSAntennaPower()
{
  pinMode(ANT_PWR_EN, OUTPUT); // Define the pinMode here in case a sleep function has disabled it
  digitalWrite(ANT_PWR_EN, HIGH);
}

// Read VIN / 3
// Return the true voltage (compensating for processor type)
float readVIN()
{
  float vin = analogRead(VIN_DIV_3);
#if defined(ARDUINO_ARDUINO_NANO33BLE)
  vin *= 3.3 / 1023.0; // nRF52840 (Arduino NANO 33 BLE) is 3.3V and defaults to 10-bit
#elif defined(ARDUINO_AM_AP3_SFE_ARTEMIS_MICROMOD)
  vin *= 2.0 / 1023.0; // Artemis (APOLLO3) is 2.0V and defaults to 10-bit
#elif defined(ARDUINO_ARCH_ESP32)
  vin *= 3.3 / 4095.0; // ESP32 is 3.3V and defaults to 12-bit
#elif defined(ARDUINO_ARCH_SAMD)
  vin *= 3.3 / 1023.0; // SAMD51 is 3.3V and defaults to 10-bit
#else
  vin *= 1.0; // Undefined PB!
#endif
  vin *= 3.0; // Correct for resistor divider
  return (vin);
}
