/*

  MicroMod Asset Tracker Example
  ==============================

  Socket "Ping Pong" - TCP Data Transfers on multiple sockets

  Written by: Paul Clark
  Date: April 7th 2022

  This example demonstrates how to transfer data to and from an external Python "Echo Server" using multiple TCP sockets

  The PDP profile is read from NVM. Please make sure you have run examples 14 & 17 previously to set up the profile.

  Here's a quick how-to (assuming you are familiar with Python):
    Open up a Python editor on your computer
    Copy the Multi_TCP_Echo.py from the SARA-R5 Library GitHub repo Utils folder: https://github.com/sparkfun/SparkFun_u-blox_SARA-R5_Arduino_Library/tree/main/Utils
    Log in to your router
    Find your computer's local IP address (usually 192.168.0.something)
    Go into your router's Security / Port Forwarding settings:
      Create a new port forwarding rule
      The IP address is your local IP address
      Set the local port range to 1200-1206 (if you changed TCP_PORT_BASE, use that port number instead)
      Set the external port range to 1200-1206
      Set the protocol to TCP (or BOTH)
      Enable the rule
    This will open up a direct connection from the outside world, through your router, to ports 1200-1206 on your computer
      Remember to lock them down again when you're done!
    Edit the Python code and change 'HOST' to your local IP number:
      HOST = '192.168.0.nnn'
    Run the Python code
    Ask Google for your computer's public IP address:
      Google "what is my IP address"
    Run this code
    Enter your computer's public IP address when asked
    Sit back and watch the ping-pong!
    The code will stop after 20 Pings+Echos and 20 Pongs+Echos on each port
      On 5 ports, that's 400 TCP transfers in total!  

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
  SparkFun MicroMod Asset Tracker     : https://www.sparkfun.com/products/17272
  SparkFun MicroMod Artemis Processor : http://www.sparkfun.com/products/16401
  SparkFun MicroMod SAMD51 Processor  : http://www.sparkfun.com/products/16791
  SparkFun MicroMod ESP32 Processor   : http://www.sparkfun.com/products/16781

  Licence: MIT
  Please see LICENSE.md for full details

*/

#include "AssetTrackerPins.h" // Include the Asset Tracker pin and port definitions

#define SERIAL_PORT Serial // This is the console serial port - change this if required

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include <SparkFun_u-blox_SARA-R5_Arduino_Library.h> //Click here to get the library: http://librarymanager/All#SparkFun_u-blox_SARA-R5_Arduino_Library

// Create a SARA_R5 object to use throughout the sketch. Pass it the power pin number.
SARA_R5 assetTracker(SARA_PWR);

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// Globals

const int numConnections = 5; // How many sockets do you want to use? Max is 7.
unsigned int TCP_PORT_BASE = 1200; // Change this if required

// Keep track of how many ping-pong exchanges have taken place. "Ping" closes the socket when pingCount reaches pingPongLimit.
volatile int pingCount[numConnections];
volatile int pongCount[numConnections];
const int pingPongLimit = 20;

// Keep track of how long the socket has been open. "Pong" closes the socket when timeLimit (millis) is reached.
unsigned long startTime;
const unsigned long timeLimit = 120000; // 120 seconds

#include <IPAddress.h> // Needed for sockets
volatile int socketNum[numConnections]; // Record the socket numbers. -1 indicates the socket is invalid or closed.

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

