#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <ArduinoJson.h>

// Replace with your network credentials
const char* ssid = "your_SSID";
const char* password = "your_PASSWORD";

// Structure example to send data
// Must match the receiver structure
typedef struct struct_message {
  bool run_ranging;
  float data[13];  // Assuming there are 12 elements
  int tag_id = 0; 
} struct_message;

float distances[4][13] = {0};
int uniqueArraysReceived = 0;

// Create a struct_message to hold incoming readings
struct_message incomingReadings;

void formatMacAddress(const uint8_t* mac, char* buffer, size_t bufferSize) {
  if(bufferSize < 18) {
    return; // Buffer is too small
  }
  sprintf(buffer, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

// Callback when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));
  Serial.print("tag_id: ");
  Serial.println(incomingReadings.tag_id);

  // Check if this array was already received
  if (distances[incomingReadings.tag_id][13] == 0) {  // Assuming 13th element of each array is used as a flag
    // Copy the received data to the corresponding array in distances
    memcpy(distances[incomingReadings.tag_id], incomingReadings.data, sizeof(incomingReadings.data));
    distances[incomingReadings.tag_id][13] = 1;  // Set the flag to indicate that this array is received
    uniqueArraysReceived++;

    // For debugging: print the received distances
    Serial.println("Distances received:");
    for (int i = 0; i < 13; i++) {
      Serial.print(distances[incomingReadings.tag_id][i]);
      Serial.print(" ");
    }
    Serial.println();

    // If we received 4 unique arrays, send the JSON object and clear the distances array
    if (uniqueArraysReceived == 4) {
      sendJson();
      memset(distances, 0, sizeof(distances));  // Clear the distances array
      uniqueArraysReceived = 0;  // Reset the counter
    }
  }
}

void sendJson() {
  DynamicJsonDocument doc(1024);  // Adjust the capacity according to your needs

  for (int tag_id = 0; tag_id < 4; tag_id++) {  // Assuming tag_id is from 0 to 3, adjust accordingly
    JsonArray data = doc.createNestedArray(String(tag_id));
    for (int i = 0; i < 12; i++) {
      data.add(distances[tag_id][i]);
    }
  }

  // Now doc contains all the distances, you can serialize it to a string and transmit it
  String jsonString;
  serializeJson(doc, jsonString);

  // Transmit jsonString using WiFi, or any other method
  // ...
}

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESPNow with a fallback logic
  WiFi.disconnect();
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESPNow Init Failed");
    ESP.restart();
  }
  Serial.println("ESPNow Init Success");

  // Register for Receive CB to get incoming data
  esp_now_register_recv_cb(OnDataRecv);

  // TODO: Add code to connect to WiFi here
}

void loop() {
  // Nothing to do here
}
