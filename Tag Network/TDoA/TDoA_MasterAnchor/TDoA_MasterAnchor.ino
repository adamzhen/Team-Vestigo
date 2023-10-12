#include "UWBOperationMaster.h"



// Global variables
uint8_t anchorId = MASTER_ANCHOR_ID;
uint64_t known_ToF;

// determine how to switch channels 

void setup() 
{
  Serial.begin(115200);

  configUWB();

  gatherSlaveToF();
}

void loop() 
{
  sendSyncSignal();
  delay(500);
}




