#include <algorithm>
#include <vector>
#include "UWBOperationSlave.h"
#include "ESPNOWOperation.h"
#include "SharedVariables.h"

int SLAVE_ID = 4;

uint64_t currentTime = 0;
uint64_t overflowCounterCurrent = 0;
uint64_t lastReceivedCurrentTime = 0;

int64_t lastPhaseOffset = 0;
double medianFrequencyOffset = 0.0;

bool isStartupComplete = false;

void setup() 
{
  Serial.begin(115200);

  setupESPNOW();

  configUWB();
}

void loop() 
{
  uint64_t receivedCurrentTime = dwt_readsystimestamphi32();
  if (receivedCurrentTime < lastReceivedCurrentTime) overflowCounterCurrent++;
  currentTime = (overflowCounterCurrent << 40) | receivedCurrentTime;

  if (!TWRData.collectToF) 
  {
    // If system startup has not been finished
    if (!isStartupComplete && !startupPhaseOffsets.empty() && !startupSlaveOffsetTimes.empty()) 
    {
      // Compute average frequency offset
      std::vector<double> frequencyOffsets;
      for (int i = 1; i < startupPhaseOffsets.size() - 1; ++i) {
        Serial.print("Time + 1: ");
        Serial.println(static_cast<int64_t>(startupSlaveOffsetTimes[i + 1]));
        Serial.print("Time - 1: ");
        Serial.println(static_cast<int64_t>(startupSlaveOffsetTimes[i - 1]));
        Serial.print("Offset + 1: ");
        Serial.println(startupPhaseOffsets[i + 1]);
        Serial.print("Offset - 1: ");
        Serial.println(startupPhaseOffsets[i - 1]);
        double frequencyOffset = static_cast<double>(startupPhaseOffsets[i+1] - startupPhaseOffsets[i-1]) / static_cast<double>(static_cast<int64_t>(startupSlaveOffsetTimes[i+1]) - static_cast<int64_t>(startupSlaveOffsetTimes[i-1]));
        if (frequencyOffset != 0)
        {
          frequencyOffsets.push_back(frequencyOffset);
        }
      }

      if (frequencyOffsets.size() > 1)
      {
        std::sort(frequencyOffsets.begin(), frequencyOffsets.end());
        medianFrequencyOffset = frequencyOffsets[frequencyOffsets.size() / 2];
      }
      else
      {
        Serial.println("Collection Error");
      }
      
      lastPhaseOffset = startupPhaseOffsets.back();

      Serial.println("");
      Serial.println("");
      Serial.println("");
      Serial.print("Phase Offset: ");
      Serial.println(lastPhaseOffset);
      Serial.print("Frequency Offset: ");
      Serial.println(medianFrequencyOffset);
      Serial.println("");
      Serial.println("");
      Serial.println("");

      isStartupComplete = true;
    }

    lastReceivedCurrentTime = receivedCurrentTime;

    receiveTDoASignal();

    // Serial.print("ToF Data: ");
    // Serial.println(TWRData.ToF);
  }
  else
  {
    sendSlaveToF();
  }
}
