/*

  MicroMod Asset Tracker Example
  ==============================

  IMU Wake-On-Motion Interrupt

  Written by: Paul Clark
  Date: October 30th 2020

  This example enables power for the IMU and then creates and monitors a Wake-On-Motion interrupt.
  Open a serial monitor and press any key to start the example.
  Tapping the board will cause an interrupt. The STAT LED will light up on each interrupt.

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

#include "ICM_20948.h"  // Click here to get the library: http://librarymanager/All#SparkFun_ICM_20948_IMU

ICM_20948_SPI myICM;  // Create an ICM_20948_SPI object

#define SERIAL_PORT Serial // This is the console serial port - change this if required

#include <SparkFun_u-blox_SARA-R5_Arduino_Library.h> //Click here to get the library: http://librarymanager/All#SparkFun_u-blox_SARA-R5_Arduino_Library

// Create a SARA_R5 object to use throughout the sketch. Pass it the power pin number.
SARA_R5 assetTracker(SARA_PWR);

// Some vars to control or respond to interrupts
volatile bool isrFired = false; // A flag to indicate that an interrupt was detected. This must be volatile.
unsigned long intSeenAt = 0; // Record the time of the interrupt (millis)

void setup()
{
  initializeAssetTrackerPins(); // Initialize the pins and ports (defined in AssetTrackerPins.ino)

  attachInterrupt(digitalPinToInterrupt(IMU_INT), icmISR, FALLING); // Set up a falling interrupt  

  SERIAL_PORT.begin(115200); // Start the serial console
  SERIAL_PORT.println(F("Asset Tracker Example"));

  delay(100);

  while (SERIAL_PORT.available()) // Make sure the serial RX buffer is empty
    SERIAL_PORT.read();

  SERIAL_PORT.println(F("IMU power is OFF"));
  SERIAL_PORT.println(F("Press any key to continue..."));

  while (!SERIAL_PORT.available()) // Wait for the user to press a key (send any serial character)
    ;

  enableIMUPower(); // Enable power for the IMU
  enableMicroSDPower(); // Enable power for the microSD card too - otherwise we can have SPI communication problems

  SD_and_IMU_SPI.begin(); // Begin SPI comms

  delay(1000); // Give the IMU time to start up

  SERIAL_PORT.println(F("IMU power is ON"));
  
  bool initialized = false;
  while( !initialized )
  {
    myICM.begin(IMU_CS, SD_and_IMU_SPI); // Start the IMU using SPI

    SERIAL_PORT.print( F("Initialization of the IMU returned: ") );
    SERIAL_PORT.println( myICM.statusString() );
    if( myICM.status != ICM_20948_Stat_Ok )
    {
      SERIAL_PORT.println( "Trying again..." );
      delay(500);
    }
    else
    {
      initialized = true;
    }
  }
  
  // In this advanced example we'll cover how to do a more fine-grained setup of your sensor
  SERIAL_PORT.println(F("Device connected!"));

  // Here we are doing a SW reset to make sure the device starts in a known state
  myICM.swReset( );
  if( myICM.status != ICM_20948_Stat_Ok){
    SERIAL_PORT.print(F("Software Reset returned: "));
    SERIAL_PORT.println(myICM.statusString());
  }
  delay(250);
  
  // Wake the sensor up (just in case it was already asleep)
  myICM.sleep( false ); // Sleep Mode turns off all the sensors. That's _really_ not what we want...
  myICM.lowPower( false ); // Low-Power Accelerometer mode turns off the Gyro and Mag and puts the Accel into Duty-Cycled mode

  // The next few configuration functions accept a bit-mask of sensors for which the settings should be applied.

  // Set Gyro and Accelerometer to the chosen sample mode
  // options: ICM_20948_Sample_Mode_Continuous
  //          ICM_20948_Sample_Mode_Cycled
  // This sets or clears the appropriate bits in the LP_CONFIG register
  // In Cycled mode the ODR is determined by the _SMPLRT_DIV register.
  myICM.setSampleMode( (ICM_20948_Internal_Acc | ICM_20948_Internal_Gyr), ICM_20948_Sample_Mode_Cycled ); 
  SERIAL_PORT.print(F("setSampleMode returned: "));
  SERIAL_PORT.println(myICM.statusString());

  // Set the Gyro sample rate (but leave the Accel rate unchanged)
  ICM_20948_smplrt_t mySmplrt;
  mySmplrt.g = 54;
  myICM.setSampleRate( ICM_20948_Internal_Gyr, mySmplrt );
  SERIAL_PORT.print(F("setSampleRate returned: "));
  SERIAL_PORT.println(myICM.statusString());
    
  // Set full scale ranges for both acc and gyr
  ICM_20948_fss_t myFSS;  // This uses a "Full Scale Settings" structure that can contain values for all configurable sensors
  
  myFSS.a = gpm2;         // (ICM_20948_ACCEL_CONFIG_FS_SEL_e)
                          // gpm2
                          // gpm4
                          // gpm8
                          // gpm16
                          
  myFSS.g = dps250;       // (ICM_20948_GYRO_CONFIG_1_FS_SEL_e)
                          // dps250
                          // dps500
                          // dps1000
                          // dps2000
                          
  myICM.setFullScale( (ICM_20948_Internal_Acc | ICM_20948_Internal_Gyr), myFSS );  
  if( myICM.status != ICM_20948_Stat_Ok)
  {
    SERIAL_PORT.print(F("setFullScale returned: "));
    SERIAL_PORT.println(myICM.statusString());
  }


  // Set up Digital Low-Pass Filter configuration
  ICM_20948_dlpcfg_t myDLPcfg;            // Similar to FSS, this uses a configuration structure for the desired sensors
  myDLPcfg.a = acc_d50bw4_n68bw8;         // (ICM_20948_ACCEL_CONFIG_DLPCFG_e)
                                          // acc_d246bw_n265bw      - means 3db bandwidth is 246 hz and nyquist bandwidth is 265 hz
                                          // acc_d111bw4_n136bw
                                          // acc_d50bw4_n68bw8
                                          // acc_d23bw9_n34bw4
                                          // acc_d11bw5_n17bw
                                          // acc_d5bw7_n8bw3        - means 3 db bandwidth is 5.7 hz and nyquist bandwidth is 8.3 hz
                                          // acc_d473bw_n499bw

  myDLPcfg.g = gyr_d51bw2_n73bw3;         // (ICM_20948_GYRO_CONFIG_1_DLPCFG_e)
                                          // gyr_d196bw6_n229bw8
                                          // gyr_d151bw8_n187bw6
                                          // gyr_d119bw5_n154bw3
                                          // gyr_d51bw2_n73bw3
                                          // gyr_d23bw9_n35bw9
                                          // gyr_d11bw6_n17bw8
                                          // gyr_d5bw7_n8bw9
                                          // gyr_d361bw4_n376bw5
                                          
  myICM.setDLPFcfg( (ICM_20948_Internal_Acc | ICM_20948_Internal_Gyr), myDLPcfg );
  if( myICM.status != ICM_20948_Stat_Ok){
    SERIAL_PORT.print(F("setDLPcfg returned: "));
    SERIAL_PORT.println(myICM.statusString());
  }

  // Choose whether or not to use DLPF
  // Here we're also showing another way to access the status values, and that it is OK to supply individual sensor masks to these functions
  ICM_20948_Status_e accDLPEnableStat = myICM.enableDLPF( ICM_20948_Internal_Acc, true );
  ICM_20948_Status_e gyrDLPEnableStat = myICM.enableDLPF( ICM_20948_Internal_Gyr, true );
  SERIAL_PORT.print(F("Enable DLPF for Accelerometer returned: ")); SERIAL_PORT.println(myICM.statusString(accDLPEnableStat));
  SERIAL_PORT.print(F("Enable DLPF for Gyroscope returned: ")); SERIAL_PORT.println(myICM.statusString(gyrDLPEnableStat));

  // Set the Accel Wake On Motion Threshold
  // The library does not have a built-in function for this so we need to do this manually
  // Threshold LSB is 4mg. Range is 0mg to 1020mg.
  // The ACCEL_WOM_THR register is in Register Bank 2 so we need to select that first.
  ICM_20948_Status_e accWOMThreshStat = myICM.setBank(2);
  SERIAL_PORT.print(F("Set ACCEL WOM Threshold: setBank(2) returned: ")); SERIAL_PORT.println(myICM.statusString(accWOMThreshStat));

  // Set the threshold to 120mg. This value seems very sensitive. 25 (100mg) triggers continuously.
  // Also, the 'threshold' is orientation sensitive. Tipping or tilting of the sensor by ~60 degrees produces continuous interrupts.
  uint8_t womThresh = 30;
  
  accWOMThreshStat = myICM.write( AGB2_REG_ACCEL_WOM_THR, (uint8_t*)&womThresh, sizeof(uint8_t) );
  SERIAL_PORT.print(F("Set ACCEL WOM Threshold returned: ")); SERIAL_PORT.println(myICM.statusString(accWOMThreshStat));
  

  // Now we're going to set up interrupts. There are a lot of options:
  
/*
    ICM_20948_Status_e  cfgIntActiveLow         ( bool active_low );
    ICM_20948_Status_e  cfgIntOpenDrain         ( bool open_drain );
    ICM_20948_Status_e  cfgIntLatch             ( bool latching );                          // If not latching then the interrupt is a 50 us pulse

    ICM_20948_Status_e  cfgIntAnyReadToClear    ( bool enabled );                           // If enabled, *ANY* read will clear the INT_STATUS register. So if you have multiple interrupt sources enabled be sure to read INT_STATUS first

    ICM_20948_Status_e  cfgFsyncActiveLow       ( bool active_low );
    ICM_20948_Status_e  cfgFsyncIntMode         ( bool interrupt_mode );                    // Can ue FSYNC as an interrupt input that sets the I2C Master Status register's PASS_THROUGH bit

    ICM_20948_Status_e  intEnableI2C            ( bool enable );
    ICM_20948_Status_e  intEnableDMP            ( bool enable );
    ICM_20948_Status_e  intEnablePLL            ( bool enable );
    ICM_20948_Status_e  intEnableWOM            ( bool enable );                            // Wake On Motion
    ICM_20948_Status_e  intEnableWOF            ( bool enable );                            // Wake On FSYNC
    ICM_20948_Status_e  intEnableRawDataReady   ( bool enable );
    ICM_20948_Status_e  intEnableOverflowFIFO   ( uint8_t bm_enable );
    ICM_20948_Status_e  intEnableWatermarkFIFO  ( uint8_t bm_enable );
 */
 
  myICM.cfgIntActiveLow(true);                      // Active low to be compatible with the breakout board's pullup resistor
  myICM.cfgIntOpenDrain(false);                     // Push-pull, though open-drain would also work thanks to the pull-up resistors on the breakout
  myICM.cfgIntAnyReadToClear(true);                 // Any read will clear the interrupt
  myICM.cfgIntLatch(false);                         // Interrupt will be a 50us pulse (latching the interrupt seems problematic?)
  SERIAL_PORT.print(F("cfgIntLatch returned: "));
  SERIAL_PORT.println(myICM.statusString());
  
  myICM.intEnableWOM(true);                         // enable the Wake On Motion interrupt
  SERIAL_PORT.print(F("intEnableWOM returned: "));
  SERIAL_PORT.println(myICM.statusString());

  SERIAL_PORT.println();
  SERIAL_PORT.println(F("Configuration complete!"));

  myICM.lowPower( true );                           // Put the sensor into low power mode - so we can wake from it!
}

