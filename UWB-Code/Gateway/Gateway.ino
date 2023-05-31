#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <ArduinoJson.h>
#include <WiFiUdp.h>

// Replace with your network credentials
const char* ssid = "Vestigo-Router";
const char* password = "Vestigo&2023";
const char* host = "192.168.8.132";
unsigned int port = 1234; // Replace with your port

// Structure example to send data
// Must match the receiver structure
typedef struct struct_message {
  bool run_ranging;
  float data[13];  // Assuming there are 12 elements
  int tag_id = 0; 
} struct_message;

// Create a WiFiUDP object
WiFiUDP udp;

// Arrays to hold the distances
float distances[4][13] = {0};
bool received[4] = {false};
struct_message incomingReadings;

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
  // Send the data over UDP
  IPAddress ip;
  if (WiFi.hostByName(host, ip)) 
  {
    // Send the Json data over the socket connection
    udp.beginPacket(ip, port);
    udp.write((uint8_t*)jsonString.c_str(), jsonString.length());
    udp.endPacket();
  } 
  else 
  {
    Serial.println("Unable to resolve hostname");
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESPNow Init Failed");
    ESP.restart();
  }
  esp_now_register_recv_cb(OnDataRecv);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting...");
  }
  udp.begin(port);
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  // nothing here
}