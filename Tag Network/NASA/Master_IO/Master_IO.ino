#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <ArduinoJson.h>
#include <WiFiUdp.h>
#include <Ethernet.h>
#include <SPI.h>

/******************************************
******** GEN VARIABLES AND STRUCTS ********
******************************************/

// PHY config
#define ETH_CLK_MODE 0
#define ETH_PHY_POWER 12
#define ETH_PHY_ADDR 0

float distances[4][13] = {0};
bool online_tags[4] = {false};
bool received[4] = {false};
bool all_tags_online = false;
int attempts[4] = {0};
bool startup_success = false;

IPAddress server(192, 168, 1, 177);
unsigned int network_port = 1234; 
unsigned int error_port = 1235;
EthernetClient client;
EthernetClient errorClient;
bool startNetworkSetupFlag = false;

volatile bool ackReceived = false;
unsigned int resetAttempts = 0;
unsigned int resetsConfirmed = 0;

typedef struct __attribute__((packed)) rangingData {
  float data[13];
  int tag_id = 0; 
} rangingData;


typedef struct __attribute__((packed)) networkData {
  bool run_ranging;
  bool network_initialize;
} networkData;

typedef struct __attribute__((packed)) ackData {
  bool ack;
} ackData;

rangingData onDeviceRangingData;
rangingData offDeviceRangingData;

networkData onDeviceNetworkData;
networkData offDeviceNetworkData;

uint8_t macs[][6] = {
  {0xD4, 0xD4, 0xDA, 0x46, 0x0C, 0xA8}, // TAG1
  {0xD4, 0xD4, 0xDA, 0x46, 0x6C, 0x6C}, // TAG2
  {0xD4, 0xD4, 0xDA, 0x46, 0x66, 0x54}, // TAG3
  {0x54, 0x43, 0xB2, 0x7D, 0xC4, 0x44}, // TAG4
  {0x08, 0x3A, 0x8D, 0x83, 0x44, 0x10}  // Master IO
};

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 0, 0);

/******************************************
************ NETWORK FUNCTIONS ************
******************************************/

void sendAck(const uint8_t *peerMAC) {
  esp_err_t result;
  ackData ack;
  ack.ack = true;
  uint8_t buf[sizeof(ackData) + 1];
  buf[0] = 2;
  memcpy(&buf[1], &ack, sizeof(ackData));
  result = esp_now_send(peerMAC, buf, sizeof(buf));
  if (result == ESP_OK) {
    Serial.println("Ack sent");
  } else {
    Serial.println("Error sending ack");
  }
}

bool waitForAck(uint8_t retries) {
  ackReceived = false;  // Reset the acknowledgement flag

  while (retries--) {
    delay(10);
    if (ackReceived) {
      return true;
    }
  }
  return false;
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("Last Packet Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void sendJson() {
  DynamicJsonDocument doc(1024);
  for(int tag_id = 0; tag_id < 4; tag_id++) {
    JsonArray data = doc.createNestedArray(String(tag_id+1));
    for(int i = 0; i < 13; i++) {
      data.add(distances[tag_id][i]);
    }
  }
  String jsonString;
  serializeJson(doc, jsonString);
  Serial.println("Sending JSON: ");
  // Serial.println(jsonString);

  // // Connect to server
  // if (client.connect(server, network_port)) {
  //   // Send JSON string
  //   client.println(jsonString);
  //   client.stop(); // Close the connection
  // } else {
  //   Serial.println("Failed to connect to server");
  // }
}

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  // Assuming the first byte in incomingData determines the type of data
  uint8_t dataType = incomingData[0];

  // If the data is rangingData
  if (dataType == 0) {
    memcpy(&offDeviceRangingData, incomingData + 1, sizeof(offDeviceRangingData));
    memcpy(distances[offDeviceRangingData.tag_id], offDeviceRangingData.data, sizeof(offDeviceRangingData.data));
    received[offDeviceRangingData.tag_id] = true;

    // For debugging: print the received distances
    Serial.print("Tag ID: ");
    Serial.println(offDeviceRangingData.tag_id + 1);
    // Serial.println("Distances received:");
    // for (int i = 0; i < 13; i++) {
    //   Serial.print(distances[offDeviceRangingData.tag_id][i]);
    //   Serial.print(" ");
    // }
    // Serial.println();

    if(received[0] && received[1] && received[2] && received[3]) {
      sendJson();
      for(int i = 0; i < 4; i++) {
        received[i] = false;
      }
    }
  }

  // If the data is networkData (potentially not necessary)
  else if (dataType == 1) {
    memcpy(&offDeviceNetworkData, incomingData + 1, sizeof(offDeviceNetworkData));
  }
  
  else if (dataType == 2) {
    Serial.println("Acknowledgement received");
    ackReceived = true;
  }
}

