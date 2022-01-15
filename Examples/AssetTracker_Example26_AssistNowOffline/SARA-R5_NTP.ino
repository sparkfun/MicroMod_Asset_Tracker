
#include <time.h> // Note: this is the standard c time library, not Time.h

// Get the time from NTP
// This code is based heavily on:
// https://docs.arduino.cc/tutorials/mkr-nb-1500/mkr-nb-library-examples#mkr-nb-gprs-udp-ntp-client
// Many thanks to: Michael Margolis, Tom Igoe, Arturo Guadalupi, et al

// NTP Server
const char* ntpServer = "pool.ntp.org";               // The Network Time Protocol Server
//const char* ntpServer = "africa.pool.ntp.org";        // The Network Time Protocol Server
//const char* ntpServer = "asia.pool.ntp.org";          // The Network Time Protocol Server
//const char* ntpServer = "europe.pool.ntp.org";        // The Network Time Protocol Server
//const char* ntpServer = "north-america.pool.ntp.org"; // The Network Time Protocol Server
//const char* ntpServer = "oceania.pool.ntp.org";       // The Network Time Protocol Server
//const char* ntpServer = "south-america.pool.ntp.org"; // The Network Time Protocol Server

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

bool getNTPTime(uint8_t *y, uint8_t *mo, uint8_t *d, uint8_t *h, uint8_t *min, uint8_t *s)
{
  int serverPort = 123; //NTP requests are to port 123
  
  // Set up the packetBuffer for the NTP request
  const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
  byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
  
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);

  // Initialize values needed to form NTP request
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  //Allocate a UDP socket to talk to the NTP server

  int socketNum = assetTracker.socketOpen(SARA_R5_UDP);
  if (socketNum == -1)
  {
    SERIAL_PORT.println(F("getNTPTime: socketOpen failed!"));
    return (false);
  }

  SERIAL_PORT.print(F("getNTPTime: using socket "));
  SERIAL_PORT.println(socketNum);

  //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  // Send the request - NTP uses UDP
  
  if (assetTracker.socketWriteUDP(socketNum, ntpServer, serverPort, (const char *)&packetBuffer, NTP_PACKET_SIZE) != SARA_R5_SUCCESS) // Send the request
  {
    SERIAL_PORT.println(F("getNTPTime: socketWrite failed!"));
    assetTracker.socketClose(socketNum); // Be nice. Close the socket
    return (false);    
  }

  //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  // Wait up to 10 seconds for the response
  unsigned long requestTime = millis();

  while (millis() < (requestTime + 10000))
  {
    // We could use the Socket Read Callback to get the data, but, just for giggles,
    // and to prove it works, let's poll the arrival of the data manually...
    int avail = 0;
    if (assetTracker.socketReadAvailableUDP(socketNum, &avail) != SARA_R5_SUCCESS)
    {
      SERIAL_PORT.println(F("getNTPTime: socketReadAvailable failed!"));
      assetTracker.socketClose(socketNum); // Be nice. Close the socket
      return (false);
    }

    if (avail >= NTP_PACKET_SIZE) // Is enough data available?
    {
      if (avail > NTP_PACKET_SIZE) // Too much data?
      {
        SERIAL_PORT.print(F("getNTPTime: too much data received! Length: "));
        SERIAL_PORT.print(avail);
        SERIAL_PORT.print(F(". Reading "));
        SERIAL_PORT.print(NTP_PACKET_SIZE);
        SERIAL_PORT.println(F(" bytes..."));
      }
      
      if (assetTracker.socketReadUDP(socketNum, NTP_PACKET_SIZE, (char *)&packetBuffer) != SARA_R5_SUCCESS)
      {
        SERIAL_PORT.println(F("getNTPTime: socketRead failed!"));
        assetTracker.socketClose(socketNum); // Be nice. Close the socket
        return (false);
      }
      
      // Extract the time from the reply

      SERIAL_PORT.print(F("getNTPTime: received "));
      SERIAL_PORT.print(avail);
      SERIAL_PORT.println(F(" bytes. Extracting the time..."));
      
      //the timestamp starts at byte 40 of the received packet and is four bytes,
      // or two words, long. First, esxtract the two words:
      unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
      unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
  
      // combine the four bytes (two words) into a long integer
      // this is NTP time (seconds since Jan 1 1900):
      unsigned long secsSince1900 = highWord << 16 | lowWord;
  
      SERIAL_PORT.print(F("getNTPTime: seconds since Jan 1 1900 = "));
      SERIAL_PORT.println(secsSince1900);
  
      // now convert NTP time into everyday time:
      SERIAL_PORT.print(F("getNTPTime: Unix time = "));
  
      // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
      const unsigned long seventyYears = 2208988800UL;
  
      // subtract seventy years:
      unsigned long epoch = secsSince1900 - seventyYears;
  
      // print Unix time:
      SERIAL_PORT.println(epoch);

      // Instead of calculating the year, month, day, etc. manually, let's use time_t and tm to do it for us!
      time_t dateTime = epoch;
      tm *theTime = gmtime(&dateTime);

      // Load the time into y, mo, d, h, min, s
      *y = theTime->tm_year - 100; // tm_year is years since 1900. Convert to years since 2000.
      *mo = theTime->tm_mon + 1; //tm_mon starts at zero. Add 1 for January.
      *d = theTime->tm_mday;
      *h = theTime->tm_hour;
      *min = theTime->tm_min;
      *s = theTime->tm_sec;

      // Finish off by printing the time
      SERIAL_PORT.print(F("getNTPTime: YY/MM/DD HH:MM:SS : "));
      if (*y < 10) SERIAL_PORT.print(F("0"));
      SERIAL_PORT.print(*y);
      SERIAL_PORT.print(F("/"));
      if (*mo < 10) SERIAL_PORT.print(F("0"));
      SERIAL_PORT.print(*mo);
      SERIAL_PORT.print(F("/"));
      if (*d < 10) SERIAL_PORT.print(F("0"));
      SERIAL_PORT.print(*d);
      SERIAL_PORT.print(F(" "));
      if (*h < 10) SERIAL_PORT.print(F("0"));
      SERIAL_PORT.print(*h);
      SERIAL_PORT.print(F(":"));
      if (*min < 10) SERIAL_PORT.print(F("0"));
      SERIAL_PORT.print(*min);
      SERIAL_PORT.print(F(":"));
      if (*s < 10) SERIAL_PORT.print(F("0"));
      SERIAL_PORT.println(*s);

      assetTracker.socketClose(socketNum); // Be nice. Close the socket
      return (true); // We are done!
    }
    
    delay(100); // Wait before trying again
  }

  //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  SERIAL_PORT.println(F("getNTPTime: no NTP data received!"));
  assetTracker.socketClose(socketNum); // Be nice. Close the socket
  return (false);
}
