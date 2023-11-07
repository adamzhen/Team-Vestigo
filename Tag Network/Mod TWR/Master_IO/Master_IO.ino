#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <esp_sleep.h>

/******************************************
******** GEN VARIABLES AND STRUCTS ********
******************************************/

float distances[4][13] = {0};
bool received[4] = {false};
int num_tags = 4;

volatile bool ackReceived = false;
volatile bool resetInProgress = false;
unsigned long lastAckTime = 0;
const unsigned long timeoutDuration = 750;
const unsigned long timeoutCascadeDuration = 3000;

typedef struct __attribute__((packed)) rangingData {
  float data[13] = {0};
  int tag_id = 0; 
  bool run_ranging; 
} rangingData;

typedef struct __attribute__((packed)) ackData {
  bool ack;
} ackData;

rangingData onDeviceRangingData;
rangingData offDeviceRangingData;

uint8_t macs[][6] = {
  {0xD4, 0xD4, 0xDA, 0x46, 0x0C, 0xA8}, // TAG1
  {0xD4, 0xD4, 0xDA, 0x46, 0x6C, 0x6C}, // TAG2
  {0xD4, 0xD4, 0xDA, 0x46, 0x66, 0x54}, // TAG3
  {0x54, 0x43, 0xB2, 0x7D, 0xC4, 0x44}, // TAG4
  {0x08, 0x3A, 0x8D, 0x83, 0x44, 0x10}  // Master IO
};

/******************************************
************ NETWORK FUNCTIONS ************
******************************************/

void sendAck(const uint8_t *peerMAC) {
  ackData message;
  message.ack = true;

  uint8_t buf[sizeof(ackData) + 1];
  buf[0] = 1;
  memcpy(&buf[1], &message, sizeof(ackData));

  esp_now_send(peerMAC, buf, sizeof(buf));
}

bool waitForAck() {
  unsigned long startMillis = millis();
  while (!ackReceived) {
    delay(5);
    if (millis() - startMillis > 100) {
      return false;
    }
  }
  ackReceived = false;
  return true;
}

////////////// WIP ///////////////////////////
void sendJson() {
  StaticJsonDocument<1024> doc;
  for (int tag_id = 0; tag_id < 4; tag_id++) {
    for (int i = 0; i < 13; i++) {
      doc["tags"][tag_id]["anchors"][i] = distances[tag_id][i];
    }
  }
  
  serializeJson(doc, Serial);
  Serial.println();
  doc.clear();
}
////////////// WIP ///////////////////////////

void sendToPeer(uint8_t *peerMAC, rangingData *message, int retries = 3) {
  esp_err_t result;

  uint8_t buf[sizeof(rangingData) + 1];  // Buffer to hold type identifier and data
  buf[0] = 0;  // rangingData type identifier
  memcpy(&buf[1], message, sizeof(rangingData));  // Copy rangingData to buffer

  for (int i = 0; i < retries; i++) {
    result = esp_now_send(peerMAC, buf, sizeof(buf));  // Send the buffer
    if (result == ESP_OK) {
      break;
    } else {
      if (i < retries - 1) {  // If it's not the last retry
        delay(50);  // Delay before retry
      }
    }
  }
}

////////////// WIP ///////////////////////////
void sendResetFlagToTag(int tag_id) {
  onDeviceRangingData.run_ranging = false;
  sendToPeer(macs[(offDeviceRangingData.tag_id + 1) % num_tags], &onDeviceRangingData);
  onDeviceRangingData.run_ranging = true;
}
////////////// WIP ///////////////////////////

////////////// WIP ///////////////////////////
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  uint8_t dataType = incomingData[0];

  // If the data is rangingData
  if (dataType == 0) {
    memcpy(&offDeviceRangingData, incomingData + 1, sizeof(offDeviceRangingData));
    memcpy(distances[offDeviceRangingData.tag_id], offDeviceRangingData.data, sizeof(offDeviceRangingData.data));

    if (!received[offDeviceRangingData.tag_id]) {
      received[offDeviceRangingData.tag_id] = true;
    } 
    else if (!resetInProgress) {
      sendJson();
      for(int i = 0; i < 4; i++) {
        received[i] = false;
        for (int j = 0; j < 13; j++) {
          distances[i][j] = 0;
        }
      } 
    }
    else {
      for(int i = 0; i < 4; i++) {
        received[i] = false;
        for (int j = 0; j < 13; j++) {
          distances[i][j] = 0;
        }
      }
      resetInProgress = false; 
    }

    onDeviceRangingData.run_ranging = true;
    delay(125);
    sendToPeer(macs[(offDeviceRangingData.tag_id + 1) % num_tags], &onDeviceRangingData);

    // For debugging: print the received distances
    // Serial.println("Distances received:");
    // for (int i = 0; i < 13; i++) {
    //   Serial.print(distances[offDeviceRangingData.tag_id][i]);
    //   Serial.print(" ");
    // }
    // Serial.println();
  }
  
  else if (dataType == 1) {
    // Serial.println("Acknowledgement received");
    ackReceived = true;
    lastAckTime = millis();
    // add checking function here

  }
}
////////////// WIP ///////////////////////////

void setup_esp_now() {
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESPNow with a fallback logic
  WiFi.disconnect();
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESPNow Init Failed");
    ESP.restart();
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Transmitted packet

  // Register for Receive CB to get incoming data
  esp_now_register_recv_cb(OnDataRecv);

  // Register peers
  for(int i=0; i<sizeof(macs)/sizeof(macs[0]); i++) {
    esp_now_peer_info_t peerInfo;
    memcpy(peerInfo.peer_addr, macs[i], 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    peerInfo.ifidx = WIFI_IF_STA;

    // Add peer        
    if (esp_now_add_peer(&peerInfo) != ESP_OK){
      Serial.println("Failed to add peer");
      return;
    }
  }
}

/**************************************
************ PROGRAM SETUP ************
**************************************/

void setup() {
  Serial.begin(256000);

  setup_esp_now();
  esp_now_register_recv_cb(OnDataRecv);

  onDeviceRangingData.run_ranging = true;
  sendToPeer(macs[0], &onDeviceRangingData);

  delay(5000);
}

/*************************************
************ PROGRAM LOOP ************
*************************************/
void loop() {
  if (ackReceived && (millis() - lastAckTime) > timeoutDuration) {
    // Serial.println();
    // Serial.println();
    // Serial.println("Standard Failure");
    // Serial.println();
    resetInProgress = true;
    sendResetFlagToTag(offDeviceRangingData.tag_id);
    // Serial.print("Failing Tag: ");
    // Serial.println(offDeviceRangingData.tag_id);
    ackReceived = false;
    sendToPeer(macs[(offDeviceRangingData.tag_id + 2) % num_tags], &onDeviceRangingData);
    // Serial.println();
    // Serial.println();
    // Serial.println("Reset Tag");
    // Serial.println();
  }

  if (millis() - lastAckTime > timeoutCascadeDuration) {
    // Serial.println();
    // Serial.println();
    // Serial.println("Cascade Failure");
    // Serial.println();
    resetInProgress = true;
    onDeviceRangingData.run_ranging = true;
    for (int i = 0; i < 4; i++) {
      sendResetFlagToTag(i);
    }
    sendToPeer(macs[0], &onDeviceRangingData);
    // Serial.println();
    // Serial.println();
    // Serial.println("All tags reset");
    // Serial.println();
  }
}

