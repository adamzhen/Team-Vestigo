#ifndef UWBOperations_h
#define UWBOperations_h

#include <Arduino.h>
#include <dw3000.h>

extern uint32_t masterTime32bit;
extern uint64_t masterTime64bit; // Global time which everything is based off of

#define PIN_RST 27
#define PIN_IRQ 34
#define PIN_SS 4

#define MASTER_ANCHOR_ID 0
#define MASTER_CHANNEL 5
#define TAG_CHANNEL 5

#define SYNC_MSG_TS_IDX 10
#define SYNC_MSG_TS_LEN 8

#define TX_ANT_DLY 16385
#define RX_ANT_DLY 16385

uint64_t receiveSyncSignal();
void receiveTagSignal();

#endif
