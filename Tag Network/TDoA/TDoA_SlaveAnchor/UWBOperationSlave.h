#ifndef UWBOperationSlave_h
#define UWBOperationSlave_h

#include <Arduino.h>
#include <dw3000.h>
#include "SharedVariables.h"

// SPI Pins
#define PIN_RST 27
#define PIN_IRQ 34
#define PIN_SS 4

// Anchor Info
#define MASTER_ANCHOR_ID 0
#define MASTER_CHANNEL 5
#define TAG_CHANNEL 5

// TDoA Sync
#define SYNC_MSG_TS_IDX 8
#define SYNC_MSG_TS_LEN 5

// TDoA Blink
#define BLINK_MSG_ID_IDX 8
#define BLINK_MSG_TS_IDX 9
#define BLINK_MSG_TS_LEN 5

// TWR ToF
#define ALL_MSG_COMMON_LEN 10
#define RESP_MSG_POLL_RX_TS_IDX 10
#define RESP_MSG_RESP_TX_TS_IDX 14
#define RESP_MSG_TS_LEN 4
#define POLL_RX_TO_RESP_TX_DLY_UUS 450

// UWB Antenna Delays
#define TX_ANT_DLY 16385
#define RX_ANT_DLY 16385

// UWB Functions
void configUWB();
void sendSlaveToF();
void receiveTDoASignal();

#endif
