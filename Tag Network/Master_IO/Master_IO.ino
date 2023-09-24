#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <ArduinoJson.h>
#include <SPI.h>

/******************************************
******** GEN VARIABLES AND STRUCTS ********
******************************************/

float distances[4][13] = {0};
bool online_tags[4] = {false};
bool received[4] = {false};
bool all_tags_online = false;
int attempts[4] = {0};
bool startup_success = false;

bool startNetworkSetupFlag = false;

volatile bool ackReceived = false;
unsigned int resetAttempts = 0;
unsigned int resetsConfirmed = 0;

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
    JsonArray data = doc.createNestedArray(String(tag_id + 1));
    for (int i = 0; i < 13; i++) {
      data.add(distances[tag_id][i]);
    }
    data.add(0);
    data.add(0);
    data.add(0);
    data.add(0);
  }
  
  byte buffer[1024];
  size_t nBytes = serializeJson(doc, buffer, sizeof(buffer));

  Serial.write('<'); // Start delimiter
  Serial.write(buffer, nBytes); // Write the raw JSON data
  Serial.write('>'); // End delimiter
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
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  uint8_t dataType = incomingData[0];

  // If the data is rangingData
  if (dataType == 0) {
    memcpy(&offDeviceRangingData, incomingData + 1, sizeof(offDeviceRangingData));
    memcpy(distances[offDeviceRangingData.tag_id], offDeviceRangingData.data, sizeof(offDeviceRangingData.data));

    if (!received[offDeviceRangingData.tag_id]) {
      received[offDeviceRangingData.tag_id] = true;
    } else {   
      sendJson();
      for(int i = 0; i < 4; i++) {
        received[i] = false;
        for (int j = 0; j < 13; j++) {
          distances[i][j] = 0;
        }
      } 
    }

    onDeviceRangingData.run_ranging = true;
    sendToPeer(macs[(offDeviceRangingData.tag_id + 1) % 4], &onDeviceRangingData);

    // For debugging: print the received distances
    Serial.println("Distances received:");
    for (int i = 0; i < 13; i++) {
      Serial.print(distances[offDeviceRangingData.tag_id][i]);
      Serial.print(" ");
    }
    Serial.println();
  }
  
  else if (dataType == 1) {
    Serial.println("Acknowledgement received");
    ackReceived = true;

    // add checking function here
  }
}
////////////// WIP ///////////////////////////

////////////// WIP ///////////////////////////
void checkTagsOnline() {
  static uint8_t tag_poll_index = 0;
  Serial.println(ackReceived = true ? "Received Flag Reset" : "Received Flag Not  Reset");

  if (ackReceived) {
    Serial.println("Acknowledgement received from tag " + String(tag_poll_index));
    online_tags[tag_poll_index] = true;

    ackReceived = false;

    // If the tag responded, reset the number of attempts for this tag
    attempts[tag_poll_index] = 0;

    // Move to the next tag
    tag_poll_index++;

    if (tag_poll_index >= 4) { 
      tag_poll_index = 0;
    }
  } else {
    // If the tag didn't respond, increase the number of attempts for this tag
    Serial.println("No acknowledgement from tag " + String(tag_poll_index));
    attempts[tag_poll_index]++;
  
    // If we've tried 5 times without success, report this to the server
    if (attempts[tag_poll_index] >= 5) {
      DynamicJsonDocument doc(64);
      JsonArray array = doc.createNestedArray("tag_failures");
      for(int i = 0; i < 4; i++){
        array.add(attempts[i]);
      }
      String output;
      serializeJson(doc, output);
      // if (client.connect(server, error_port)) {
      //   client.println(output);
      //   client.stop();
      // }

      // Reset the number of attempts for all tags
      for (int i = 0; i < 4; i++) {
        attempts[i] = 0;
        online_tags[i] = false;  // Reset all tags to offline
      }
  
      // Move to the next tag
      tag_poll_index++;

      if (tag_poll_index >= 4) { 
        tag_poll_index = 0;
      }

      return;
    }
  }

  // Check if all tags are online
  all_tags_online = true;
  for (int i = 0; i < 4; i++) {
    if (!online_tags[i]) {
      all_tags_online = false;
      break;
    }
  }

  // Debug: Print the online status of all tags and the value of all_tags_online
  for (int i = 0; i < 4; i++) {
    Serial.print("Tag ");
    Serial.print(i);
    Serial.print(" online status: ");
    Serial.println(online_tags[i] ? "Online" : "Offline");
  }
  Serial.print("All tags online: ");
  Serial.println(all_tags_online ? "Yes" : "No");

  // If not all tags are online, continue polling
  if (!all_tags_online) {
    Serial.println("Sending polling request to tag " + String(tag_poll_index));
    // sendToPeer(macs[tag_poll_index]); // change later
    ackReceived = false;
    waitForAck();
  } else {
    // If all tags are online, set startup_success to true
    startup_success = true;
  }
}
////////////// WIP ///////////////////////////

////////////// WIP ///////////////////////////
void startNetworkSetup() {
  // // Connect to the server
  // if (client.connect(server, error_port)) {
  //   Serial.println("Connected to the server");
  // } else {
  //   Serial.println("Failed to connect to the server");
  //   return;  // If connection to the server failed, return from this function
  // }

  //TEMP VARIABLE SET
  startNetworkSetupFlag = true;

  // Wait for the server to send "start"
  // while(!startNetworkSetupFlag) {
  //   if (client.available() > 0) {
  //     String serverMsg = client.readStringUntil('\n');

  //     if (serverMsg == "start") {
  //       startNetworkSetupFlag = true;
  //       Serial.println("Received start message from server, starting network setup");
  //     }
  //   }
  //   delay(10);
  // }

  // while(!all_tags_online) {
  //   Serial.println(ackReceived = true ? "Received Flag Reset" : "Received Flag Not  Reset");
  //   checkTagsOnline();
  //   delay(10);
  // }
}
////////////// WIP ///////////////////////////

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
////////////// WIP ///////////////////////////

/**************************************
************ PROGRAM SETUP ************
**************************************/

void setup() {
  Serial.begin(256000);

  setup_esp_now();
  esp_now_register_recv_cb(OnDataRecv);

  unsigned long initCheck = millis();
  bool didBreak = true; // change later
  bool isEmpty = false;

  while (millis() - initCheck < 1000) {
    // Check if empty
    for(int i = 0; i < 4; ++i) {
      for(int j = 0; j < 13; ++j) {
        if(distances[i][j] != 0.0f) {
          isEmpty = false;
          break;
        }
      }

      if(!isEmpty) {
        break;
      }
    }

    if(!isEmpty) {
      break;
    }

    delay(10);
    
  }

  // init network if no data received yet
  if (isEmpty) {
    while (!startup_success) {
      startNetworkSetup();
    }
    // sendInitializationToTag(macs[0]);
  }
}

/*************************************
************ PROGRAM LOOP ************
*************************************/
void loop() {}