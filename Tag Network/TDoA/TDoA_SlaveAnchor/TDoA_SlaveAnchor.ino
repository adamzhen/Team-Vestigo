#include <algorithm>
#include <vector>
#include "UWBOperationSlave.h"
#include "ESPNOWOperation.h"
#include "TimeFunctions.h"
#include "SharedVariables.h"

int SLAVE_ID = 2;

void setup() 
{
  Serial.begin(115200);

  setupESPNOW();

  configUWB();
}

void loop() 
{
  if (TWRData.collectToF) 
  {
    sendSlaveToF();
  }
  else
  {
    receiveTDoASignal();
  }
}