void loop()
{
  if( isrFired )                        // If our isr flag is set then clear the interrupts on the ICM
  {
    isrFired = false;                   // Clear our interrupt flag
    myICM.getAGMT();                    // Get the A, G, M, and T readings
    //myICM.clearInterrupts();            // Any read should clear the interrupt. So we should not need to call this.
    intSeenAt = millis();               // Record when we saw the interrupt
    digitalWrite(LED_BUILTIN, HIGH);    // Turn the LED on to indicate the interrupt
    printScaledAGMT( myICM.agmt);       // Print the AGMT readngs
  }

  if( millis() > intSeenAt + 1000 ) // Keep the LED on for a full second
  {
    digitalWrite(LED_BUILTIN, LOW);
  }
}

void icmISR( void ){
  isrFired = true;                      // Just set a flag to indicate that an interrupt has occurred
}

void printFormattedFloat(float val, uint8_t leading, uint8_t decimals){
  float aval = abs(val);
  if(val < 0){
    SERIAL_PORT.print("-");
  }else{
    SERIAL_PORT.print(" ");
  }
  for( uint8_t indi = 0; indi < leading; indi++ ){
    uint32_t tenpow = 0;
    if( indi < (leading-1) ){
      tenpow = 1;
    }
    for(uint8_t c = 0; c < (leading-1-indi); c++){
      tenpow *= 10;
    }
    if( aval < tenpow){
      SERIAL_PORT.print("0");
    }else{
      break;
    }
  }
  if(val < 0){
    SERIAL_PORT.print(-val, decimals);
  }else{
    SERIAL_PORT.print(val, decimals);
  }
}

