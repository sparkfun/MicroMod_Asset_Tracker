// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

// processSocketData is provided to the SARA-R5 library via a 
// callback setter -- setSocketReadCallback. (See setup())
void processSocketData(int socket, String theData)
{
  SERIAL_PORT.print(F("processSocketData: Data received on socket "));
  SERIAL_PORT.print(socket);
  SERIAL_PORT.print(F(". Length is "));
  SERIAL_PORT.print(theData.length());
  
  if (connectionOpen)
  {
    SERIAL_PORT.println(F(". Pushing it to the GNSS..."));
    myGNSS.pushRawData((uint8_t *)theData.c_str(), theData.length());

    lastReceivedRTCM_ms = millis(); // Update lastReceivedRTCM_ms
  }
  else
  {
    SERIAL_PORT.println();
  }
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

// processSocketClose is provided to the SARA-R5 library via a 
// callback setter -- setSocketCloseCallback. (See setup())
// 
// Note: the SARA-R5 only sends a +UUSOCL URC when the socket is closed by the remote
void processSocketClose(int socket)
{
  SERIAL_PORT.print(F("processSocketClose: Socket "));
  SERIAL_PORT.print(socket);
  SERIAL_PORT.println(F(" closed!"));

  if (socket == socketNum)
  {
    socketNum = -1;
    connectionOpen = false;
  }
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

// processPSDAction is provided to the SARA-R5 library via a 
// callback setter -- setPSDActionCallback. (See setup())
void processPSDAction(int result, IPAddress ip)
{
  SERIAL_PORT.print(F("processPSDAction: result: "));
  SERIAL_PORT.print(String(result));
  if (result == 0)
    SERIAL_PORT.print(F(" (success)"));
  SERIAL_PORT.print(F("  IP Address: \""));
  SERIAL_PORT.print(String(ip[0]));
  SERIAL_PORT.print(F("."));
  SERIAL_PORT.print(String(ip[1]));
  SERIAL_PORT.print(F("."));
  SERIAL_PORT.print(String(ip[2]));
  SERIAL_PORT.print(F("."));
  SERIAL_PORT.print(String(ip[3]));
  SERIAL_PORT.println(F("\""));
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