// processSocketListen is provided to the SARA-R5 library via a 
// callback setter -- setSocketListenCallback. (See setup())
void processSocketListen(int listeningSocket, IPAddress localIP, unsigned int listeningPort, int socket, IPAddress remoteIP, unsigned int port)
{
  SERIAL_PORT.println();
  SERIAL_PORT.print(F("Socket connection made: listeningSocket "));
  SERIAL_PORT.print(listeningSocket);
  SERIAL_PORT.print(F(" localIP "));
  SERIAL_PORT.print(localIP[0]);
  SERIAL_PORT.print(F("."));
  SERIAL_PORT.print(localIP[1]);
  SERIAL_PORT.print(F("."));
  SERIAL_PORT.print(localIP[2]);
  SERIAL_PORT.print(F("."));
  SERIAL_PORT.print(localIP[3]);
  SERIAL_PORT.print(F(" listeningPort "));
  SERIAL_PORT.print(listeningPort);
  SERIAL_PORT.print(F(" socket "));
  SERIAL_PORT.print(socket);
  SERIAL_PORT.print(F(" remoteIP "));
  SERIAL_PORT.print(remoteIP[0]);
  SERIAL_PORT.print(F("."));
  SERIAL_PORT.print(remoteIP[1]);
  SERIAL_PORT.print(F("."));
  SERIAL_PORT.print(remoteIP[2]);
  SERIAL_PORT.print(F("."));
  SERIAL_PORT.print(remoteIP[3]);
  SERIAL_PORT.print(F(" port "));
  SERIAL_PORT.println(port);
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

// processSocketData is provided to the SARA-R5 library via a 
// callback setter -- setSocketReadCallback. (See setup())
void processSocketData(int socket, String theData)
{
  int connection = -1;
  for (int i = 0; i < numConnections; i++)
    if (socketNum[i] == socket)
      connection = i;

  if (connection == -1)
  {
    SERIAL_PORT.println();
    SERIAL_PORT.print(F("Data received on unexpected socket "));
    SERIAL_PORT.println(socket);
    return;
  }
  
  SERIAL_PORT.println();
  SERIAL_PORT.print(F("Data received on socket "));
  SERIAL_PORT.print(socket);
  SERIAL_PORT.print(F(" : "));
  SERIAL_PORT.println(theData);
  
  if (theData == String("Ping")) // Look for the "Ping"
  {
    if (pongCount[connection] < pingPongLimit)
    {
      const char pong[] = "Pong";
      assetTracker.socketWrite(socket, pong); // Send the "Pong"
      pongCount[connection]++;
    }
  }

  if (theData == String("Pong")) // Look for the "Pong"
  {
    if (pingCount[connection] < pingPongLimit)
    {
      const char ping[] = "Ping";
      assetTracker.socketWrite(socket, ping); // Send the "Ping"
      pingCount[connection]++;
    }
  }

  SERIAL_PORT.print(F("pingCount = "));
  SERIAL_PORT.print(pingCount[connection]);
  SERIAL_PORT.print(F(" : pongCount = "));
  SERIAL_PORT.println(pongCount[connection]);
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

// processSocketClose is provided to the SARA-R5 library via a 
// callback setter -- setSocketCloseCallback. (See setup())
// 
// Note: the SARA-R5 only sends a +UUSOCL URC when the socket os closed by the remote
void processSocketClose(int socket)
{
  SERIAL_PORT.println();
  SERIAL_PORT.print(F("Socket "));
  SERIAL_PORT.print(socket);
  SERIAL_PORT.println(F(" closed!"));
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

// processPSDAction is provided to the SARA-R5 library via a 
// callback setter -- setPSDActionCallback. (See setup())
void processPSDAction(int result, IPAddress ip)
{
  SERIAL_PORT.println();
  SERIAL_PORT.print(F("PSD Action:  result: "));
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

void setup()
{
  initializeAssetTrackerPins(); // Initialize the pins and ports (defined in AssetTrackerPins.ino)
  
  SERIAL_PORT.begin(115200); // Start the serial console

  // Wait for user to press key to begin
  SERIAL_PORT.println(F("SARA-R5 Example"));
  SERIAL_PORT.println(F("Wait for the SARA NI LED to light up - then press any key to begin"));
  
  while (!SERIAL_PORT.available()) // Wait for the user to press a key (send any serial character)
    ;
  while (SERIAL_PORT.available()) // Empty the serial RX buffer
    SERIAL_PORT.read();

  //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  // Start communication with the SARA-R5. Load and activate the Packet Switched Data profile.

  //assetTracker.enableDebugging(); // Uncomment this line to enable helpful debug messages on Serial

  // For the MicroMod Asset Tracker, we need to invert the power pin so it pulls high instead of low
  // Comment the next line if required
  assetTracker.invertPowerPin(true); 

  // Initialize the SARA
    if (assetTracker.begin(SARA_Serial, 115200) )
  {
    SERIAL_PORT.println(F("SARA-R5 connected!"));
  }
  else
  {
    SERIAL_PORT.println(F("Unable to communicate with the SARA."));
    SERIAL_PORT.println(F("Manually power-on (hold the SARA On button for 3 seconds) on and try again."));
    while (1) ; // Loop forever on fail
  }
  SERIAL_PORT.println();

  // First check to see if we're connected to an operator:
  String currentOperator = "";
  if (assetTracker.getOperator(&currentOperator) == SARA_R5_SUCCESS)
  {
    SERIAL_PORT.print(F("Connected to: "));
    SERIAL_PORT.println(currentOperator);
  }
  else
  {
    SERIAL_PORT.print(F("The SARA is not yet connected to an operator. Please use the previous examples to connect. Or wait and retry. Freezing..."));
    while (1)
      ; // Do nothing more
  }

  // Deactivate the PSD profile - in case one is already active
  if (assetTracker.performPDPaction(0, SARA_R5_PSD_ACTION_DEACTIVATE) != SARA_R5_SUCCESS)
  {
    SERIAL_PORT.println(F("Warning: performPDPaction (deactivate profile) failed. Probably because no profile was active."));
  }

  // Load the PSD profile from NVM - these were saved by a previous example
  if (assetTracker.performPDPaction(0, SARA_R5_PSD_ACTION_LOAD) != SARA_R5_SUCCESS)
  {
    SERIAL_PORT.println(F("performPDPaction (load from NVM) failed! Freezing..."));
    while (1)
      ; // Do nothing more
  }

  // Activate the profile
  if (assetTracker.performPDPaction(0, SARA_R5_PSD_ACTION_ACTIVATE) != SARA_R5_SUCCESS)
  {
    SERIAL_PORT.println(F("performPDPaction (activate profile) failed! Freezing..."));
    while (1)
      ; // Do nothing more
  }

  //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  //Print the dynamic IP Address (for profile 0)
  IPAddress myAddress;
  assetTracker.getNetworkAssignedIPAddress(0, &myAddress);
  SERIAL_PORT.print(F("\r\nMy IP Address is: "));
  SERIAL_PORT.print(myAddress[0]);
  SERIAL_PORT.print(F("."));
  SERIAL_PORT.print(myAddress[1]);
  SERIAL_PORT.print(F("."));
  SERIAL_PORT.print(myAddress[2]);
  SERIAL_PORT.print(F("."));
  SERIAL_PORT.println(myAddress[3]);

  //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  for (int i = 0; i < numConnections; i++) // Reset the ping and pong counts
  {
    pingCount[i] = 0;
    pongCount[i] = 0;
  }

  //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  // Set a callback to process the socket listen
  assetTracker.setSocketListenCallback(&processSocketListen);
  
  //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  // Set a callback to process the socket data
  assetTracker.setSocketReadCallback(&processSocketData);
  
  //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  // Set a callback to process the socket close
  // 
  // Note: the SARA-R5 only sends a +UUSOCL URC when the socket os closed by the remote
  assetTracker.setSocketCloseCallback(&processSocketClose);
  
  //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  SERIAL_PORT.println(F("\r\nPlease enter the IP number you want to ping (nnn.nnn.nnn.nnn followed by LF / Newline): "));

  char c = 0;
  bool selected = false;
  int val = 0;
  IPAddress theAddress = {0,0,0,0};
  int field = 0;
  while (!selected)
  {
    while (!SERIAL_PORT.available()) ; // Wait for a character to arrive
    c = SERIAL_PORT.read(); // Read it
    if (c == '\n') // Is it a LF?
    {
      theAddress[field] = val; // Store the current value
      if (field == 3)
        selected = true;
      else
      {
        SERIAL_PORT.println(F("Invalid IP Address. Please try again:"));
        val = 0;
        field = 0;
      }
    }
    else if (c == '.') // Is it a separator
    {
      theAddress[field] = val; // Store the current value
      if (field <= 2)
        field++; // Increment the field
      val = 0; // Reset the value
    }
    else if ((c >= '0') && (c <= '9'))
    {
      val *= 10; // Multiply by 10
      val += c - '0'; // Add the digit
    }
  }

  SERIAL_PORT.print(F("Remote address is "));
  SERIAL_PORT.print(theAddress[0]);
  SERIAL_PORT.print(F("."));
  SERIAL_PORT.print(theAddress[1]);
  SERIAL_PORT.print(F("."));
  SERIAL_PORT.print(theAddress[2]);
  SERIAL_PORT.print(F("."));
  SERIAL_PORT.println(theAddress[3]);
  
  // Open the sockets
  for (int i = 0; i < numConnections; i++)
  {

    socketNum[i] = assetTracker.socketOpen(SARA_R5_TCP);
    if (socketNum[i] == -1)
    {
      SERIAL_PORT.println(F("socketOpen failed! Freezing..."));
      while (1)
        assetTracker.bufferedPoll(); // Do nothing more except process any received data
    }

    SERIAL_PORT.print(F("Connection "));
    SERIAL_PORT.print(i);
    SERIAL_PORT.print(F(" is using socket "));
    SERIAL_PORT.println(socketNum[i]);

    // Connect to the remote IP Address
    if (assetTracker.socketConnect(socketNum[i], theAddress, TCP_PORT_BASE + i) != SARA_R5_SUCCESS)
    {
      SERIAL_PORT.println(F("socketConnect failed! Freezing..."));
      while (1)
        assetTracker.bufferedPoll(); // Do nothing more except process any received data
    }
    else
    {
      SERIAL_PORT.println(F("Socket connected!"));
    }

    // Send the first ping to start the ping-pong
    const char ping[] = "Ping";
    assetTracker.socketWrite(socketNum[i], ping); // Send the "Ping"
  }
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

void loop()
{
  assetTracker.bufferedPoll(); // Process the backlog (if any) and any fresh serial data

  for (int i = 0; i < numConnections; i++)
  {
    // Close the socket when we've reached pingPongLimit
    if ((pingCount[i] >= pingPongLimit) && (socketNum[i] >= 0))
    {
      printSocketParameters(socketNum[i]);
      
      //Comment the next line if you want the remote to close the sockets when they timeout
      //assetTracker.socketClose(socketNum[i]); // Close the socket
      
      socketNum[i] = -1;
    }
  }
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

// Print the socket parameters
// Note: the socket must be open. ERRORs will be returned if the socket is closed.
void printSocketParameters(int socket)
{
  SERIAL_PORT.print(F("\r\nSocket parameters for socket: "));
  SERIAL_PORT.println(socket);
  
  SERIAL_PORT.print(F("Socket type: "));
  SARA_R5_socket_protocol_t socketType;
  assetTracker.querySocketType(socket, &socketType);
  if (socketType == SARA_R5_TCP)
    SERIAL_PORT.println(F("TCP"));
  else if (socketType == SARA_R5_UDP)
    SERIAL_PORT.println(F("UDP"));
  else
    SERIAL_PORT.println(F("UNKNOWN! (Error!)"));
  
  SERIAL_PORT.print(F("Last error: "));
  int lastError;
  assetTracker.querySocketLastError(socket, &lastError);
  SERIAL_PORT.println(lastError);
  
  SERIAL_PORT.print(F("Total bytes sent: "));
  uint32_t bytesSent;
  assetTracker.querySocketTotalBytesSent(socket, &bytesSent);
  SERIAL_PORT.println(bytesSent);
  
  SERIAL_PORT.print(F("Total bytes received: "));
  uint32_t bytesReceived;
  assetTracker.querySocketTotalBytesReceived(socket, &bytesReceived);
  SERIAL_PORT.println(bytesReceived);
  
  SERIAL_PORT.print(F("Remote IP Address: "));
  IPAddress remoteAddress;
  int remotePort;
  assetTracker.querySocketRemoteIPAddress(socket, &remoteAddress, &remotePort);
  SERIAL_PORT.print(remoteAddress[0]);
  SERIAL_PORT.print(F("."));
  SERIAL_PORT.print(remoteAddress[1]);
  SERIAL_PORT.print(F("."));
  SERIAL_PORT.print(remoteAddress[2]);
  SERIAL_PORT.print(F("."));
  SERIAL_PORT.println(remoteAddress[3]);

  SERIAL_PORT.print(F("Socket status (TCP sockets only): "));
  SARA_R5_tcp_socket_status_t socketStatus;
  assetTracker.querySocketStatusTCP(socket, &socketStatus);
  if (socketStatus == SARA_R5_TCP_SOCKET_STATUS_INACTIVE)
    SERIAL_PORT.println(F("INACTIVE"));
  else if (socketStatus == SARA_R5_TCP_SOCKET_STATUS_LISTEN)
    SERIAL_PORT.println(F("LISTEN"));
  else if (socketStatus == SARA_R5_TCP_SOCKET_STATUS_SYN_SENT)
    SERIAL_PORT.println(F("SYN_SENT"));
  else if (socketStatus == SARA_R5_TCP_SOCKET_STATUS_SYN_RCVD)
    SERIAL_PORT.println(F("SYN_RCVD"));
  else if (socketStatus == SARA_R5_TCP_SOCKET_STATUS_ESTABLISHED)
    SERIAL_PORT.println(F("ESTABLISHED"));
  else if (socketStatus == SARA_R5_TCP_SOCKET_STATUS_FIN_WAIT_1)
    SERIAL_PORT.println(F("FIN_WAIT_1"));
  else if (socketStatus == SARA_R5_TCP_SOCKET_STATUS_FIN_WAIT_2)
    SERIAL_PORT.println(F("FIN_WAIT_2"));
  else if (socketStatus == SARA_R5_TCP_SOCKET_STATUS_CLOSE_WAIT)
    SERIAL_PORT.println(F("CLOSE_WAIT"));
  else if (socketStatus == SARA_R5_TCP_SOCKET_STATUS_CLOSING)
    SERIAL_PORT.println(F("CLOSING"));
  else if (socketStatus == SARA_R5_TCP_SOCKET_STATUS_LAST_ACK)
    SERIAL_PORT.println(F("LAST_ACK"));
  else if (socketStatus == SARA_R5_TCP_SOCKET_STATUS_TIME_WAIT)
    SERIAL_PORT.println(F("TIME_WAIT"));
  else
    SERIAL_PORT.println(F("UNKNOWN! (Error!)"));
      
  SERIAL_PORT.print(F("Unacknowledged outgoing bytes: "));
  uint32_t bytesUnack;
  assetTracker.querySocketOutUnackData(socket, &bytesUnack);
  SERIAL_PORT.println(bytesUnack);
}
