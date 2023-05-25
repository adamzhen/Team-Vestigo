#include <dw3000.h>
#include <ArduinoJson.h>
#include <WiFiUdp.h>
#include <vector>
#include <math.h>
#include <WiFi.h>
#include <algorithm>

// IP Addresses
const char *Aiden_laptop = "192.168.8.101";
const char *Evan_laptop = "192.168.8.162";
const char *Adam_laptop = "192.168.8.203";
const char *Aiden_PC = "192.168.8.122";
const char *Aiden_laptop_LAN = "192.168.8.219";
const char *Adam_laptop_LAN = "192.168.8.148";
const char *Aiden_PC_LAN = "192.168.8.132";

// Wifi creds
const char *ssid = "Vestigo-Router";
const char *password = "Vestigo&2023";
const char *host = Aiden_PC_LAN;
const int port = 1234;


// list creation
std::vector<std::pair<int, std::vector<float>>> keys;
std::vector<float> clock_offset;
std::vector<float> averages;



/* Hold copies of computed time of flight and distance here for reference so that it can be examined at a debug breakpoint. */
static double tof;
static double distance;

/*******************************************
************ GEN CONFIG OPTIONS ************
*******************************************/
 
// connection pins
const uint8_t PIN_RST = 27; // reset pin
const uint8_t PIN_IRQ = 34; // irq pin
const uint8_t PIN_SS = 4; // spi select pin

#define RNG_DELAY_MS 1
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

/**********************************************
************ TWR TRANSMISTTER MODE ************
**********************************************/

void twr_transmitter_mode(int key, double& tof)
{
  
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
  delayMicroseconds(750);
}

// /******************************************
// ************ TWR RECEIVER MODE ************
// ******************************************/
 
// void twr_receiver_mode(int key, bool& received)
// {
//   while(true) 
//   {
//     uint8_t rx_poll_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'W', 'A', (uint8_t) key, 'E', 0xE0, 0, 0};
//     uint8_t tx_resp_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, (uint8_t) key, 'E', 'W', 'A', 0xE1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

//     /* Frame sequence number, incremented after each transmission. */
//     uint8_t frame_seq_nb = 0;

//     /* Buffer to store received messages.
//     Its size is adjusted to longest frame that this example code is supposed to handle. */
//     int RX_BUF_LEN = 12; //Must be less than FRAME_LEN_MAX_EX
//     uint8_t rx_buffer[RX_BUF_LEN];

//     /* Delay between frames, in UWB microseconds. See NOTE 1 below. */
//     #define POLL_RX_TO_RESP_TX_DLY_UUS 450

//     /* Activate reception immediately. */
//     dwt_rxenable(DWT_START_RX_IMMEDIATE);
    
//     /* Poll for reception of a frame or error/timeout. See NOTE 6 below. */
//     while (!((status_reg = dwt_read32bitreg(SYS_STATUS_ID)) & (SYS_STATUS_RXFCG_BIT_MASK | SYS_STATUS_ALL_RX_ERR)))
//     { };
    
//     if (!(status_reg & SYS_STATUS_RXFCG_BIT_MASK))
//     {
//       /* Clear RX error events in the DW IC status register. */
//       dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_ERR);
//     }

//     uint32_t frame_len;
    
//     /* Clear good RX frame event in the DW IC status register. */
//     dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG_BIT_MASK);
    
//     /* A frame has been received, read it into the local buffer. */
//     frame_len = dwt_read32bitreg(RX_FINFO_ID) & RXFLEN_MASK;
//     if (frame_len > sizeof(rx_buffer))
//     {
//       break;
//     }
      
//     dwt_readrxdata(rx_buffer, frame_len, 0);
    
//     /* Check that the frame is a poll sent by "SS TWR initiator" example.
//     As the sequence number field of the frame is not relevant, it is cleared to simplify the validation of the frame. */
//     rx_buffer[ALL_MSG_SN_IDX] = 0;
//     if (memcmp(rx_buffer, rx_poll_msg, ALL_MSG_COMMON_LEN) != 0)
//     {
//       break;
//     }
        
//     uint32_t resp_tx_time;
//     int ret;
    
//     /* Retrieve poll reception timestamp. */
//     poll_rx_ts = get_rx_timestamp_u64();
    
//     /* Compute response message transmission time. See NOTE 7 below. */
//     resp_tx_time = (poll_rx_ts + (POLL_RX_TO_RESP_TX_DLY_UUS * UUS_TO_DWT_TIME)) >> 8;
//     dwt_setdelayedtrxtime(resp_tx_time);
    
//     /* Response TX timestamp is the transmission time we programmed plus the antenna delay. */
//     resp_tx_ts = (((uint64_t)(resp_tx_time & 0xFFFFFFFEUL)) << 8) + TX_ANT_DLY;
    
//     /* Write all timestamps in the final message. See NOTE 8 below. */
//     resp_msg_set_ts(&tx_resp_msg[RESP_MSG_POLL_RX_TS_IDX], poll_rx_ts);
//     resp_msg_set_ts(&tx_resp_msg[RESP_MSG_RESP_TX_TS_IDX], resp_tx_ts);
    
//     /* Write and send the response message. See NOTE 9 below. */
//     tx_resp_msg[ALL_MSG_SN_IDX] = frame_seq_nb;
//     dwt_writetxdata(sizeof(tx_resp_msg), tx_resp_msg, 0); /* Zero offset in TX buffer. */
//     dwt_writetxfctrl(sizeof(tx_resp_msg), 0, 1); /* Zero offset in TX buffer, ranging. */
//     ret = dwt_starttx(DWT_START_TX_DELAYED);
    
