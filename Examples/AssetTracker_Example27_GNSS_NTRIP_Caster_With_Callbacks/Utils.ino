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
