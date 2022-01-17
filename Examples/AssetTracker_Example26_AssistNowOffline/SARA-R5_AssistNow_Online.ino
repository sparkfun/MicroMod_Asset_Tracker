
#include "secrets.h" // Update secrets.h with your AssistNow token string

// u-blox AssistNow https servers
const char assistNowOfflineServer[] = "offline-live1.services.u-blox.com";
//const char assistNowOfflineServer[] = "offline-live2.services.u-blox.com"; // Alternate server

const char getQuery[] = "GetOfflineData.ashx?";
const char tokenPrefix[] = "token=";
const char tokenSuffix[] = ";";
const char getGNSS[] = "gnss=gps,glo;"; // GNSS can be: gps,qzss,glo,bds,gal
const char getFormat[] = "format=mga;"; // Data format. Leave set to mga for M8 onwards. Can be aid.
const char getPeriod[] = "period=1;"; // Optional. The number of weeks into the future that the data will be valid. Can be 1-5. Default = 4.
const char getMgaResolution[] = "resolution=1;"; // Optional. Data resolution: 1 = every day; 2 = every other day; 3 = every 3rd day.
//Note: always use resolution=1. findMGAANOForDate does not yet support finding the 'closest' date. It needs an exact match.

volatile bool httpResultSeen = false; // Flag to indicate that the HTTP URC was received

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// processHTTPcommandResult is provided to the SARA-R5 library via a 
// callback setter -- setHTTPCommandCallback. (See the end of setup())
void processHTTPcommandResult(int profile, int command, int result)
{
  SERIAL_PORT.println();
  SERIAL_PORT.print(F("HTTP Command Result:  profile: "));
  SERIAL_PORT.print(profile);
  SERIAL_PORT.print(F("  command: "));
  SERIAL_PORT.print(command);
  SERIAL_PORT.print(F("  result: "));
  SERIAL_PORT.print(result);
  if (result == 0)
    SERIAL_PORT.print(F(" (fail)"));
  if (result == 1)
    SERIAL_PORT.print(F(" (success)"));
  SERIAL_PORT.println();

  // Get and print the most recent HTTP protocol error
  int error_class;
  int error_code;
  assetTracker.getHTTPprotocolError(0, &error_class, &error_code);
  SERIAL_PORT.print(F("Most recent HTTP protocol error:  class: "));
  SERIAL_PORT.print(error_class);
  SERIAL_PORT.print(F("  code: "));
  SERIAL_PORT.print(error_code);
  if (error_code == 0)
    SERIAL_PORT.print(F(" (no error)"));
  SERIAL_PORT.println();

  httpResultSeen = true; // Set the flag
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

bool getAssistNowOfflineData(String theFilename)
{
  // Use HTTP GET to receive the AssistNow_Offline data. Store it in the SARA-R5's internal file system.

  String theServer = assistNowOfflineServer; // Convert the AssistNow server to String

  const int REQUEST_BUFFER_SIZE  = 256;
  char theRequest[REQUEST_BUFFER_SIZE];

  // Assemble the request
  // Note the slash at the beginning
  snprintf(theRequest, REQUEST_BUFFER_SIZE, "/%s%s%s%s%s%s%s%s",
    getQuery,
    tokenPrefix,
    myAssistNowToken,
    tokenSuffix,
    getGNSS,
    getFormat,
    getPeriod,
    getMgaResolution
    );


  String theRequestStr = theRequest; // Convert to String

  SERIAL_PORT.print(F("getAssistNowOfflineData: HTTP GET is https://"));
  SERIAL_PORT.print(theServer);
  SERIAL_PORT.println(theRequestStr);

  SERIAL_PORT.print(F("getAssistNowOfflineData: the AssistNow data will be stored in: "));
  SERIAL_PORT.println(theFilename);

  // Reset HTTP profile 0
  assetTracker.resetHTTPprofile(0);
  
  // Set the server name
  assetTracker.setHTTPserverName(0, theServer);
  
  // Use HTTPS
  assetTracker.setHTTPsecure(0, false); // Setting this to true causes the GET to fail. Maybe due to the default CMNG profile?

  // Set a callback to process the HTTP command result
  assetTracker.setHTTPCommandCallback(&processHTTPcommandResult);

  httpResultSeen = false; // Clear the flag

  // HTTP GET
  assetTracker.sendHTTPGET(0, theRequestStr, theFilename);

  // Wait for 20 seconds while calling assetTracker.bufferedPoll() to see the HTTP result.
  SERIAL_PORT.print(F("getAssistNowOfflineData: Waiting up to 20 seconds for the HTTP Result"));
  int i = 0;
  while ((i < 20000) && (httpResultSeen == false))
  {
    assetTracker.bufferedPoll(); // Keep processing data from the SARA so we can catch the HTTP command result
    i++;
    delay(1);
    if (i % 1000 == 0)
      SERIAL_PORT.print(F("."));
  }
  SERIAL_PORT.println();
  
  if (httpResultSeen == false)
  {
    SERIAL_PORT.print(F("getAssistNowOfflineData: HTTP GET failed!"));
    return false;
  }

  int fileSize;
  if (assetTracker.getFileSize(theFilename, &fileSize) != SARA_R5_SUCCESS)
  {
    SERIAL_PORT.print(F("getAssistNowOfflineData: No file written?!"));
    return false;    
  }
  
  SERIAL_PORT.print(F("File size is: "));
  SERIAL_PORT.println(fileSize);

  return true;
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

void prettyPrintString(String theString) // Pretty-print a String in HEX and ASCII format
{
  int theLength = theString.length();
  
  SERIAL_PORT.println();
  SERIAL_PORT.print(F("String length is "));
  SERIAL_PORT.print(theLength);
  SERIAL_PORT.print(F(" (0x"));
  SERIAL_PORT.print(theLength, HEX);
  SERIAL_PORT.println(F(")"));
  SERIAL_PORT.println();

  for (int i = 0; i < theLength; i += 16)
  {
    if (i < 10000) SERIAL_PORT.print(F("0"));
    if (i < 1000) SERIAL_PORT.print(F("0"));
    if (i < 100) SERIAL_PORT.print(F("0"));
    if (i < 10) SERIAL_PORT.print(F("0"));
    SERIAL_PORT.print(i);

    SERIAL_PORT.print(F(" 0x"));

    if (i < 0x1000) SERIAL_PORT.print(F("0"));
    if (i < 0x100) SERIAL_PORT.print(F("0"));
    if (i < 0x10) SERIAL_PORT.print(F("0"));
    SERIAL_PORT.print(i, HEX);

    SERIAL_PORT.print(F(" "));

    int j;
    for (j = 0; ((i + j) < theLength) && (j < 16); j++)
    {
      if (theString[i + j] < 0x10) SERIAL_PORT.print(F("0"));
      SERIAL_PORT.print(theString[i + j], HEX);
      SERIAL_PORT.print(F(" "));
    }

    if (((i + j) == theLength) && (j < 16))
    {
      for (int k = 0; k < (16 - (theLength % 16)); k++)
      {
        SERIAL_PORT.print(F("   "));
      }
    }
      
    for (j = 0; ((i + j) < theLength) && (j < 16); j++)
    {
      if ((theString[i + j] >= 0x20) && (theString[i + j] <= 0x7E))
        SERIAL_PORT.write(theString[i + j]);
      else
        SERIAL_PORT.print(F("."));
    }

    SERIAL_PORT.println();
  }

  SERIAL_PORT.println();
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

void prettyPrintChars(char *theData, int theLength) // Pretty-print char data in HEX and ASCII format
{
  SERIAL_PORT.println();
  SERIAL_PORT.print(F("String length is "));
  SERIAL_PORT.print(theLength);
  SERIAL_PORT.print(F(" (0x"));
  SERIAL_PORT.print(theLength, HEX);
  SERIAL_PORT.println(F(")"));
  SERIAL_PORT.println();

  for (int i = 0; i < theLength; i += 16)
  {
    if (i < 10000) SERIAL_PORT.print(F("0"));
    if (i < 1000) SERIAL_PORT.print(F("0"));
    if (i < 100) SERIAL_PORT.print(F("0"));
    if (i < 10) SERIAL_PORT.print(F("0"));
    SERIAL_PORT.print(i);

    SERIAL_PORT.print(F(" 0x"));

    if (i < 0x1000) SERIAL_PORT.print(F("0"));
    if (i < 0x100) SERIAL_PORT.print(F("0"));
    if (i < 0x10) SERIAL_PORT.print(F("0"));
    SERIAL_PORT.print(i, HEX);

    SERIAL_PORT.print(F(" "));

    int j;
    for (j = 0; ((i + j) < theLength) && (j < 16); j++)
    {
      if (theData[i + j] < 0x10) SERIAL_PORT.print(F("0"));
      SERIAL_PORT.print(theData[i + j], HEX);
      SERIAL_PORT.print(F(" "));
    }

    if (((i + j) == theLength) && (j < 16))
    {
      for (int k = 0; k < (16 - (theLength % 16)); k++)
      {
        SERIAL_PORT.print(F("   "));
      }
    }
      
    for (j = 0; ((i + j) < theLength) && (j < 16); j++)
    {
      if ((theData[i + j] >= 0x20) && (theData[i + j] <= 0x7E))
        SERIAL_PORT.write(theData[i + j]);
      else
        SERIAL_PORT.print(F("."));
    }

    SERIAL_PORT.println();
  }

  SERIAL_PORT.println();
}
