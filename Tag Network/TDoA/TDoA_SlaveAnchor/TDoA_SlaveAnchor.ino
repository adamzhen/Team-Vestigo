#include <algorithm>
#include <vector>
#include "UWBOperationSlave.h"
#include "ESPNOWOperation.h"
#include "TimeFunctions.h"
#include "SharedVariables.h"

int SLAVE_ID = 6;

int64_t lastPhaseOffset = 0;
double frequencyOffset = 0.0;

bool isStartupComplete = false;

void setup() 
{
  Serial.begin(115200);

  setupESPNOW();

  configUWB();
}

void loop() 
{
  if (!TWRData.collectToF) 
  {
    // If system startup has not been finished
    if (!isStartupComplete && !startupPhaseOffsets.empty() && !startupSlaveOffsetTimes.empty()) 
    {
      // Compute frequency offsets
      std::vector<double> frequencyOffsets;
      for (int i = 1; i < startupPhaseOffsets.size() - 1; ++i) 
      {
        double frequencyOffset = static_cast<double>(startupPhaseOffsets[i+1] - startupPhaseOffsets[i-1]) / static_cast<double>(static_cast<int64_t>(startupSlaveOffsetTimes[i+1]) - static_cast<int64_t>(startupSlaveOffsetTimes[i-1]));
        if (frequencyOffset != 0)
        {
          frequencyOffsets.push_back(frequencyOffset);
        }
      }

      // Sort frequencyOffsets
      std::sort(frequencyOffsets.begin(), frequencyOffsets.end());

      // Remove outliers using Tukey's fences
      double Q1 = frequencyOffsets[frequencyOffsets.size() / 4];
      double Q3 = frequencyOffsets[(3 * frequencyOffsets.size()) / 4];
      double IQR = Q3 - Q1;
      double lowerBound = Q1 - 1.5 * IQR;
      double upperBound = Q3 + 1.5 * IQR;

      std::vector<double> filteredFrequencyOffsets;
      for (double val : frequencyOffsets)
      {
        if (val >= lowerBound && val <= upperBound)
        {
          filteredFrequencyOffsets.push_back(val);
        }
      }

      // Calculate 25th and 75th percentiles
      double P25 = filteredFrequencyOffsets[filteredFrequencyOffsets.size() / 4];
      double P75 = filteredFrequencyOffsets[(3 * filteredFrequencyOffsets.size()) / 4];

      // Average the data between P25 and P75
      double sum = 0;
      int count = 0;
      for (double val : filteredFrequencyOffsets)
      {
        if (val >= P25 && val <= P75)
        {
          sum += val;
          count++;
        }
      }

      if (count > 0)
      {
        frequencyOffset = sum / count;
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
      Serial.println(frequencyOffset, 12);
      Serial.println("");
      Serial.println("");
      Serial.println("");

      isStartupComplete = true;
    }

    receiveTDoASignal();
  }
  else
  {
    sendSlaveToF();
  }
}
