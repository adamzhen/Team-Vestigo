#include "UWBOperationSlave.h"
#include "ESPNOWOperation.h"
#include "SharedVariables.h"

int SLAVE_ID = 1;

void setup() 
{
  Serial.begin(115200);

  setupESPNOW();

  configUWB();
}

void loop() 
{
  while (TWRData.collectToF)
  {
    sendSlaveToF();
  }
  
  receiveTDoASignal();
}
