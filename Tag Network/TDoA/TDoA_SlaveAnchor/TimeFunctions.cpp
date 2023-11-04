#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include "TimeFunctions.h"
#include "SharedVariables.h"

// Time Variables
uint64_t masterTime = 0, slaveTime = 0, tagTime = 0, syncTime = 0, TWRTime = 0; 
uint64_t lastReceivedMasterTime = 0, lastReceivedTagTime = 0, lastReceivedTWRTime = 0; 
uint64_t overflowCounterMaster = 0, overflowCounterTag = 0, overflowCounterTWR = 0;
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
  const uint64_t lower39bitMask = 0x7FFFFFFFFF; // Mask for the lower 39 bits
  if ((curTime & lower39bitMask) < (lastTime & lower39bitMask)) overflowCounter++;
  return (overflowCounter << 40) | (curTime & lower39bitMask);
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

// Corrects for overflow when adding two uint64_t values
uint64_t addWithOverflow(uint64_t a, uint64_t b) {
    uint64_t result = a + b;
    if (result < a) { // overflow occurred
        result = (result + 1); // wrap around
    }
    return result;
}

// Corrects for underflow when subtracting two uint64_t values
uint64_t subtractWithWrapAround(uint64_t a, uint64_t b) {
    if (a >= b) {
        return a - b; // no wrap-around
    } else {
        // wrap-around occurred, handle it
        uint64_t wrapAroundValue = ~static_cast<uint64_t>(0) - b + 1;
        return a + wrapAroundValue; // effectively subtracts b from a
    }
}

// Corrects for overflow in multiplication
uint64_t multiplyWithOverflow(uint64_t a, double b) {
    double result = static_cast<double>(a) * b;

    // Wrap around if result exceeds the maximum value for uint64_t
    result = fmod(result, static_cast<double>(~static_cast<uint64_t>(0)) + 1);

    return static_cast<uint64_t>(result);
}

// Corrects for division, ensuring no division by zero occurs
double safeDivide(uint64_t a, uint64_t b) {
    if (b == 0) {
        // Handle error: division by zero
        return 0.0; // or throw an exception, or return a sentinel value
    }
    return static_cast<double>(a) / static_cast<double>(b);
}