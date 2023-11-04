#pragma once
#include <cstdint>
#include <deque>
#include <dw3000.h>

// Function prototypes
uint64_t getUWBTime(uint64_t time, uint8_t buffer[], uint8_t index);
uint64_t adjustTo64bitTime(uint64_t curTime, uint64_t lastTime, uint64_t& overflowCounter);
void updateTimeOffsets(uint64_t& masterTime, uint64_t& syncTime, int64_t& timeOffset);
uint64_t addWithOverflow(uint64_t a, uint64_t b);
uint64_t subtractWithWrapAround(uint64_t a, uint64_t b);
uint64_t multiplyWithOverflow(uint64_t a, double b);
double safeDivide(uint64_t a, uint64_t b);