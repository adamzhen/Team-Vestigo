#include "UWBOperationSlave.h"
#include "ESPNOWOperation.h"
#include "SharedVariables.h"

// Global variables
uint8_t anchorId;  // To be set to the Slave Anchor's ID


void setup() 
{
  Serial.begin(115200);

  setupESPNOW();

  configUWB();

  while (TWRData.collectToF) 
  {
    sendSlaveToF();
  }
}

void loop() 
{
  receiveSyncSignal();
  // receiveTagSignal(); 
}
