#ifndef UWBOperationMaster_h
#define UWBOperationMaster_h

#include <Arduino.h>
#include <dw3000.h>

#define PIN_RST 27
#define PIN_IRQ 34
#define PIN_SS 4

#define MASTER_ANCHOR_ID 0
#define MASTER_CHANNEL 5

// TDoA Sync
#define SYNC_MSG_TS_IDX 8
#define SYNC_MSG_TS_LEN 5

// TWR ToF
#define ALL_MSG_COMMON_LEN 10
#define RESP_MSG_POLL_RX_TS_IDX 10
#define RESP_MSG_RESP_TX_TS_IDX 14
#define RESP_MSG_TS_LEN 4
#define POLL_TX_TO_RESP_RX_DLY_UUS 240
#define RESP_RX_TIMEOUT_UUS 400

#define TX_ANT_DLY 16385
#define RX_ANT_DLY 16385

void configUWB();
uint64_t gatherSlaveToF();
void sendSyncSignal();

#endif