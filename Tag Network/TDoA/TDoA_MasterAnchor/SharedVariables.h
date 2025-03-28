#ifndef SHARED_VARIABLES_H
#define SHARED_VARIABLES_H

#include <Arduino.h>

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
  double frequencyOffset = 0.0;
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

// UWB Variables
extern uint64_t averageToF;
extern double frequencyOffset;

#endif
