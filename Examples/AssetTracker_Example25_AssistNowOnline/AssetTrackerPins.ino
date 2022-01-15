/*

  MicroMod Asset Tracker Pin & Port Support Functions
  ===================================================

  Written by: Paul Clark
  Date: December 28th 2021

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

  digitalWrite(SARA_PWR, LOW); // Make sure SARA_PWR is low before making the pin an output
  pinMode(SARA_PWR, OUTPUT);
  digitalWrite(SARA_PWR, LOW);

  pinMode(SARA_RI, INPUT);
  
  pinMode(SARA_INT, INPUT);

  if (SARA_DSR >= 0) pinMode(SARA_DSR, INPUT);

  if (SARA_ON >= 0) pinMode(SARA_ON, INPUT);

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
// Note: if this is called before assetTracker.begin, it will take 10 seconds to time out
void disableGNSSAntennaPower()
{
  // On v11 of the Asset Tracker, the antenna power is controlled by SARA GPIO2.
  // We need to pull GPIO2 (Pin 23) low to disable the power.
  assetTracker.setGpioMode(assetTracker.GPIO2, assetTracker.GPIO_OUTPUT, 0); // Disable
}
// Enable power for the GNSS active antenna
// Note: if this is called before assetTracker.begin, it will take 10 seconds to time out
void enableGNSSAntennaPower()
{
  // On v11 of the Asset Tracker, the antenna power is controlled by SARA GPIO2.
  // We need to pull GPIO2 (Pin 23) high to enable the power.
  assetTracker.setGpioMode(assetTracker.GPIO2, assetTracker.GPIO_OUTPUT, 1); // Enable
}

// Read VIN / 3
// Return the true voltage (compensating for processor type)
float readVIN()
{
  float vin = analogRead(VIN_DIV_3);
#if defined(ARDUINO_ARDUINO_NANO33BLE)
  // nRF52840 (Arduino NANO 33 BLE) is 3.3V and defaults to 10-bit
  // BUT the Schottky diode D2 on the 3.3V line reduces VDD to 3.05V
  vin *= 3.05 / 1023.0;
  vin *= 3.0; // Correct for resistor divider
#elif defined(ARDUINO_APOLLO3_SFE_ARTEMIS_MM_PB)
  vin *= 2.0 / 1023.0; // Artemis (APOLLO3) is 2.0V and defaults to 10-bit
  vin *= 2.5 / 1.5; // Artemis PB has a built-in 150k/100k divider
  vin *= 1.41; // Correction factor to compensate for the divider resistance
  vin *= 3.0; // Correct for resistor divider
#elif defined(ARDUINO_ARCH_ESP32)
  // ESP32 is 3.3V and defaults to 12-bit
  // Manual measurements:
  // VIN  ADC
  // 3.5V 1150
  // 4.0V 1350
  // 4.5V 1535
  // 5.0V 1735
  // 5.5V 1930
  // 6.0V 2130
  // so, VIN = (ADC / 392) + 0.565
  vin /= 392;
  vin += 0.565;
#elif defined(ARDUINO_ARCH_SAMD)
  // SAMD51 is 3.3V and defaults to 10-bit
  // BUT the Schottky diode D3 on the 3.3V line reduces VDD to 3.02V
  vin *= 3.02 / 1023.0;
  vin *= 3.0; // Correct for resistor divider
#elif defined(ARDUINO_ARCH_STM32)
  // STM32 is 3.3V and defaults to 10-bit
  vin *= 3.3 / 1023.0;
  vin *= 3.0; // Correct for resistor divider
#else
  vin *= 3.0; // Undefined PB!
#endif
  return (vin);
}
