#include "UWBOperationTag.h"

uint8_t TAG_ID = 1;

void setup() 
{
  configUWB();
}

void loop()
{
  sendBlink();
  delay(500);
}