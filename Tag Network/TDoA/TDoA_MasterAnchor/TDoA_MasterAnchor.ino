#include "UWBOperationMaster.h"

// Global variables
uint8_t anchorId = MASTER_ANCHOR_ID;
uint64_t averageToF;

// determine how to switch channels 

void setup() 
{
  Serial.begin(115200);

  configUWB();

  uint32_t numSamples = 0;
  uint64_t totalToF = 0.0;

  uint64_t startTime = millis();
  while (millis() - startTime < 10000)
  {
    Serial.println("Loop");
    uint64_t ToF = gatherSlaveToF();
    if (ToF != 0) 
    {
      Serial.println("Data gathered");
      totalToF += ToF;
      numSamples++;
    }
  }
  Serial.println("exit");

  uint64_t averageToF = totalToF / numSamples;
  Serial.print("Average ToF: ");
  Serial.println(averageToF * DWT_TIME_UNITS, 12);
}

void loop() 
{
  sendSyncSignal();
  delay(200);
}




