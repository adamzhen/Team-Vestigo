#include "UWBOperationSlave.h"
//#include "ESP-NOWOperations.h"

// Global variables
uint8_t anchorId;  // To be set to the Slave Anchor's ID

void setup() 
{
  Serial.begin(115200);

  configUWB();

  while (true) 
  {
    sendSlaveToF();
  }
}

void loop() 
{
  receiveSyncSignal();
  // receiveTagSignal(); 
}
