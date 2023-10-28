#include "ESPNOWOperation.h"
#include "SharedVariables.h"

// Structs
TDoAstruct TDoAData;
TWRstruct TWRData;
Resetstruct deviceResetFlag;

// Mac Addresses
uint8_t slaveMacs[][6] = {
  {0x40, 0x22, 0xD8, 0x01, 0x77, 0x34}, // Slave 1
  {0x40, 0x22, 0xD8, 0x07, 0x1F, 0x2C}, // Slave 2
  {0xEC, 0x62, 0x60, 0xA7, 0xCC, 0xDC}, // Slave 3
  {0xEC, 0x62, 0x60, 0xA7, 0xF0, 0xF0}, // Slave 4
  {0x30, 0xC6, 0xF7, 0x3A, 0x69, 0x9C}, // Slave 5
  {0xD4, 0xD4, 0xDA, 0x46, 0x6B, 0x84}  // Slave 6
};
uint8_t masterMac[6] = {0xD4, 0xD4, 0xDA, 0x46, 0x66, 0x54};
uint8_t MIOMac[6] = {0x08, 0x3A, 0x8D, 0x83, 0x44, 0x10};

// ESP-NOW Functions
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) 
{
  uint8_t dataType = incomingData[0];
  
  switch(dataType) 
  {
    case TWR_TYPE:
      memcpy(&TWRData, incomingData + 1, sizeof(TWRData));
      break;
    
    case TDOA_TYPE:
      memcpy(&TDoAData, incomingData + 1, sizeof(TDoAData));
      break;
    
    case RESET_TYPE:
      ESP.restart();
      break;

    default:
      // Received an unknown data type
      // Handle this case, if needed
      break;
  }
}

void sendToPeer(uint8_t *peerMAC, Data *message, size_t dataSize) 
{
  uint8_t buf[dataSize + 1];
  buf[0] = message->type;
  memcpy(&buf[1], message, dataSize);
  esp_now_send(peerMAC, buf, dataSize + 1);
}


void setupESPNOW() {
  WiFi.mode(WIFI_STA);

  WiFi.disconnect();
  if (esp_now_init() != ESP_OK) {
    ESP.restart();
  }

  esp_now_register_recv_cb(OnDataRecv);

  // Register Slaves
  for(int i=0; i<sizeof(slaveMacs)/sizeof(slaveMacs[0]); i++) {
    esp_now_peer_info_t peerInfo;
    memcpy(peerInfo.peer_addr, slaveMacs[i], 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    peerInfo.ifidx = WIFI_IF_STA;
    if (esp_now_add_peer(&peerInfo) != ESP_OK){
      return;
    }
  }

  // Register Master
  esp_now_peer_info_t peerInfoMaster;
  memcpy(peerInfoMaster.peer_addr, masterMac, 6);
  peerInfoMaster.channel = 0;
  peerInfoMaster.encrypt = false;
  peerInfoMaster.ifidx = WIFI_IF_STA;
  if (esp_now_add_peer(&peerInfoMaster) != ESP_OK){
    return;
  }

  // Register MIO
  esp_now_peer_info_t peerInfoMIO;
  memcpy(peerInfoMIO.peer_addr, MIOMac, 6);
  peerInfoMIO.channel = 0;
  peerInfoMIO.encrypt = false;
  peerInfoMIO.ifidx = WIFI_IF_STA;    
  if (esp_now_add_peer(&peerInfoMIO) != ESP_OK){
    return;
  }
}