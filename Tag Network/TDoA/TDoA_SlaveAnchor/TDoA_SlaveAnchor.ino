#include "dw3000.h"
#include "SPI.h"

extern SPISettings _fastSPI;

#define PIN_RST 27
#define PIN_IRQ 34
#define PIN_SS 4

#define MASTER_ANCHOR_ID 0
#define MASTER_CHANNEL 9
#define TAG_CHANNEL 5

#define SYNC_MSG_TS_IDX 10
#define SYNC_MSG_TS_LEN 8

#define TX_ANT_DLY 16385
#define RX_ANT_DLY 16385

// Function prototypes
void receiveSyncSignal();
void receiveTagSignal();
void adjustClockWithMasterTime(uint64_t master_time, uint64_t slave_time);
void setup();

// Global variables
uint8_t anchorId;  // To be set to the Slave Anchor's ID
uint64_t slaveCurrentTime, t_tag_signal;
float timeOffset;

// Initialize the DW3000 configuration
dwt_config_t config = 
{
  MASTER_CHANNEL,
  DWT_PLEN_128,
  DWT_PAC8,
  9,
  9,
  1,
  DWT_BR_6M8,
  DWT_PHRMODE_STD,
  DWT_PHRRATE_STD,
  (129 + 8 - 8),
  DWT_STS_MODE_OFF,
  DWT_STS_LEN_64,
  DWT_PDOA_M0
};

extern dwt_txconfig_t txconfig_options;

uint8_t rx_sync_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'M', 'A', 0xE0, 0, 0, 0, 0, 0, 0, 0, 0};
uint8_t rx_buffer[20];

void setup() 
{
  Serial.begin(115200);
  // Initialize SPI settings
  spiBegin(PIN_IRQ, PIN_RST);
  spiSelect(PIN_SS);

  // Initialize the DW3000
  if (dwt_initialise(DWT_DW_INIT) == DWT_ERROR) 
  {
    // Initialization failed
    // Handle the error
  }

  // Configure the DW3000
  if (dwt_configure(&config) == DWT_ERROR) 
  {
    // Configuration failed
    // Handle the error
  }

  dwt_setrxantennadelay(RX_ANT_DLY);
  dwt_settxantennadelay(TX_ANT_DLY);

  // Enable LEDs for debugging
  dwt_setleds(DWT_LEDS_ENABLE | DWT_LEDS_INIT_BLINK);
  Serial.println("Setup Complete");
}

void loop() 
{
  Serial.println("Looping");
  receiveSyncSignal();
  // adjustClockWithMasterTime(master_time, uint64slave_time);
  receiveTagSignal();
}

// Rest of the functions
void receiveSyncSignal() 
{
  Serial.println("Enable Receive");
  dwt_write32bitreg(SYS_STATUS_ID, 0xFFFFFFFF);

  // Enable receiver with immediate start
  dwt_rxenable(DWT_START_RX_IMMEDIATE);

  // Poll for reception of a frame or error/timeout
  uint32_t status;
  while (!((status = dwt_read32bitreg(SYS_STATUS_ID)) & (SYS_STATUS_RXFCG_BIT_MASK | SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR))) {};

  Serial.println("Received");
  Serial.print("Status: ");
  Serial.println(status, HEX);
  if (status & SYS_STATUS_RXFCG_BIT_MASK)
  {
    Serial.println("Pass status check");
    // Clear the RXFCG event
    dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG_BIT_MASK);

    Serial.println("Event Clear");

    uint64_t slaveCurrentTime = dwt_readsystimestamphi32();

    Serial.println("Current Time Read");

    // Read the received packet to extract master's timestamp
    uint32_t frame_len = dwt_read32bitreg(RX_FINFO_ID) & RXFLEN_MASK;
    Serial.print("Frame Length: ");
    Serial.println(frame_len);
    Serial.print("Buffer Size: ");
    Serial.println(sizeof(rx_buffer));
    if (frame_len <= sizeof(rx_buffer)) 
    {
      Serial.println("Frame Length Good");
      dwt_readrxdata(rx_buffer, frame_len, 0);
      
      // Validate the received packet
      Serial.print("Received Buffer: ");
      for (int i = 0; i < frame_len; i++) {
        Serial.print(rx_buffer[i], HEX);
        Serial.print(" ");
      }
      Serial.println();
      if (memcmp(rx_buffer, rx_sync_msg, sizeof(SYNC_MSG_TS_IDX)) == 0) 
      {
        Serial.println("Packet Validated");
        // Extract the master's timestamp and adjust the slave's internal clock
        uint64_t master_time;
        memcpy(&master_time, &rx_buffer[SYNC_MSG_TS_IDX], SYNC_MSG_TS_LEN);
        adjustClockWithMasterTime(master_time, slaveCurrentTime);
        double master_time_in_seconds = (double)master_time / (1 / 15.65e-12);
        double slave_time_in_seconds = (double)slaveCurrentTime / (1 / 15.65e-12);

        Serial.print("Master Time Received: ");
        Serial.println(master_time_in_seconds, 12);  // 9 decimal places for more precision
        Serial.print("Slave Time Received: ");
        Serial.println(slave_time_in_seconds, 12);  // 9 decimal places for more precision
      }
    }
  }
  // Handle error cases (not shown)

  dwt_write32bitreg(SYS_STATUS_ID, 0xFFFFFFFF);
}


void receiveTagSignal() 
{
  // Activate reception on channel 5
  // dwt_setchannel(TAG_CHANNEL); work on this later
  dwt_rxenable(DWT_START_RX_IMMEDIATE);

  // Poll for reception of tag signal
  uint32_t status;
  while (!((status = dwt_read32bitreg(SYS_STATUS_ID)) & (SYS_STATUS_RXFCG_BIT_MASK | SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR))) {};

  if (status & SYS_STATUS_RXFCG_BIT_MASK) 
  {
    // Capture the time of tag signal receipt
    // t_tag_signal = dwt_readsystime(); // Fix later
  }
}

void adjustClockWithMasterTime(uint64_t master_time, uint64_t slave_time) 
{
  Serial.println("adjustClock");
  // Implement your Kalman filter or other clock adjustment logic here
  // ...
}

