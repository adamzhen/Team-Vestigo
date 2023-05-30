#include <esp_now.h>
#include <WiFi.h>

int int_value;
float float_value;
bool bool_value = true;

// MAC Address of Responder ESP32
uint8_t broadcastAddress[] = {0x54, 0x43, 0xB2, 0x7D, 0xC4, 0xC0};

typedef struct struct_message {
  char a[32];
  int b;
  float c;
  bool d;
} struct_message;

struct_message myData;

esp_now_peer_info_t peerInfo;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.print(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_send_cb(OnDataSent);

  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

}

void loop() {
  int_value = random(1,20);

  float_value = 1.3 * int_value;

  bool_value = !bool_value;

  strcpy(myData.a, "TESTING THE TEST WITH A TEST");
  myData.b = int_value;
  myData.c = float_value;
  myData.d = bool_value;

  esp_err_t result = esp_now_send(broadcastAddress, (uin8_t *) &myData, sizeof(myData));

  if (result == ESP_OK) {
    Serial.println("Sending Confirmed");
  }
  else {
    Serial.println("Sending Error");
  }

  delay(2000);

}
