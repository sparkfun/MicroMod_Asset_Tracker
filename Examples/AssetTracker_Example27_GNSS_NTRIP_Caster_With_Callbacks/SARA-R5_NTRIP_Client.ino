
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
    char serverRequest[SERVER_BUFFER_SIZE];
    snprintf(serverRequest,
             SERVER_BUFFER_SIZE,
             "GET /%s HTTP/1.0\r\nUser-Agent: NTRIP SparkFun u-blox Client v1.0\r\n",
             mountPoint);

    // Set up the credentials
    char credentials[512];
    if (strlen(casterUser) == 0)
    {
      strncpy(credentials, "Accept: */*\r\nConnection: close\r\n", sizeof(credentials));
    }
    else
    {
      //Pass base64 encoded user:pw
      char userCredentials[sizeof(casterUser) + sizeof(casterUserPW) + 1]; //The ':' takes up a spot
      snprintf(userCredentials, sizeof(userCredentials), "%s:%s", casterUser, casterUserPW);

      SERIAL_PORT.print(F("beginClient: Sending credentials: "));
      SERIAL_PORT.println(userCredentials);

#if defined(ARDUINO_ARCH_ESP32)
      //Encode with ESP32 built-in library
      base64 b;
      String strEncodedCredentials = b.encode(userCredentials);
      char encodedCredentials[strEncodedCredentials.length() + 1];
      strEncodedCredentials.toCharArray(encodedCredentials, sizeof(encodedCredentials)); //Convert String to char array
#elif defined(ARDUINO_ARCH_APOLLO3) || defined(ARDUINO_ARDUINO_NANO33BLE)
      char encodedCredentials[sizeof(userCredentials) * 8];
      size_t olen;
      mbedtls_base64_encode((unsigned char *)encodedCredentials, sizeof(userCredentials) * 8, &olen, (const unsigned char *)userCredentials, strlen(userCredentials));
#else
      //Encode with nfriendly library
      int encodedLen = base64_enc_len(strlen(userCredentials));
      char encodedCredentials[encodedLen];                                         //Create array large enough to house encoded data
      base64_encode(encodedCredentials, userCredentials, strlen(userCredentials)); //Note: Input array is consumed
#endif

      snprintf(credentials, sizeof(credentials), "Authorization: Basic %s\r\n", encodedCredentials);
    }

    // Add the encoded credentials to the server request
    strncat(serverRequest, credentials, SERVER_BUFFER_SIZE);
    strncat(serverRequest, "\r\n", SERVER_BUFFER_SIZE);

    SERIAL_PORT.print(F("beginClient: serverRequest size: "));
    SERIAL_PORT.print(strlen(serverRequest));
    SERIAL_PORT.print(F(" of "));
    SERIAL_PORT.print(sizeof(serverRequest));
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
        return (false);
      }
      delay(100);
    }

    SERIAL_PORT.print(F("beginClient: server replied with "));
    SERIAL_PORT.print(availableLength);
    SERIAL_PORT.println(F(" bytes"));

    //Check reply
    int connectionResult = 0;
    char response[512];
    memset(response, 0, 512);
    size_t responseSpot = 0;
    while ((availableLength > 0) && (connectionResult == 0)) // Read bytes from the caster and store them
    {
      if ((responseSpot + availableLength) >= (sizeof(response) - 1)) // Exit the loop if we get too much data
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

//Check the connection
//Return false if: the connection has dropped, or if we receive no data for maxTimeBeforeHangup_ms
bool checkConnection(int theSocket, bool connectionIsOpen)
{
  if ((theSocket >= 0) && connectionIsOpen) // Check that the connection is still open
  {
    ; // Nothing to do here. The RTCM is pushed to the GNSS by the callabck.
  }
  else
  {
    SERIAL_PORT.println(F("checkConnection: Connection dropped!"));
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
