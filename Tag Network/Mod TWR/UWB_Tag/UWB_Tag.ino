#include <dw3000.h>
#include <ArduinoJson.h>
#include <WiFiUdp.h>
#include <vector>
#include <math.h>
#include <WiFi.h>
#include <algorithm>
#include <SPI.h>
#include <esp_now.h>

/******************************************
******** GEN VARIABLES AND STRUCTS ********
******************************************/

std::vector<std::pair<int, std::vector<float>>> keys;
std::vector<float> clock_offset;
std::vector<float> averages;

const int tag_id = 1;
const int num_tags = 2;
volatile bool ackReceived = false;

unsigned long lastRangingTime = 0;
const unsigned long UWBtimeoutDuration = 1000;

typedef struct __attribute__((packed)) rangingData {
  float data[13];
  int tag_id = 0;
  bool run_ranging; 
} rangingData;

typedef struct __attribute__((packed)) ackData {
  bool ack;
} ackData;

rangingData onDeviceRangingData;
rangingData offDeviceRangingData;

uint8_t MIOmac[6] = {0x08, 0x3A, 0x8D, 0x83, 0x44, 0x10};  // Master IO

/*******************************************
************ GEN CONFIG OPTIONS ************
*******************************************/
 
// connection pins
const uint8_t PIN_RST = 27; // reset pin
const uint8_t PIN_IRQ = 34; // irq pin
const uint8_t PIN_SS = 4; // spi select pin

#define TX_ANT_DLY 16385
#define RX_ANT_DLY 16385
#define ALL_MSG_COMMON_LEN 10
#define ALL_MSG_SN_IDX 2
#define RESP_MSG_POLL_RX_TS_IDX 10
#define RESP_MSG_RESP_TX_TS_IDX 14
#define RESP_MSG_TS_LEN 4
#define POLL_TX_TO_RESP_RX_DLY_UUS 240
#define RESP_RX_TIMEOUT_UUS 400
 
/* Default communication configuration. We use default non-STS DW mode. */
static dwt_config_t config = {
  5,               /* Channel number. */
  DWT_PLEN_128,    /* Preamble length. Used in TX only. */
  DWT_PAC8,        /* Preamble acquisition chunk size. Used in RX only. */
  9,               /* TX preamble code. Used in TX only. */
  9,               /* RX preamble code. Used in RX only. */
  1,               /* 0 to use standard 8 symbol SFD, 1 to use non-standard 8 symbol, 2 for non-standard 16 symbol SFD and 3 for 4z 8 symbol SDF type */
  DWT_BR_6M8,      /* Data rate. */
  DWT_PHRMODE_STD, /* PHY header mode. */
  DWT_PHRRATE_STD, /* PHY header rate. */
  (129 + 8 - 8),   /* SFD timeout (preamble length + 1 + SFD length - PAC size). Used in RX only. */
  DWT_STS_MODE_OFF, /* STS disabled */
  DWT_STS_LEN_64,/* STS length see allowed values in Enum dwt_sts_lengths_e */
  DWT_PDOA_M0      /* PDOA mode off */
};
 
static uint32_t status_reg = 0;
static uint64_t poll_rx_ts;
static uint64_t resp_tx_ts;
extern dwt_txconfig_t txconfig_options;

/******************************************
************ NETWORK FUNCTIONS ************
******************************************/

void sendAck(const uint8_t *peerMAC) {
  ackData message;
  message.ack = true;

  uint8_t buf[sizeof(ackData) + 1];
  buf[0] = 1;  // ackData type identifier
  memcpy(&buf[1], &message, sizeof(ackData));

  esp_now_send(peerMAC, buf, sizeof(buf));
}

bool waitForAck() {
  unsigned long startMillis = millis();
  while(!ackReceived) {
    delay(5);
    if (millis() - startMillis > 100) {  // Adjust timeout as needed
      return false;
    }
  }
  ackReceived = false;  // Reset the flag for the next transmission
  return true;
}

// Callback when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  uint8_t dataType = incomingData[0];
  
  // If the data is networkData
  if (dataType == 0) {
    memcpy(&offDeviceRangingData, incomingData + 1, sizeof(offDeviceRangingData));
    if (!offDeviceRangingData.run_ranging) {
      ESP.restart();
    }
    sendAck(mac);
  }
  // If the data is ackData
  else if (dataType == 1) {
    ackReceived = true;
  }
}

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


void setup_esp_now() {
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESPNow with a fallback logic
  WiFi.disconnect();
  if (esp_now_init() != ESP_OK) {
    ESP.restart();
  }

  // Register for Receive CB to get incoming data
  esp_now_register_recv_cb(OnDataRecv);

  // Register peers
  for(int i=0; i<sizeof(MIOmac)/sizeof(MIOmac); i++) {
    esp_now_peer_info_t peerInfo;
    memcpy(peerInfo.peer_addr, MIOmac, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    peerInfo.ifidx = WIFI_IF_STA;

    // Add peer        
    if (esp_now_add_peer(&peerInfo) != ESP_OK){
      return;
    }
  }

  // Register MIO
  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, MIOmac, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  peerInfo.ifidx = WIFI_IF_STA;

  // Add MIO      
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    return;
  }
}

