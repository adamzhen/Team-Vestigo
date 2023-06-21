#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <ArduinoJson.h>
#include <WiFiUdp.h>
#include <Ethernet.h>

// PHY config
#define ETH_CLK_MODE    ETH_CLOCK_GPIO17_OUT
#define ETH_PHY_POWER   12

// Add your server's IP address
IPAddress server(192, 168, 8, 132);
unsigned int port = 1234; // Replace with your port

EthernetClient client;

// MAC addresses
uint8_t macs[][6] = {
  {0xD4, 0xD4, 0xDA, 0x46, 0x0C, 0xA8}, // TAG1
  {0xD4, 0xD4, 0xDA, 0x46, 0x6C, 0x6C}, // TAG2
  {0xD4, 0xD4, 0xDA, 0x46, 0x66, 0x54}, // TAG3
  {0x54, 0x43, 0xB2, 0x7D, 0xC4, 0x44}, // TAG4
  {0x54, 0x43, 0xB2, 0x7D, 0xC4, 0xC0}  // GATEWAY
};

// Global variable to indicate if ESP-NOW data was sent
volatile bool packetSent = false;

unsigned long lastDataReceivedMillis = 0;
unsigned long dataTimeoutMillis = 3000; // Set timeout to 3 seconds

// Variables to keep track of reset attempts
unsigned int resetAttempts = 0;
unsigned int resetsConfirmed = 0;

// Structure example to send data
// Must match the receiver structure
typedef struct struct_message {
  bool run_ranging;
  bool reset_chain;
  float data[13];  // Assuming there are 12 elements
  int tag_id = 0; 
} struct_message;


// Arrays to hold the distances
float distances[4][13] = {0};
bool received[4] = {false};

// Create a struct_message called myData
struct_message myData;

// Create a struct_message to hold incoming readings
struct_message incomingReadings;

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
  memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));
  if(incomingReadings.tag_id >= 0 && incomingReadings.tag_id <= 3 && !received[incomingReadings.tag_id]) {
    memcpy(distances[incomingReadings.tag_id], incomingReadings.data, sizeof(incomingReadings.data));
    received[incomingReadings.tag_id] = true;

    // For debugging: print the received distances
    Serial.println("Distances received:");
    for (int i = 0; i < 13; i++) {
      Serial.print(distances[incomingReadings.tag_id][i]);
      Serial.print(" ");
    }
    Serial.println();
  }
  if(received[0] && received[1] && received[2] && received[3]) {
    sendJson();
    for(int i = 0; i < 4; i++) {
      received[i] = false;
    }
  }
  // If myData.reset_chain is true, increment resetsConfirmed
  if (myData.reset_chain) {
    resetsConfirmed++;
    if (resetsConfirmed == (sizeof(macs)/sizeof(macs[0]))-1) { // if all devices have been reset
      resetAttempts = 0;
      resetsConfirmed = 0;
    }
  }
  // Update the last data received time
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
  if (client.connect(server, port)) {
    // Send JSON string
    client.println(jsonString);
    client.stop(); // Close the connection
  } else {
    Serial.println("Failed to connect to server");
  }
}

void sendReset() {
  // Prepare and send reset message to each device in the chain
  myData.reset_chain = true;
  for(int i=(sizeof(macs)/sizeof(macs[0]))-2; i>=0; i--) {  // Start from the one before last, since the last one is Gateway.
    sendToPeer(macs[i], &myData);
    waitForPacketSent();
  }
  myData.reset_chain = false;
}

void setup() 
{
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

void loop() 
{
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
      if (errorClient.connect(server, 1235)) {
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
