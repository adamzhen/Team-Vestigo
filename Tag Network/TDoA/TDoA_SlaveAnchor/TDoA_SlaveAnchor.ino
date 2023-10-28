#include "UWBOperationSlave.h"
#include "ESPNOWOperation.h"
#include "SharedVariables.h"

int SLAVE_ID = 1;

uint64_t currentTime = 0;
uint64_t overflowCounterCurrent = 0;
uint64_t lastReceivedCurrentTime = 0;

KalmanState slaveKalmanState;

void setup() 
{
  Serial.begin(115200);

  setupESPNOW();

  configUWB();

  initializeKalman(slaveKalmanState);
}

void loop() 
{
  uint64_t receivedCurrentTime = dwt_readsystimestamphi32();
  if (receivedCurrentTime < lastReceivedCurrentTime) overflowCounterCurrent++;
  currentTime = (overflowCounterCurrent << 40) | receivedCurrentTime;

  if (!TWRData.collectToF) 
  {
    // If system startup has not been finished
    if (!startupPhaseOffsets.empty() && !startupSlaveOffsetTimes.empty()) 
    {
      // Compute average frequency offset
      uint64_t frequencyOffsetSum = 0;
      int count = 0;
      for (int i = 1; i < startupPhaseOffsets.size() - 1; ++i) {
        uint64_t frequencyOffset = (startupPhaseOffsets[i+1] - startupPhaseOffsets[i-1]) / (startupSlaveOffsetTimes[i+1] - startupSlaveOffsetTimes[i-1]);
        frequencyOffsetSum += frequencyOffset;
        count++;
      }
      uint64_t averageFrequencyOffset = frequencyOffsetSum / count;

      // Initialize Kalman filter with last phase offset and computed average frequency offset
      uint64_t lastPhaseOffset = startupPhaseOffsets.back();
      slaveKalmanState.frequencyOffset = averageFrequencyOffset;
      slaveKalmanState.phaseOffset = lastPhaseOffset;
    }

    predictKalman(slaveKalmanState, currentTime - lastReceivedCurrentTime);

    lastReceivedCurrentTime = currentTime;

    receiveTDoASignal();
  }
  else
  {
    sendSlaveToF();
  }
}
