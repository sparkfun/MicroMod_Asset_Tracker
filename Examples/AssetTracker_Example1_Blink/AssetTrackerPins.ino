/*

  MicroMod Asset Tracker Pin & Port Definitions
  =============================================

  Written by: Paul Clark
  Date: October 30th 2020

  The file defines the pins and ports for the MicroMod Asset Tracker.

  The pin and ports are defined using the MicroMod naming scheme.
  This allows the code to run on different MicroMod Processor Boards
  without needing to change the pin numbers.

  Licence: MIT
  Please see the LICENSE.md for full details

*/

/*

 Notes:

  Artemis PB enables RTS/CTS hardware handshaking by default. So we do not need to define those pins here.
  Artemis PB does not support TX2 (SARA_DTR) or RX2 (SARA_DCD) and so cannot use the SARA's 2-UART mode.

  SAMD51 PB does not support RTS (SARA_RTS) or CTS (SARA_CTS). So we do not need to define those pins here.
  SAMD51 PB does support TX2 (SARA_DTR) and RX2 (SARA_DCD) and so can probably use the SARA's 2-UART mode.
    But the pins are shared with I2C_SCL1 so care needs to be taken when closing the SARA_SCL and SARA_SDA
    split pads (so the PB can monitor the SARA's I2C bus).

  ESP32 PB does not support RTS (SARA_RTS) or CTS (SARA_CTS). So we do not need to define those pins here.
  ESP32 PB does not support TX2 (SARA_DTR) or RX2 (SARA_DCD) and so cannot use the SARA's 2-UART mode.
  ESP32 PB connects G1->AUD_LRCLK->SCL1 and G2->AUD_BCLK->SDA1 and so a decision is required:
    The user can control either the digital microphone or the MICROSD_PWR_EN and SARA_PWR pins.
    The user can open the PDM_CLK and PDM_DAT split pads to isolate the microphone.
    Or can open the G1/SD_PWR and G2/LTE_PWR split pads to isolate the power enables. (Both enables will then default to ON)

*/

// Ports

#define SD_and_IMU_SPI SPI  // SPI interface used by the Micro SD card and IMU
#define SARA_Serial Serial1 // Serial interface used by the SARA
#define AT_Wire Wire        // Wire (I2C) interface used by the Qwiic bus and the Battery Fuel Gauge

// Only some of the PBs support a second UART
#if defined(ARDUINO_AM_AP3_SFE_ARTEMIS_MICROMOD) //|| defined()
#define SARA_2UART_MODE_NOT_SUPPORTED
#endif

// This is hopefully a temporary fix for the SAMD51 pins
#if defined(ARDUINO_ARCH_SAMD)
#define D0 0
#define G0 2
#define G1 3
#define G2 4
#define G3 5
#define G4 6
#define G5 7
#define G6 8
#define G7 9
#define G9 11
#define I2CINT 12
#define CS 48
#define BATTVIN3 A4
#endif

// Pins

// Pre-defined MicroMod PB I/O pins:
// D0   : I/O : intented to be used as the Chip Select for an external SPI device
// D1   : I/O
// A0   : I/O : can be used for analog input
// A1   : I/O : can be used for analog input
// PWM0 : I/O : can be used for PWM output
// PWM1 : I/O : can be used for PWM output

const int EXT_SPI_CS = D0; // Output: Active low

const int MICROSD_CS     = G0; // Output: Active low
const int MICROSD_PWR_EN = G1; // Output: Pull low to enable power for the SD card. Pull high to disable.
const int SARA_PWR       = G2; // Output: Pull low then high to switch the SARA-R5 on. Pull low then high again to switch it off.
const int IMU_PWR_EN     = G3; // Output: Pull high to enable power for the IMU. Pull low to disable.
const int SARA_RI        = G4; // Input:  SARA-R5 Ring Indicator pin. Becomes CTS2 in 2-UART mode.
const int SARA_INT       = G5; // Input:  SARA-R5 EXT_INT interrupt pin.
const int ANT_PWR_EN     = G6; // Output: Pull high to enable power for the GNSS active antenna. Pull low to disable.
const int SARA_DSR       = G7; // Input:  SARA-R5 DSR pin. Becomes RTS2 (Output) in 2-UART mode. (Change the split pad to change the direction of the 74AVC4T774)
const int SARA_ON        = G9; // Input:  Pulled low when the SARA-R5 is on. Pulled high when the SARA-R5 is off.

#if defined(ARDUINO_ARCH_SAMD)
const int SARA_ON_ALT    = -1; // Not connected on the SAMD51
#else
const int SARA_ON_ALT    = G10; // Input: Alternate connection for the SARA_ON signal.
#endif

//const int SARA_RTS = ; // Output: SARA-R5 RTS pin.
//const int SARA_CTS = ; // Input:  SARA-R5 CTS pin.
//const int SARA_DTR = ; // Output: SARA-R5 DTR pin. Becomes TXD2 in 2-UART mode.
//const int SARA_DCD = ; // Input:  SARA-R5 DCD pin. Becomes RXD2 in 2-UART mode.

const int IMU_INT = I2CINT; // Input: IMU interrupt pin
const int IMU_CS = CS; // Output: IMU SPI Chip Select

const int VIN_DIV_3 = BATTVIN3; // Analog input: VIN supply voltage divided by 3.

// Initialize the I/O pins.
// Default to having everything turned off.
// See below for the enable/disable functions.
void initializeAssetTrackerPins()
{
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

  pinMode(SARA_DSR, INPUT);

  pinMode(SARA_ON, INPUT);

  pinMode(SARA_ON_ALT, INPUT);

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
