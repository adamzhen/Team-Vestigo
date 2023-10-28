#ifndef ESPNOWOperation_h
#define ESPNOWOperation_h

#include <Arduino.h>
#include <WiFiUdp.h>
#include <WiFi.h>
#include <esp_now.h>
#include "SharedVariables.h"

#define MAX_TAGS 2
#define MAX_ANCHORS 6

void setupESPNOW();
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len);
void sendToPeer(uint8_t *peerMAC, Data *message, size_t dataSize);

#endif