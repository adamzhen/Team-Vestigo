#include <WiFi.h>

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  
  // Print the MAC address to the Serial Monitor
  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());
}

void loop() {
  // Nothing to do here
}
