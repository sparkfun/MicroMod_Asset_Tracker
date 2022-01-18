
#include "secrets.h" // Update secrets.h with your AssistNow token string

const unsigned long maxTimeBeforeHangup_ms = 20000UL; //If we fail to get a complete RTCM frame after 20s, then disconnect from caster

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//Connect to NTRIP Caster. Return true is connection is successful.
bool beginClient(int *theSocket, bool *connectionIsOpen)
{
  // Sanity check - return now if the socket is already open (should be impossible, but still...)
  if (*theSocket >= 0)
  {
    SERIAL_PORT.print(F("beginClient: Socket is already open"));
    if (*connectionIsOpen)
    {
      SERIAL_PORT.println(F(" and the connection is open!"));      
    }
    else
    {
      SERIAL_PORT.println(F("!"));
    }
    return (false);
  }

  SERIAL_PORT.println(F("beginClient: Opening TCP socket"));

  *theSocket = assetTracker.socketOpen(SARA_R5_TCP);
  if (*theSocket == -1)
  {
    SERIAL_PORT.println(F("beginClient: socketOpen failed!"));
    return (false);
  }

  SERIAL_PORT.print(F("beginClient: Using socket "));
  SERIAL_PORT.println(*theSocket);

  SERIAL_PORT.print(F("beginClient: Connecting to "));
  SERIAL_PORT.print(casterHost);
  SERIAL_PORT.print(F(" on port "));
  SERIAL_PORT.println(casterPort);

  if (assetTracker.socketConnect(*theSocket, casterHost, casterPort) != SARA_R5_SUCCESS)
  {
    SERIAL_PORT.println(F("beginClient: socketConnect failed!"));
  }
  else
  {
    SERIAL_PORT.print(F("beginClient: Connected to "));
    SERIAL_PORT.print(casterHost);
    SERIAL_PORT.print(F(" : "));
    SERIAL_PORT.println(casterPort);

    SERIAL_PORT.print(F("beginClient: Requesting NTRIP Data from mount point "));
    SERIAL_PORT.println(mountPoint);

    // Set up the server request (GET)
    const int SERVER_BUFFER_SIZE = 512;
    char *serverRequest = new char[SERVER_BUFFER_SIZE];
    memset(serverRequest, 0, SERVER_BUFFER_SIZE);
    snprintf(serverRequest,
             SERVER_BUFFER_SIZE,
             "GET /%s HTTP/1.0\r\nUser-Agent: NTRIP SparkFun u-blox Client v1.0\r\n",
             mountPoint);

    // Set up the credentials
    const int CREDENTIALS_BUFFER_SIZE = 512;
    char *credentials = new char[CREDENTIALS_BUFFER_SIZE];
    memset(credentials, 0, CREDENTIALS_BUFFER_SIZE);
    if (strlen(casterUser) == 0)
    {
      strncpy(credentials, "Accept: */*\r\nConnection: close\r\n", CREDENTIALS_BUFFER_SIZE);
    }
    else
    {
      //Pass base64 encoded user:pw
      int userCredentialsSize = sizeof(casterUser) + sizeof(casterUserPW) + 1; //The ':' takes up a spot
      char *userCredentials = new char[userCredentialsSize];
      memset(userCredentials, 0, userCredentialsSize);
      snprintf(userCredentials, userCredentialsSize, "%s:%s", casterUser, casterUserPW);

      SERIAL_PORT.print(F("beginClient: Sending credentials: "));
      SERIAL_PORT.println(userCredentials);

#if defined(ARDUINO_ARCH_ESP32)
      //Encode with ESP32 built-in library
      base64 b;
      String strEncodedCredentials = b.encode(userCredentials);
      int encodedCredentialsSize = strEncodedCredentials.length() + 1;
      char *encodedCredentials = new char[encodedCredentialsSize];
      memset(encodedCredentials, 0, encodedCredentialsSize);
      strEncodedCredentials.toCharArray(encodedCredentials, encodedCredentialsSize); //Convert String to char array
#elif defined(ARDUINO_ARCH_APOLLO3) || defined(ARDUINO_ARDUINO_NANO33BLE)
      int encodedCredentialsSize = userCredentialsSize * 8;
      char *encodedCredentials = new char[encodedCredentialsSize];
      memset(encodedCredentials, 0, encodedCredentialsSize);
      size_t olen;
      mbedtls_base64_encode((unsigned char *)encodedCredentials, encodedCredentialsSize, &olen, (const unsigned char *)userCredentials, strlen(userCredentials));
#else
      //Encode with nfriendly library
      int encodedLen = base64_enc_len(userCredentialsSize);
      char *encodedCredentials = new char[encodedLen]; //Create array large enough to house encoded data
      memset(encodedCredentials, 0, encodedLen);
      base64_encode(encodedCredentials, userCredentials, strlen(userCredentials)); //Note: Input array is consumed
#endif

      snprintf(credentials, CREDENTIALS_BUFFER_SIZE, "Authorization: Basic %s\r\n", encodedCredentials);
      delete[] userCredentials;
      delete[] encodedCredentials;
    }

    // Add the encoded credentials to the server request
    strncat(serverRequest, credentials, SERVER_BUFFER_SIZE - 1);
    strncat(serverRequest, "\r\n", SERVER_BUFFER_SIZE - 1);

    SERIAL_PORT.print(F("beginClient: serverRequest size: "));
    SERIAL_PORT.print(strlen(serverRequest));
    SERIAL_PORT.print(F(" of "));
    SERIAL_PORT.print(SERVER_BUFFER_SIZE);
    SERIAL_PORT.println(F(" bytes available"));

    // Send the server request
    SERIAL_PORT.println(F("beginClient: Sending server request: "));
    SERIAL_PORT.println(serverRequest);

    assetTracker.socketWrite(*theSocket, (const char *)serverRequest);

    //Wait up to 5 seconds for response. Poll the number of available bytes. Don't use the callback yet.
    unsigned long startTime = millis();
    int availableLength = 0;
    while (availableLength == 0)
    {
      assetTracker.socketReadAvailable(*theSocket, &availableLength);
      if (millis() > (startTime + 5000))
      {
        SERIAL_PORT.println(F("beginClient: Caster timed out!"));
        closeConnection(theSocket, connectionIsOpen);
        delete[] serverRequest;
        delete[] credentials;
        return (false);
      }
      delay(100);
    }

    SERIAL_PORT.print(F("beginClient: server replied with "));
    SERIAL_PORT.print(availableLength);
    SERIAL_PORT.println(F(" bytes"));

    //Check reply
    int connectionResult = 0;
    char *response = new char[512 * 4];
    memset(response, 0, 512 * 4);
    size_t responseSpot = 0;
    while ((availableLength > 0) && (connectionResult == 0)) // Read bytes from the caster and store them
    {
      if ((responseSpot + availableLength) >= ((512 * 4) - 1)) // Exit the loop if we get too much data
        break;

      assetTracker.socketRead(*theSocket, availableLength, &response[responseSpot]);
      responseSpot += availableLength;

      //SERIAL_PORT.print(F("beginClient: response is: "));
      //SERIAL_PORT.println(response);

      if (connectionResult == 0) // Only print success/fail once
      {
        if (strstr(response, "200") != NULL) //Look for '200 OK'
        {
          SERIAL_PORT.println(F("beginClient: 200 seen!"));
          connectionResult = 200;
        }
        if (strstr(response, "401") != NULL) //Look for '401 Unauthorized'
        {
          SERIAL_PORT.println(F("beginClient: Hey - your credentials look bad! Check your caster username and password."));
          connectionResult = 401;
        }
      }

      assetTracker.socketReadAvailable(*theSocket, &availableLength); // Update availableLength

      SERIAL_PORT.print(F("beginClient: socket now has "));
      SERIAL_PORT.print(availableLength);
      SERIAL_PORT.println(F(" bytes available"));
    }
    response[responseSpot] = '\0'; // NULL-terminate the response

    //SERIAL_PORT.print(F("beginClient: Caster responded with: ")); SERIAL_PORT.println(response); // Uncomment this line to see the full response

    // Free the memory allocated for serverRequest, credentials and response
    delete[] serverRequest;
    delete[] credentials;
    delete[] response;

    if (connectionResult != 200)
    {
      SERIAL_PORT.print(F("beginClient: Failed to connect to "));
      SERIAL_PORT.println(casterHost);
      closeConnection(theSocket, connectionIsOpen);
      return (false);
    }
    else
    {
      SERIAL_PORT.print(F("beginClient: Connected to: "));
      SERIAL_PORT.println(casterHost);
      lastReceivedRTCM_ms = millis(); //Reset timeout
    }
  
  } //End attempt to connect

  *connectionIsOpen = true;
  return (true);
} // /beginClient

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//Check the connection: print PVT data; check for and push GPGGA; check for and push RTCM data
//Return false if: the connection has dropped, or if we receive no data for maxTimeBeforeHangup_ms
bool checkConnection(int theSocket, bool connectionIsOpen)
{
  // See if new PVT data is available
  if (myGNSS.getPVT()) // getPVT will return true if fresh data is available
  {
    long latitude = myGNSS.getLatitude(); // Print the latitude
    SERIAL_PORT.print(F("checkConnection: Lat: "));
    SERIAL_PORT.print(latitude / 10000000L);
    SERIAL_PORT.print(F("."));
    SERIAL_PORT.print(abs(latitude % 10000000L));
  
    long longitude = myGNSS.getLongitude(); // Print the longitude
    SERIAL_PORT.print(F("  Long: "));
    SERIAL_PORT.print(longitude / 10000000L);
    SERIAL_PORT.print(F("."));
    SERIAL_PORT.print(abs(longitude % 10000000L));
  
    long altitude = myGNSS.getAltitudeMSL(); // Print the height above mean sea level
    SERIAL_PORT.print(F("  Height: "));
    SERIAL_PORT.print(altitude);
    SERIAL_PORT.print(F(" (mm)"));
  
    uint8_t fixType = myGNSS.getFixType(); // Print the fix type
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
  
    uint8_t carrSoln = myGNSS.getCarrierSolutionType(); // Print the carrier solution
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
  
    uint32_t hAcc = myGNSS.getHorizontalAccEst(); // Print the horizontal accuracy estimate
    SERIAL_PORT.print(F("  Horizontal Accuracy Estimate: "));
    SERIAL_PORT.print(hAcc);
    SERIAL_PORT.print(F(" (mm)"));
  
    SERIAL_PORT.println();          
  }

  if ((theSocket >= 0) && connectionIsOpen) // Check that the connection is still open
  {
    
    // Check if new NMEA GPGGA data is available
    NMEA_GGA_data_t *gpgga = new NMEA_GGA_data_t; // Allocate storage for the GPGGA data
    if (myGNSS.getLatestNMEAGPGGA(gpgga) == 2) // Is new /fresh GPGGA data available?
    {
      //Push our current GGA sentence to caster
      SERIAL_PORT.print(F("checkConnection: pushing NMEA GPGGA to Caster: "));
      SERIAL_PORT.print((const char *)gpgga->nmea); // .nmea is printable and has a \r\n on the end
      assetTracker.socketWrite(theSocket, (const char *)gpgga->nmea);
    }
    delete gpgga; // Delete (free) the allocated memory

    // Check if new RTCM data is available
    int length = 0;
    if (assetTracker.socketReadAvailable(theSocket, &length) == SARA_R5_SUCCESS)
    {
      if (length > 0)
      {
        // Push RTCM data to the GNSS
        char *rtcm = new char[length]; // Allocate storage for the RTCM data
        int bytesRead = 0;
        if (assetTracker.socketRead(theSocket, length, rtcm, &bytesRead) == SARA_R5_SUCCESS) // Get the data. bytesRead could be less than length
        {
          SERIAL_PORT.print(F("checkConnection: RTCM data received. Length is "));
          SERIAL_PORT.print(length);
          SERIAL_PORT.println(F(". Pushing it to the GNSS"));          
          
          myGNSS.pushRawData((uint8_t *)rtcm, (size_t)bytesRead);
        
          lastReceivedRTCM_ms = millis(); // Update lastReceivedRTCM_ms
        }
        delete rtcm; // Delete (free) the allocated memory
      }
    }
  }
  else
  {
    //SERIAL_PORT.println(F("checkConnection: Connection dropped!"));
    return (false); // Connection has dropped - return false
  }  
  
  //Timeout if we don't have new data for maxTimeBeforeHangup_ms
  if ((millis() - lastReceivedRTCM_ms) > maxTimeBeforeHangup_ms)
  {
    SERIAL_PORT.println(F("checkConnection: RTCM timeout!"));
    return (false); // Connection has timed out - return false
  }

  return (true);
} // /processConnection

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

void closeConnection(int *theSocket, bool *connectionIsOpen)
{
  // Check the socket is actually open, otherwise we'll get an error when we try to close it
  if (*theSocket >= 0)
  {
    assetTracker.socketClose(*theSocket);
    SERIAL_PORT.println(F("closeConnection: Connection closed!"));
  }
  else
  {
    SERIAL_PORT.println(F("closeConnection: Connection was already closed!"));    
  }

  *theSocket = -1; // Clear the socket number to indicate it is closed
  *connectionIsOpen = false; // Flag that the connection is closed
}  

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//Return true if a key has been pressed
bool keyPressed()
{
  if (SERIAL_PORT.available()) // Check for a new key press
  {
    delay(100); // Wait for any more keystrokes to arrive
    while (SERIAL_PORT.available()) // Empty the serial buffer
      SERIAL_PORT.read();
    return (true);   
  }

  return (false);
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
