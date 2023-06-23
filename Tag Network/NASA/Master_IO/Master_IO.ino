#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <ArduinoJson.h>
#include <WiFiUdp.h>
#include <Ethernet.h>

/******************************************
******** GEN VARIABLES AND STRUCTS ********
******************************************/

// PHY config
#define ETH_CLK_MODE    ETH_CLOCK_GPIO17_OUT
#define ETH_PHY_POWER   12

float distances[4][13] = {0};
bool received[4] = {false};

IPAddress server(192, 168, 8, 132);
unsigned int network_port = 1234; 
unsigned int error_port = 1235;
EthernetClient client;
volatile bool packetSent = false;
unsigned long lastDataReceivedMillis = 0;
unsigned long dataTimeoutMillis = 3000;
unsigned int resetAttempts = 0;
unsigned int resetsConfirmed = 0;

typedef struct rangingData {
  bool run_ranging;
  float data[13];
  int tag_id = 0; 
} rangingData;

typedef struct networkData {
  bool reset_chain;
  bool failed_tags[4];
} networkData;

rangingData onDeviceRangingData;
rangingData offDeviceRangingData;

networkData onDeviceNetworkData;
networkData offDeviceNetworkData;

uint8_t macs[][6] = {
  {0xD4, 0xD4, 0xDA, 0x46, 0x0C, 0xA8}, // TAG1
  {0xD4, 0xD4, 0xDA, 0x46, 0x6C, 0x6C}, // TAG2
  {0xD4, 0xD4, 0xDA, 0x46, 0x66, 0x54}, // TAG3
  {0x54, 0x43, 0xB2, 0x7D, 0xC4, 0x44}, // TAG4
  {0x54, 0x43, 0xB2, 0x7D, 0xC4, 0xC0}  // Master IO
};

/******************************************
************ NETWORK FUNCTIONS ************
******************************************/

void formatMacAddress(const uint8_t* mac, char* buffer, size_t bufferSize) {
  if(bufferSize < 18) {
    return;
  }
  sprintf(buffer, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  char macStr[18];
  formatMacAddress(mac_addr, macStr, 18);
  Serial.print("Last Packet Sent to: ");
  Serial.println(macStr);
  Serial.print("Last Packet Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");

  // If the packet was sent successfully, set the flag to true
  if (status == ESP_NOW_SEND_SUCCESS) {
    packetSent = true;
  }
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
    Serial.println("Distances received:");
    for (int i = 0; i < 13; i++) {
      Serial.print(distances[offDeviceRangingData.tag_id][i]);
      Serial.print(" ");
    }
    Serial.println();

    if(received[0] && received[1] && received[2] && received[3]) {
      sendJson();
      for(int i = 0; i < 4; i++) {
        received[i] = false;
      }
    }
  }

  // If the data is networkData
  else if (dataType == 1) {
    memcpy(&offDeviceNetworkData, incomingData + 1, sizeof(offDeviceNetworkData));
  
    // If offDeviceNetworkData.reset_chain is true, send a reset command to all devices
    if (offDeviceNetworkData.reset_chain) {
      Serial.println("Reset command received from network device");
      sendResetGlobal();
    }

    // Check if there's any failed device
    for (int i = 0; i < 4; i++) {
      if (offDeviceNetworkData.failed_tags[i]) {
        Serial.print("Tag ");
        Serial.print(i);
        Serial.println(" malfunctioning. Resetting this tag.");

        // If the first tag is malfunctioning, reset the entire network
        if (i == 0) {
          Serial.println("First tag malfunctioning. Resetting entire network.");
          sendResetGlobal();
        } else {  // If it's another tag, reset only that tag
          sendResetToTag(i);
        }
      }
    }
  }

  lastDataReceivedMillis = millis();
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
  Serial.println(jsonString);

  // Connect to server
  if (client.connect(server, network_port)) {
    // Send JSON string
    client.println(jsonString);
    client.stop(); // Close the connection
  } else {
    Serial.println("Failed to connect to server");
  }
}

void sendResetGlobal() {
  onDeviceNetworkData.reset_chain = true;
  for(int i=(sizeof(macs)/sizeof(macs[0]))-2; i>=0; i--) {
    sendToPeerNetwork(macs[i], &onDeviceNetworkData);
    waitForPacketSent();
  }
  onDeviceNetworkData.reset_chain = false;
}

void sendResetToTag(int tag_id) {
  onDeviceNetworkData.reset_chain = true;
  sendToPeerNetwork(macs[tag_id], &onDeviceNetworkData);
  waitForPacketSent();
  onDeviceNetworkData.reset_chain = false;
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
  
  // Start Ethernet
  Ethernet.init(ETH_PHY_POWER);   // power pin
  Ethernet.begin(macs[sizeof(macs)/sizeof(macs[0]) - 1], ETH_PHY_ADDR, ETH_CLK_MODE);  // Use Gateway MAC for Ethernet.begin
  delay(1000);

  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Ethernet shield was not found.");
    while (true) {
      delay(1); // do nothing, no point running without Ethernet hardware
    }
  }
  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Ethernet cable is not connected.");
  }

  setup_esp_now();
  esp_now_register_recv_cb(OnDataRecv);
}

/*************************************
************ PROGRAM LOOP ************
*************************************/

void loop() {
  // Add this part to maintain the Ethernet connection
  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Ethernet link has been lost, waiting for reconnection...");
    delay(10);
    return;
  }

  // Check if no data was received for more than dataTimeoutMillis milliseconds
  if (millis() - lastDataReceivedMillis > dataTimeoutMillis) {
    sendReset();
    resetAttempts++;
    // If we've tried resetting 5 times with no success, send an error
    if (resetAttempts > 5) {
      if (errorClient.connect(server, error_port)) {
        errorClient.println("Error: Unable to reset devices after 5 attempts.");
        errorClient.stop();
        resetAttempts = 0;
      }
    }
    // Reset lastDataReceivedMillis to avoid multiple reset commands
    lastDataReceivedMillis = millis();
  }

  delay(10);  // Delay before checking the condition again
}