//     /* If dwt_starttx() returns an error, abandon this ranging exchange and proceed to the next one. See NOTE 10 below. */
//     if (ret != DWT_SUCCESS)
//     {
//       break;
//     }
      
//     /* Poll DW IC until TX frame sent event set. See NOTE 6 below. */
//     while (!(dwt_read32bitreg(SYS_STATUS_ID) & SYS_STATUS_TXFRS_BIT_MASK))
//     { };
    
//     /* Clear TXFRS event. */
//     dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS_BIT_MASK);
    
//     /* Increment frame sequence number after transmission of the poll message (modulo 256). */
//     frame_seq_nb++;

//     received = true;
//   }
// }

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
  Serial.println("Reset Distance Data");

  // Loop between keys until exit conditions are met
  while(looping)
  {
    for (int i = 0; i < 7; i++) 
    {
      const auto& key = keys[i];
      float distance = 0;
      double tof = 0;

      Serial.print("Key: ");
      Serial.println(key.first);
      twr_transmitter_mode(key.first, tof);                  
      distance = tof * SPEED_OF_LIGHT;

      Serial.println("Transmitted");

      if (distance != 0) 
      {
        // Append data to the appropriate vector
        keys[key.first - 1].second.push_back(distance);
        Serial.println("Distance Data Append");

        // counter
        int unique_distance_counter = 0;
        int total_distance_counter = 0;

        for (int i = 0; i < 12; i++) 
        {
          Serial.println(keys[i].second.size());
          if (keys[i].second.size() >= 2) 
          {
            unique_distance_counter += 1;
          } 
          if (keys[i].second.size() >= 1) {
            total_distance_counter += 1;
          }
        }
        Serial.print("Distance Counter Added: ");
        Serial.println(unique_distance_counter);

        // checks if there is enough data to send
        if (unique_distance_counter >= 5) 
        {
          // resets averages
          std::vector<float> averages(12, 0);

          // find average of each distance list
          for (int i = 0; i < 12; ++i) {
            if (!keys[i].second.empty()) {
              float sum = 0;
              for (float d : keys[i].second) {
                sum += d;
              }
            averages[i] = sum / keys[i].second.size();
            } else {
              averages[i] = 0;
            }
          }

          // convert vector into Json array
          const size_t capacity = JSON_ARRAY_SIZE(12);
          DynamicJsonDocument doc(capacity);
          JsonArray averaged_points = doc.to<JsonArray>();
          for (int i = 0; i < averages.size(); i++)
          {
            averaged_points.add(averages[i]);
            Serial.print("Averaged Points: ");
            Serial.println(averages[i]);
          }

          // convert the Json array to a string
          String jsonString;
          serializeJson(doc, jsonString);

          // Create a UDP connection to the laptop
          WiFiUDP udp;
          udp.begin(port);
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

          // Sort Key Order
          std::sort(keys.begin(), keys.end(), [](const std::pair<int, std::vector<float>>& a, const std::pair<int, std::vector<float>>& b) {
            float avg_a = 0;
            if (!a.second.empty()) {
              for (float d : a.second) {
                avg_a += d;
              }
              avg_a /= a.second.size();
            }

            float avg_b = 0;
            if (!b.second.empty()) {
              for (float d : b.second) {
                avg_b += d;
              }
              avg_b /= b.second.size();
            }

            return avg_a < avg_b;
          });


          Serial.println("Key Sort");
          for (const auto& key : keys) 
          {
            Serial.print("Key: ");
            Serial.print(key.first + 1);
            Serial.print(", Average Value: ");
            
            float avg = 0;
            if (!key.second.empty()) {
              for (float d : key.second) {
                avg += d;
              }
              avg /= key.second.size();
            }
            Serial.println(avg);
          }

          // Move the first 5 elements to the end
          Serial.print("Total Distance: ");
          Serial.println(total_distance_counter);
          std::rotate(keys.begin(), keys.begin() + (12 - total_distance_counter), keys.end());

          Serial.println("Key Rotate");
          for (const auto& key : keys) 
          {
            Serial.print("Key: ");
            Serial.print(key.first + 1);
            Serial.print(", Average Value: ");
            
            float avg = 0;
            if (!key.second.empty()) {
              for (float d : key.second) {
                avg += d;
              }
              avg /= key.second.size();
            }
            Serial.println(avg);
          }

          // Swap elements to allow for auto switching
          s// Save the original 8th element in a temporary variable
          std::pair<int, std::vector<float>> temp = keys[7];

          // Move the 6th element to the 7th position
          keys[7] = keys[5];

          // Move the 7th element to the 9th position
          keys[8] = keys[6];

          // Move the original 8th element to the 6th position
          keys[5] = temp;

          // Move the 9th element to the 8th position
          keys[6] = keys[8];

          Serial.println("Key Swap");
          for (const auto& key : keys) 
          {
            Serial.print("Key: ");
            Serial.print(key.first + 1);
            Serial.print(", Average Value: ");
            
            float avg = 0;
            if (!key.second.empty()) {
              for (float d : key.second) {
                avg += d;
              }
              avg /= key.second.size();
            }
            Serial.println(avg);
          }

          // clears data collected
          averages.clear();
          Serial.println("Average Clear");

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

void setup() 
{
  Serial.begin(115200);

  // WiFi Connection
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to Wifi... ");
  }
  Serial.println("Wifi connected");
  delay(2000);

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

  bool received = true;
  
}

/*************************************
************ PROGRAM LOOP ************
*************************************/

void loop() 
{
  advancedRanging();

  // double tof = 1;

  // while (tof = 0)
  // {
  //   twr_transmitter_mode(102, tof);
  // }

  // bool received = true;

  // while (!received)
  // {
  //   twr_receiver_mode(101, received);
  // }
}