void printScaledAGMT( ICM_20948_AGMT_t agmt){
  SERIAL_PORT.print("Scaled. Acc (mg) [ ");
  printFormattedFloat( myICM.accX(), 5, 2 );
  SERIAL_PORT.print(", ");
  printFormattedFloat( myICM.accY(), 5, 2 );
  SERIAL_PORT.print(", ");
  printFormattedFloat( myICM.accZ(), 5, 2 );
  SERIAL_PORT.print(" ], Gyr (DPS) [ ");
  printFormattedFloat( myICM.gyrX(), 5, 2 );
  SERIAL_PORT.print(", ");
  printFormattedFloat( myICM.gyrY(), 5, 2 );
  SERIAL_PORT.print(", ");
  printFormattedFloat( myICM.gyrZ(), 5, 2 );
  SERIAL_PORT.print(" ], Mag (uT) [ ");
  printFormattedFloat( myICM.magX(), 5, 2 );
  SERIAL_PORT.print(", ");
  printFormattedFloat( myICM.magY(), 5, 2 );
  SERIAL_PORT.print(", ");
  printFormattedFloat( myICM.magZ(), 5, 2 );
  SERIAL_PORT.print(" ], Tmp (C) [ ");
  printFormattedFloat( myICM.temp(), 5, 2 );
  SERIAL_PORT.print(" ]");
  SERIAL_PORT.println();
}
