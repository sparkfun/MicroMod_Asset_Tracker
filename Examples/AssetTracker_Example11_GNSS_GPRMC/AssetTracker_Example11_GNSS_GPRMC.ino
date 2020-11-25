/*

  MicroMod Asset Tracker Example
  ==============================

  GNSS GPRMC

  Written by: Paul Clark
  Date: October 30th 2020

  This example enables the SARA-R5 built-in GNSS receiver and reads the GPRMC message to
  get position, speed and time data.

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

// Global variables to store the GPRMC data
PositionData gps;
SpeedData spd;
ClockData clk;
boolean valid;

#define GPS_POLL_RATE 5000 // Read GPS every 5 seconds
unsigned long lastGpsPoll = 0;

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

  assetTracker.enableDebugging(SERIAL_PORT); // Uncomment this line to enable helpful debug messages

  assetTracker.invertPowerPin(true); // For the Asset Tracker, we need to invert the power pin so it pulls high instead of low

  // Initialize the SARA using SARA_Serial and 9600 Baud
  // begin calls init. init sets: SARA GPIO1 to NETWORK_STATUS, and SARA GPIO6 to TIME_PULSE_OUTPUT
  if (assetTracker.begin(SARA_Serial, 9600) )
  {
    SERIAL_PORT.println(F("Asset Tracker (SARA-R5) connected!"));
  }

  // Enable power for the GNSS active antenna
  enableGNSSAntennaPower();

  // From the u-blox SARA-R5 Positioning Implementation Application Note UBX-20012413 - R01
  // To enable the PPS output we need to:
  //  Enable the timing information request with +UTIMEIND=1
  //  Reset the time offset configuration with +UTIMECFG=0,0
  //  Configure GPIO6 for TIME_PULSE_OUTPUT: .init does this
  //  Request PPS output using GNSS+LTE (Best effort) with +UTIME=1,1
  // The bit that doesn't seem to be mentioned in the documentation is that +UTIME=1,1 also
  // enables the GPS module and so we don't need to call gpsPower or gpsEnableRmc

  // Enable the timing information request
  //assetTracker.setUtimeIndication(); // Use default (SARA_R5_UTIME_URC_CONFIGURATION_ENABLED)
  
  // Clear the time offset
  assetTracker.setUtimeConfiguration(); // Use default offset (offsetNanoseconds = 0, offsetSeconds = 0)
  
  // Set the UTIME mode to pulse-per-second output using a best effort from GNSS and LTE
  assetTracker.setUtimeMode(); // Use defaults (mode = SARA_R5_UTIME_MODE_PPS, sensor = SARA_R5_UTIME_SENSOR_GNSS_LTE)
}

void loop()
{
  if ((lastGpsPoll == 0) || (lastGpsPoll + GPS_POLL_RATE < millis()))
  {
    // Call (assetTracker.gpsGetRmc to get coordinate, speed, and timing data
    // from the GPS module. Valid can be used to check if the GPS is
    // reporting valid data
    if (assetTracker.gpsGetRmc(&gps, &spd, &clk, &valid) == SARA_R5_SUCCESS)
    {
      printGPS();
      lastGpsPoll = millis();
    }
    else
    {
      delay(1000); // If RMC read fails, wait a second and try again
    }
  }

  // Echo any incoming characters to the serial monitor
  if (SARA_Serial.available()) {
    SERIAL_PORT.write((char) SARA_Serial.read());
  }
}

void printGPS(void)
{
  SERIAL_PORT.println();
  SERIAL_PORT.println("UTC: " + String(gps.utc));
  SERIAL_PORT.print("Time: ");
  if (clk.time.hour < 10) SERIAL_PORT.print('0'); // Print leading 0
  SERIAL_PORT.print(String(clk.time.hour) + ":");
  if (clk.time.minute < 10) SERIAL_PORT.print('0'); // Print leading 0
  SERIAL_PORT.print(String(clk.time.minute) + ":");
  if (clk.time.second < 10) SERIAL_PORT.print('0'); // Print leading 0
  SERIAL_PORT.print(String(clk.time.second) + ".");
  if (clk.time.ms < 10) SERIAL_PORT.print('0'); // Print leading 0
  SERIAL_PORT.println(String(clk.time.ms));
  SERIAL_PORT.println("Latitude: " + String(gps.lat, 7));
  SERIAL_PORT.println("Longitude: " + String(gps.lon, 7));
  SERIAL_PORT.println("Speed: " + String(spd.speed, 4) + " @ " + String(spd.cog, 4));
  SERIAL_PORT.println("Date (MM/DD/YY): " + String(clk.date.month) + "/" + 
    String(clk.date.day) + "/" + String(clk.date.year));
  SERIAL_PORT.println("Magnetic variation: " + String(spd.magVar));
  SERIAL_PORT.println("Status: " + String(gps.status));
  SERIAL_PORT.println("Mode: " + String(gps.mode));
  SERIAL_PORT.println();
}