/*****************************************
************ TWR TRANSMISTTER ************
*****************************************/

void twr_transmitter_mode(int key, double& tof) {
  uint8_t frame_seq_nb = 0;
  uint8_t rx_buffer[20];

  uint8_t tx_poll_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'W', 'A', (uint8_t) key, 'E', 0xE0, 0, 0};
  uint8_t rx_resp_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, (uint8_t) key, 'E', 'W', 'A', 0xE1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  /* Write frame data to DW IC and prepare transmission. See NOTE 7 below. */
  tx_poll_msg[ALL_MSG_SN_IDX] = frame_seq_nb;
  dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS_BIT_MASK);
  dwt_writetxdata(sizeof(tx_poll_msg), tx_poll_msg, 0); /* Zero offset in TX buffer. */
  dwt_writetxfctrl(sizeof(tx_poll_msg), 0, 1); /* Zero offset in TX buffer, ranging. */

  /* Start transmission, indicating that a response is expected so that reception is enabled automatically after the frame is sent and the delay
   * set by dwt_setrxaftertxdelay() has elapsed. */
  dwt_starttx(DWT_START_TX_IMMEDIATE | DWT_RESPONSE_EXPECTED);

  /* We assume that the transmission is achieved correctly, poll for reception of a frame or error/timeout. See NOTE 8 below. */
  while (!((status_reg = dwt_read32bitreg(SYS_STATUS_ID)) & (SYS_STATUS_RXFCG_BIT_MASK | SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR)))
  { 
  };

  /* Increment frame sequence number after transmission of the poll message (modulo 256). */
  frame_seq_nb++;

  if (status_reg & SYS_STATUS_RXFCG_BIT_MASK)
  {
    uint32_t frame_len;

    /* Clear good RX frame event in the DW IC status register. */
    dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG_BIT_MASK);

    /* A frame has been received, read it into the local buffer. */
    frame_len = dwt_read32bitreg(RX_FINFO_ID) & RXFLEN_MASK;
    if (frame_len <= sizeof(rx_buffer))
    {
      dwt_readrxdata(rx_buffer, frame_len, 0);

      /* Check that the frame is the expected response from the companion "SS TWR responder" example.
       * As the sequence number field of the frame is not relevant, it is cleared to simplify the validation of the frame. */
      rx_buffer[ALL_MSG_SN_IDX] = 0;
      if (memcmp(rx_buffer, rx_resp_msg, ALL_MSG_COMMON_LEN) == 0)
      {
        uint32_t poll_tx_ts, resp_rx_ts, poll_rx_ts, resp_tx_ts;
        int32_t rtd_init, rtd_resp;
        float clockOffsetRatio;

        /* Retrieve poll transmission and response reception timestamps. See NOTE 9 below. */
        poll_tx_ts = dwt_readtxtimestamplo32();
        resp_rx_ts = dwt_readrxtimestamplo32();

        /* Read carrier integrator value and calculate clock offset ratio. See NOTE 11 below. */
        clockOffsetRatio = ((float)dwt_readclockoffset()) / (uint32_t)(1 << 26);

        /* Get timestamps embedded in response message. */
        resp_msg_get_ts(&rx_buffer[RESP_MSG_POLL_RX_TS_IDX], &poll_rx_ts);
        resp_msg_get_ts(&rx_buffer[RESP_MSG_RESP_TX_TS_IDX], &resp_tx_ts);

        /* Compute time of flight and distance, using clock offset ratio to correct for differing local and remote clock rates */
        rtd_init = resp_rx_ts - poll_tx_ts;
        rtd_resp = resp_tx_ts - poll_rx_ts;

        tof = ((rtd_init - rtd_resp * (1 - clockOffsetRatio)) / 2.0) * DWT_TIME_UNITS;
      }
    }
  }
  else
  {
    /* Clear RX error/timeout events in the DW IC status register. */
    dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR);
  }

  /* Execute a delay between ranging exchanges. */
  delayMicroseconds(700);
}
 
/******************************************
************ ADVANCED RANGING ************
******************************************/

