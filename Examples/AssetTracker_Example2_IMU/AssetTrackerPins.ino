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
  
  pinMode(MICROSD_CS, OUTPUT);
  digitalWrite(MICROSD_CS, HIGH);
  
  disableMicroSDPower();

  digitalWrite(SARA_PWR, HIGH); // Make sure SARA_PWR is high before making the pin an output
  pinMode(SARA_PWR, OUTPUT);
  digitalWrite(SARA_PWR, HIGH);

  disableIMUPower();

  pinMode(SARA_RI, INPUT);
  
  pinMode(SARA_INT, INPUT);

  disableGNSSAntennaPower();

  if (SARA_DSR >= 0) pinMode(SARA_DSR, INPUT);

  if (SARA_ON >= 0) pinMode(SARA_ON, INPUT);

  if (SARA_ON >= 0) pinMode(SARA_ON_ALT, INPUT);

  pinMode(IMU_INT, INPUT);

  pinMode(IMU_CS, INPUT);
  digitalWrite(IMU_CS, HIGH);

  pinMode(VIN_DIV_3, INPUT);

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
