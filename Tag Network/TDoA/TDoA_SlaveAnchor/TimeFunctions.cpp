#include <algorithm>
#include <cmath>
#include "TimeFunctions.h"
#include "SharedVariables.h"

// Time Variables
uint64_t masterTime = 0, slaveTime = 0, tagTime = 0, syncTime = 0, TWRTime = 0; 
uint64_t lastReceivedMasterTime = 0, lastReceivedSlaveTime = 0, lastReceivedTagTime = 0, lastReceivedSyncTime = 0, lastReceivedTWRTime = 0; 
uint64_t overflowCounterMaster = 0, overflowCounterSlave = 0, overflowCounterTag = 0, overflowCounterSync = 0, overflowCounterTWR = 0;
int64_t timeOffset = 0;

// History Variables
std::deque<int64_t> timeDiffHistory;
std::deque<uint64_t> startupSlaveOffsetTimes;
std::deque<int64_t> startupPhaseOffsets;
const size_t historySize = 20;

// Constants
uint64_t unitsPerSecond = static_cast<uint64_t>(1.0 / DWT_TIME_UNITS);

// Functions
uint64_t getUWBTime(uint64_t time, uint8_t buffer[], uint8_t index)
{
  for (int i = 7; i >= 0; i--) 
  {
    time <<= 8;
    time |= buffer[index + i];
  }
  return time;
}

uint64_t adjustTo64bitTime(uint64_t curTime, uint64_t lastTime, uint64_t& overflowCounter)
{
  if (curTime < lastTime) overflowCounter++;
  return (overflowCounter << 40) | curTime;
}

void updateTimeOffsets(uint64_t& masterTime, uint64_t& syncTime, int64_t& timeOffset)
{
  uint64_t fracMasterTime = masterTime % unitsPerSecond;
  uint64_t fracSyncTime = syncTime % unitsPerSecond;

  int64_t signedFracMasterTime = static_cast<int64_t>(fracMasterTime);
  int64_t signedFracSyncTime = static_cast<int64_t>(fracSyncTime);
  
  timeOffset = signedFracSyncTime - signedFracMasterTime;

  if (std::abs(timeOffset) > static_cast<int64_t>(unitsPerSecond / 2)) 
  {
    timeOffset = static_cast<int64_t>(unitsPerSecond) - std::abs(timeOffset);
    timeOffset *= (fracSyncTime >= fracMasterTime) ? -1 : 1;
  }
}
