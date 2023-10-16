#include "ESPNOWOperation.h"
#include "SharedVariables.h"

TDoAstruct TDoAData;
TWRstruct TWRData;

uint8_t slaveMacs[][6] = {
  // all placeholders
  {0xD4, 0xD4, 0xDA, 0x46, 0x0C, 0xA8}, // Slave 1
  {0xD4, 0xD4, 0xDA, 0x46, 0x6C, 0x6C}, // Slave
  {0xD4, 0xD4, 0xDA, 0x46, 0x66, 0x54}, // TAG3
  {0x54, 0x43, 0xB2, 0x7D, 0xC4, 0x44}, // TAG4
};
uint8_t masterMac[6] = {0x08, 0x3A, 0x8D, 0x83, 0x44, 0x10}; //placeholder
uint8_t MIOMac[6] = {0x08, 0x3A, 0x8D, 0x83, 0x44, 0x10};  // Master IO

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
      // Now, TDoAData is populated and you can use it.
      // For example:
      // Serial.print("Received TDoA Data, anchor_id: ");
      // Serial.println(TDoAData.anchor_id);
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
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESPNow with a fallback logic
  WiFi.disconnect();
  if (esp_now_init() != ESP_OK) {
    ESP.restart();
  }

  // Register for Receive CB to get incoming data
  esp_now_register_recv_cb(OnDataRecv);

  // Register Slaves
  for(int i=0; i<sizeof(slaveMacs)/sizeof(slaveMacs[0]); i++) {
    esp_now_peer_info_t peerInfo;
    memcpy(peerInfo.peer_addr, slaveMacs[i], 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    peerInfo.ifidx = WIFI_IF_STA;

    // Add peer        
    if (esp_now_add_peer(&peerInfo) != ESP_OK){
      return;
    }
  }

  // Register Master
  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, masterMac, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  peerInfo.ifidx = WIFI_IF_STA;

  // Add Master      
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    return;
  }

  // Register MIO
  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, MIOMac, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  peerInfo.ifidx = WIFI_IF_STA;

  // Add MIO      
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    return;
  }
}