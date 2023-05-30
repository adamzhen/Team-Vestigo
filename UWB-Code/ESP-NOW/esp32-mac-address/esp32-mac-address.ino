#include <Wifi.h>

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_MODE_STA);

  delay(2000);

  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());
}

void loop() {

}
