#include "dw3000.h"
#include "SPI.h"

extern SPISettings _fastSPI;

#define PIN_RST 27
#define PIN_IRQ 34
#define PIN_SS 4

#define MASTER_ANCHOR_ID 0
#define MASTER_CHANNEL 9
#define TAG_CHANNEL 5

// Function prototypes
void receiveSyncSignal();
void receiveTagSignal();
void adjustClockWithKalman();
void setup();

// Global variables
uint8_t anchorId;  // To be set to the Slave Anchor's ID
uint64_t t_master_sync, t_tag_signal;
float timeOffset;

// Initialize the DW3000 configuration
dwt_config_t config = {
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

void setup() {
  // Initialize SPI settings
  spiBegin(PIN_IRQ, PIN_RST);
  spiSelect(PIN_SS);

  // Initialize the DW3000
  if (dwt_initialise(DWT_DW_INIT) == DWT_ERROR) {
    // Initialization failed
    // Handle the error
  }

  // Configure the DW3000
  if (dwt_configure(&config) == DWT_ERROR) {
    // Configuration failed
    // Handle the error
  }

  // Enable LEDs for debugging
  dwt_setleds(DWT_LEDS_ENABLE | DWT_LEDS_INIT_BLINK);
}

void loop() {
  receiveSyncSignal();
  adjustClockWithKalman();
  receiveTagSignal();
}

// Rest of the functions
void receiveSyncSignal() {
  // Enable receiver with immediate start
  dwt_rxenable(DWT_START_RX_IMMEDIATE);

  // Poll for reception of a frame or error/timeout
  uint32_t status;
  while (!((status = dwt_read32bitreg(DW_SYS_STATUS_ID)) & (DWT_BIT_MASK(SYS_STATUS_RXFCG_BIT) | DWT_BIT_MASK(SYS_STATUS_ALL_RX_ERR)))) {};

  if (status & DWT_BIT_MASK(SYS_STATUS_RXFCG_BIT)) {
    // Clear the RXFCG event
    dwt_write32bitreg(DW_SYS_STATUS_ID, DWT_BIT_MASK(SYS_STATUS_RXFCG_BIT));

    // Capture the time of sync signal receipt
    t_master_sync = dwt_readsystime();
  }
  // Handle error cases (not shown)
}


void receiveTagSignal() {
  // Activate reception on channel 5
  dwt_setchannel(TAG_CHANNEL);
  dwt_rxenable(DWT_START_RX_IMMEDIATE);

  // Poll for reception of tag signal
  uint32_t status;
  while (!((status = dwt_read32bitreg(SYS_STATUS_ID)) & (SYS_STATUS_RXFCG_BIT_MASK | SYS_STATUS_ALL_RX_ERR))) {};

  if (status & SYS_STATUS_RXFCG_BIT_MASK) {
    // Capture the time of tag signal receipt
    t_tag_signal = dwt_readsystime();
  }
}

void adjustClockWithKalman() {
  // Placeholder for Kalman filter logic
  // Use t_master_sync, t_tag_signal, and other variables to adjust the clock
}