void sendToPeerNetwork(uint8_t *peerMAC, networkData *message, int retries = 3) {
  esp_err_t result;
  uint8_t buf[sizeof(networkData) + 1];  // Buffer to hold type identifier and data
  buf[0] = 1;  // NetworkData type identifier
  memcpy(&buf[1], message, sizeof(networkData));  // Copy networkData to buffer

  for (int i = 0; i < retries; i++) {
    ackReceived = false; // Reset ackReceived before sending the message
    result = esp_now_send(peerMAC, buf, sizeof(buf));  // Send the buffer
    if (result == ESP_OK) {
      Serial.println("Sent networkData success");
      break;
    } else {
      Serial.println("Error sending the networkData");
      if (i < retries - 1) {
        delay(250);
      }
    }
  }
  waitForAck(1);
}

void sendInitializationToTag(uint8_t *tag_mac) {
  onDeviceNetworkData.network_initialize = true;
  sendToPeerNetwork(tag_mac, &onDeviceNetworkData);
  onDeviceNetworkData.network_initialize = false;
}

void sendNetworkPoll(uint8_t *tag_mac) {
  onDeviceNetworkData.network_initialize = false;
  onDeviceNetworkData.run_ranging = false;
  sendToPeerNetwork(tag_mac, &onDeviceNetworkData);
}

void checkTagsOnline() {
  static uint8_t tag_poll_index = 0;

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

  // // Debug: Print the online status of all tags and the value of all_tags_online
  // for (int i = 0; i < 4; i++) {
  //   Serial.print("Tag ");
  //   Serial.print(i);
  //   Serial.print(" online status: ");
  //   Serial.println(online_tags[i] ? "Online" : "Offline");
  // }
  // Serial.print("All tags online: ");
  // Serial.println(all_tags_online ? "Yes" : "No");

  // If not all tags are online, continue polling
  if (!all_tags_online) {
    Serial.println("Sending polling request to tag " + String(tag_poll_index));
    sendNetworkPoll(macs[tag_poll_index]);
    ackReceived = false;
    waitForAck(2);
  } else {
    // If all tags are online, set startup_success to true
    startup_success = true;
  }
}



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
  while(!startNetworkSetupFlag) {
    if (client.available() > 0) {
      String serverMsg = client.readStringUntil('\n');

      if (serverMsg == "start") {
        startNetworkSetupFlag = true;
        Serial.println("Received start message from server, starting network setup");
      }
    }
    delay(10);
  }

  while(!all_tags_online) {
    checkTagsOnline();
    delay(10);
  }
}

void setup_esp_now() {
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESPNow with a fallback logic
  WiFi.disconnect();
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESPNow Init Failed");
    ESP.restart();
  }
  Serial.println("ESPNow Init Success");

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Transmitted packet
  esp_now_register_send_cb(OnDataSent);

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
  Serial.begin(115200);

  Serial.println("");
  Serial.println("NEW RUN NEW RUN NEW RUN");
  Serial.println("");
  
  // // Start Ethernet
  // Ethernet.init(ETH_PHY_POWER);   // power pin
  // Ethernet.begin(mac, server, );
  // delay(1000);

  // // Check for Ethernet hardware present
  // if (Ethernet.hardwareStatus() == EthernetNoHardware) {
  //   Serial.println("Ethernet shield was not found.");
  //   while (true) {
  //     delay(1); // do nothing, no point running without Ethernet hardware
  //   }
  // }
  // if (Ethernet.linkStatus() == LinkOFF) {
  //   Serial.println("Ethernet cable is not connected.");
  // }

  setup_esp_now();
  esp_now_register_recv_cb(OnDataRecv);

  while (!startup_success) {
    startNetworkSetup();
  }
  
  sendInitializationToTag(macs[0]);
}

/*************************************
************ PROGRAM LOOP ************
*************************************/

void loop() {
  // Add this part to maintain the Ethernet connection
  // if (Ethernet.linkStatus() == LinkOFF) {
  //   Serial.println("Ethernet link has been lost, waiting for reconnection...");
  //   delay(10);
  //   return;
  // }
  delay(10);  // Delay before checking the condition again
}
