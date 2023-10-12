#ifndef UWBOperationMaster_h
#define UWBOperationMaster_h

#include <Arduino.h>
#include <dw3000.h>

#define PIN_RST 27
#define PIN_IRQ 34
#define PIN_SS 4

#define MASTER_ANCHOR_ID 0
#define MASTER_CHANNEL 5

#define SYNC_MSG_TS_IDX 10
#define SYNC_MSG_TS_LEN 8

#define TX_ANT_DLY 16385
#define RX_ANT_DLY 16385

const uint64_t unitsPerSecond = static_cast<uint64_t>(1.0 / DWT_TIME_UNITS);

void configUWB();
void gatherSlaveToF();
void sendSyncSignal();

#endif