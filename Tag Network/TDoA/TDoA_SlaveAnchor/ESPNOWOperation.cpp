#include "ESPNOWOperation.h"
#include "SharedVariables.h"

// Structs
TDoAstruct TDoAData;
TWRstruct TWRData;

// Mac Addresses
uint8_t slaveMacs[][6] = {
  // all placeholders
  {0xD4, 0xD4, 0xDA, 0x46, 0x0C, 0xA8}, // Slave 1
  {0xD4, 0xD4, 0xDA, 0x46, 0x6C, 0x6C}, // Slave
  {0xD4, 0xD4, 0xDA, 0x46, 0x66, 0x54}, // TAG3
  {0x54, 0x43, 0xB2, 0x7D, 0xC4, 0x44}, // TAG4
};
uint8_t masterMac[6] = {0x08, 0x3A, 0x8D, 0x83, 0x44, 0x10}; //placeholder
uint8_t MIOMac[6] = {0x08, 0x3A, 0x8D, 0x83, 0x44, 0x10};  // Master IO

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