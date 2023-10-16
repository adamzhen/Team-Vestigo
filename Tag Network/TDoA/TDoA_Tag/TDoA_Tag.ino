#include "UWBOperationTag.h"

void setup() 
{
  configUWB();
}

void loop()
{
  sendBlink();
  delay(500);
}