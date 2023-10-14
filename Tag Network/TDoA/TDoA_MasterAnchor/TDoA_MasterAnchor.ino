#include "UWBOperationMaster.h"
#include "ESPNOWOperation.h"
#include "SharedVariables.h"

// Global variables
uint8_t anchorId = MASTER_ANCHOR_ID;
uint64_t averageToF = 0;

// determine how to switch channels 

void setup() 
{
  Serial.begin(115200);

  setupESPNOW();

  configUWB();

  TWRData.collectToF = true;
  sendToPeer(anchorMacs[3], &TWRData, sizeof(TWRData));

  uint32_t numSamples = 0;
  uint64_t totalToF = 0.0;

  uint64_t startTime = millis();
  while (millis() - startTime < 5000)
  {
    uint64_t ToF = gatherSlaveToF();
    if (ToF != 0) 
    {
      totalToF += ToF;
      numSamples++;
    }
    delay(5);
  }

  averageToF = totalToF / numSamples;
  Serial.print("Average ToF: ");
  Serial.println(averageToF * DWT_TIME_UNITS, 12);

  TWRData.collectToF = false;
  sendToPeer(anchorMacs[3], &TWRData, sizeof(TWRData));
}

void loop() 
{
  sendSyncSignal();
  delay(200);
}




