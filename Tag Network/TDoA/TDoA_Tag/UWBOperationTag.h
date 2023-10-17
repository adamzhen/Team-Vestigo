#ifndef UWBOperationTag_h
#define UWBOperationTag_h

#include <Arduino.h>
#include <dw3000.h>

#define PIN_RST 27
#define PIN_IRQ 34
#define PIN_SS 4

#define MASTER_CHANNEL 5

#define BLINK_MSG_ID_IDX 8
#define BLINK_MSG_TS_IDX 9
#define BLINK_MSG_TS_LEN 5

#define TX_ANT_DLY 16385
#define RX_ANT_DLY 16385

void configUWB();
void sendBlink();

#endif