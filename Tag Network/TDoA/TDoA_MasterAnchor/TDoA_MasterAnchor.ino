#include <vector>
#include <algorithm>
#include "UWBOperationMaster.h"
#include "ESPNOWOperation.h"
#include "SharedVariables.h"

void setup() 
{
  Serial.begin(115200);

  setupESPNOW();

  configUWB();

  for (int i = 0; i < 6; i++) 
  {
    TWRData.collectToF = true;
    sendToPeer(slaveMacs[i], &TWRData, sizeof(TWRData));

    uint32_t numSamples = 0;
    std::vector<uint64_t> tofSamples;
    uint64_t averageToF = 1000000;
    bool stopCollect = false;

    while (!stopCollect)
    {
      uint64_t startTime = millis();
      while (millis() - startTime < 10000)
      {
        uint64_t ToF = gatherSlaveToF();
        if (ToF != 0) 
        {
          tofSamples.push_back(ToF);
          numSamples++;
        }
        delay(5);
      }

      
      if (numSamples > 10)
      {
        std::sort(tofSamples.begin(), tofSamples.end());
        averageToF = tofSamples[numSamples / 2];
      } 
      else 
      {
        Serial.println("error, not enough samples");
      }

      if ((double) (averageToF * DWT_TIME_UNITS) < 1e-6)
      {
        stopCollect = true;
      }
      
      Serial.print("Slave: ");
      Serial.println(i + 1);
      Serial.print("Average ToF: ");
      Serial.print(averageToF * DWT_TIME_UNITS, 12);
      Serial.print(", ");
      Serial.println(averageToF);
      Serial.print("Num Samples: ");
      Serial.println(numSamples);
    }

    TWRData.collectToF = false;
    TWRData.ToF = averageToF;
    sendToPeer(slaveMacs[i], &TWRData, sizeof(TWRData));
  }
}

void loop() 
{
  sendSyncSignal();
  delay(250);
}




