#ifndef SHARED_VARIABLES_H
#define SHARED_VARIABLES_H

#include <Arduino.h>
#include <deque>

// Device Variables
extern int SLAVE_ID;

// UWB Variables
extern std::deque<uint64_t> startupSlaveOffsetTimes;
extern std::deque<uint64_t> startupPhaseOffsets;

// ESP-NOW Variables
extern uint8_t slaveMacs[][6];
extern uint8_t masterMac[6];
extern uint8_t MIOMac[6];

// ESP-NOW Structs
enum DataType {
  TWR_TYPE = 0,
  TDOA_TYPE = 1,
  RESET_TYPE = 2
};

struct __attribute__((packed)) Data {
  DataType type;
};

struct __attribute__((packed)) TDoAstruct : public Data {
  int anchor_id = 0; 
  int tag_id = 0;
  double difference = 0.0;
  TDoAstruct() {
    type = TDOA_TYPE;
  }
};

struct __attribute__((packed)) TWRstruct : public Data {
  bool collectToF = false;
  uint64_t ToF = 0;
  TWRstruct() {
    type = TWR_TYPE;
  }
};

struct __attribute__((packed)) Resetstruct : public Data {
  bool reset = false;
  Resetstruct() {
    type = RESET_TYPE;
  }
};

extern TWRstruct TWRData;
extern TDoAstruct TDoAData;
extern Resetstruct deviceResetFlag;

// Kalman Filter Structs
struct KalmanState {
  uint64_t frequencyOffset;
  uint64_t phaseOffset;
  uint64_t P[2][2];  // Covariance matrix
};

extern KalmanState slaveKalmanState;

// Kalman Filter Variables
extern uint64_t processNoise;  // Process noise, to be tuned
extern uint64_t measurementNoise;  // Measurement noise, to be tuned
extern uint64_t lastUpdateTime;  // Last update time for the Kalman filter

#endif