void advancedRanging()
{
  bool looping = true;

  // Distance reset        
  for (auto& key : keys) {
    key.second.clear();
  }

  lastRangingTime = millis();

  // Loop between keys until exit conditions are met
  while(looping)
  {
    //if ((millis() - lastRangingTime) > UWBtimeoutDuration) {
      //ESP.restart();
    //}

    for (int i = 0; i < 7; i++) 
    {
      const auto& key = keys[i];
      float distance = 0;
      double tof = 0;

      twr_transmitter_mode(key.first + 1, tof);                  
      distance = tof * SPEED_OF_LIGHT;

      if (distance != 0) 
      {
        // Find the correct position in the keys vector for the current anchor
        auto it = std::find_if(keys.begin(), keys.end(), [&](const std::pair<int, std::vector<float>>& element) {
            return element.first == key.first;
        });

        // If the anchor was found in the keys vector, append the distance
        if (it != keys.end()) {
            it->second.push_back(distance);
        }

        // counter
        int unique_distance_counter = 0;
        int total_distance_counter = 0;

        for (int i = 0; i < 12; i++) 
        {
          if (keys[i].second.size() >= 2) 
          {
            unique_distance_counter += 1;
          } 
          if (keys[i].second.size() >= 1) {
            total_distance_counter += 1;
          }
        }

        // checks if there is enough data to send
        if (unique_distance_counter >= 5) 
        {

          for (int i = 0; i < 12; ++i) {
            if (!keys[i].second.empty()) {
              float sum = 0;
              for (float d : keys[i].second) {
                sum += d;
              }
              // Replace the distance vector with its average
              int key_size = keys[i].second.size();
              keys[i].second.clear();
              keys[i].second.push_back(sum / key_size);
            } else {
              keys[i].second.push_back(0);
            }
          }

          std::vector<std::pair<int, std::vector<float>>> sortedKeys = keys;
          std::sort(sortedKeys.begin(), sortedKeys.end(), [](const std::pair<int, std::vector<float>>& a, const std::pair<int, std::vector<float>>& b) {
            return a.first < b.first;
          });

          for (int i = 0; i < sortedKeys.size(); i++) {
            onDeviceRangingData.data[i] = sortedKeys[i].second[0];
          }

          sendToPeer(MIOmac, &onDeviceRangingData);

          // Sort Key Order
          std::sort(keys.begin(), keys.end(), [](const std::pair<int, std::vector<float>>& a, const std::pair<int, std::vector<float>>& b) {
            return a.second[0] < b.second[0];
          });

          // Move the first 5 elements to the end
          std::rotate(keys.begin(), keys.begin() + (12 - total_distance_counter), keys.end());

          if(keys.size() < 9) // check if we have enough elements to swap
          return;

          // Save the original elements in temporary variables
          std::pair<int, std::vector<float>> temp_7 = keys[6];
          std::pair<int, std::vector<float>> temp_8 = keys[7];
          std::pair<int, std::vector<float>> temp_9 = keys[8];

          keys[6] = keys[5];
          keys[8] = temp_7;
          keys[5] = temp_8;
          keys[7] = temp_9;

          looping = false;
          break;
        }  
      }
    }
  }
}

/**************************************
************ PROGRAM SETUP ************
**************************************/

void setup() {
  setup_esp_now();

  UART_init();

  spiBegin(PIN_IRQ, PIN_RST);
  spiSelect(PIN_SS);

  delay(2); // Time needed for DW3000 to start up (transition from INIT_RC to IDLE_RC, or could wait for SPIRDY event)

  while (!dwt_checkidlerc()) // Need to make sure DW IC is in IDLE_RC before proceeding
  {
    UART_puts("IDLE FAILED\r\n");
    while (1)
      ;
  }

  if (dwt_initialise(DWT_DW_INIT) == DWT_ERROR)
  {
    UART_puts("INIT FAILED\r\n");
    while (1)
      ;
  }

  // Enabling LEDs here for debug so that for each TX the D1 LED will flash on DW3000 red eval-shield boards.
  dwt_setleds(DWT_LEDS_ENABLE | DWT_LEDS_INIT_BLINK);

  /* Configure DW IC. See NOTE 6 below. */
  if (dwt_configure(&config)) // if the dwt_configure returns DWT_ERROR either the PLL or RX calibration has failed the host should reset the device
  {
    UART_puts("CONFIG FAILED\r\n");
    while (1)
      ;
  }

  /* Configure the TX spectrum parameters (power, PG delay and PG count) */
  dwt_configuretxrf(&txconfig_options);

  /* Apply default antenna delay value. See NOTE 2 below. */
  dwt_setrxantennadelay(RX_ANT_DLY);
  dwt_settxantennadelay(TX_ANT_DLY);

  /* Set expected response's delay and timeout. See NOTE 1 and 5 below.
   * As this example only handles one incoming frame with always the same delay and timeout, those values can be set here once for all. */
  dwt_setrxaftertxdelay(POLL_TX_TO_RESP_RX_DLY_UUS);
  dwt_setrxtimeout(RESP_RX_TIMEOUT_UUS);

  /* Next can enable TX/RX states output on GPIOs 5 and 6 to help debug, and also TX/RX LEDs
   * Note, in real low power applications the LEDs should not be used. */
  dwt_setlnapamode(DWT_LNA_ENABLE | DWT_PA_ENABLE);

  for (int i = 0; i < 12; ++i) {
    keys.push_back(std::make_pair(i, std::vector<float>()));
  }  

  onDeviceRangingData.tag_id = tag_id - 1;
}

/*************************************
************ PROGRAM LOOP ************
*************************************/

void loop() {
  if (offDeviceRangingData.run_ranging) {
    offDeviceRangingData.run_ranging = false;

    advancedRanging();
  }
}

