#ifndef SHARED_VARIABLES_H
#define SHARED_VARIABLES_H

#include <Arduino.h>

// Global Variables
#define DWT_TIME_UNITS (1.0 / 499.2e6 / 128.0)
#define SPEED_OF_LIGHT (299702547)

// ESP-NOW Variables
extern uint8_t slaveMacs[][6];
extern uint8_t masterMac[6];
extern uint8_t MIOMac[6];
extern unsigned long lastReceptionTime;

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
  uint32_t difference = 0;
  TDoAstruct() {
    type = TDOA_TYPE;
  }
};

struct __attribute__((packed)) Resetstruct : public Data {
  bool reset = false;
  Resetstruct() {
    type = RESET_TYPE;
  }
};

extern TDoAstruct TDoAData;
extern Resetstruct deviceResetFlag;

#endif