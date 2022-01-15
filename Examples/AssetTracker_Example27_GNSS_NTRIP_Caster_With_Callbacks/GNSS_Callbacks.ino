//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// Callback: pushGPGGA will be called when new GPGGA NMEA data arrives
// See u-blox_structs.h for the full definition of NMEA_GGA_data_t
//         _____  You can use any name you like for the callback. Use the same name when you call setNMEAGPGGAcallback
//        /               _____  This _must_ be NMEA_GGA_data_t
//        |              /           _____ You can use any name you like for the struct
//        |              |          /
//        |              |          |
void pushGPGGA(NMEA_GGA_data_t nmeaData)
{
  if (connectionOpen)
  {
    SERIAL_PORT.print(F("Pushing GGA to server: "));
    SERIAL_PORT.print((const char *)nmeaData.nmea); // .nmea is printable (NULL-terminated) and already has \r\n on the end

    //Push our current GGA sentence to caster
    assetTracker.socketWrite(socketNum, (const char *)nmeaData.nmea);
  }
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// Callback: printPVTdata will be called when new NAV PVT data arrives
// See u-blox_structs.h for the full definition of UBX_NAV_PVT_data_t
//         _____  You can use any name you like for the callback. Use the same name when you call setAutoPVTcallback
//        /                  _____  This _must_ be UBX_NAV_PVT_data_t
//        |                 /               _____ You can use any name you like for the struct
//        |                 |              /
//        |                 |              |
void printPVTdata(UBX_NAV_PVT_data_t ubxDataStruct)
{
  long latitude = ubxDataStruct.lat; // Print the latitude
  SERIAL_PORT.print(F("Lat: "));
  SERIAL_PORT.print(latitude / 10000000L);
  SERIAL_PORT.print(F("."));
  SERIAL_PORT.print(abs(latitude % 10000000L));

  long longitude = ubxDataStruct.lon; // Print the longitude
  SERIAL_PORT.print(F("  Long: "));
  SERIAL_PORT.print(longitude / 10000000L);
  SERIAL_PORT.print(F("."));
  SERIAL_PORT.print(abs(longitude % 10000000L));

  long altitude = ubxDataStruct.hMSL; // Print the height above mean sea level
  SERIAL_PORT.print(F("  Height: "));
  SERIAL_PORT.print(altitude);
  SERIAL_PORT.print(F(" (mm)"));

  uint8_t fixType = ubxDataStruct.fixType; // Print the fix type
  SERIAL_PORT.print(F("  Fix: "));
  SERIAL_PORT.print(fixType);
  if (fixType == 0)
    SERIAL_PORT.print(F(" (None)"));
  else if (fixType == 1)
    SERIAL_PORT.print(F(" (Dead Reckoning)"));
  else if (fixType == 2)
    SERIAL_PORT.print(F(" (2D)"));
  else if (fixType == 3)
    SERIAL_PORT.print(F(" (3D)"));
  else if (fixType == 3)
    SERIAL_PORT.print(F(" (GNSS + Dead Reckoning)"));
  else if (fixType == 5)
    SERIAL_PORT.print(F(" (Time Only)"));
  else
    SERIAL_PORT.print(F(" (UNKNOWN)"));

  uint8_t carrSoln = ubxDataStruct.flags.bits.carrSoln; // Print the carrier solution
  SERIAL_PORT.print(F("  Carrier Solution: "));
  SERIAL_PORT.print(carrSoln);
  if (carrSoln == 0)
    SERIAL_PORT.print(F(" (None)"));
  else if (carrSoln == 1)
    SERIAL_PORT.print(F(" (Floating)"));
  else if (carrSoln == 2)
    SERIAL_PORT.print(F(" (Fixed)"));
  else
    SERIAL_PORT.print(F(" (UNKNOWN)"));

  uint32_t hAcc = ubxDataStruct.hAcc; // Print the horizontal accuracy estimate
  SERIAL_PORT.print(F("  Horizontal Accuracy Estimate: "));
  SERIAL_PORT.print(hAcc);
  SERIAL_PORT.print(F(" (mm)"));

  SERIAL_PORT.println();    
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
