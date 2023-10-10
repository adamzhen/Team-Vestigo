#include "dw3000.h"
#include "SPI.h"

extern SPISettings _fastSPI;

#define PIN_RST 27
#define PIN_IRQ 34
#define PIN_SS 4

#define MASTER_ANCHOR_ID 0
#define MASTER_CHANNEL 9

// Function prototypes
void sendSyncSignal();
void setup();

// Global variables
uint8_t anchorId = MASTER_ANCHOR_ID;
uint64_t known_ToF;

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
  
  // Set channel to 9 for Master Anchor
  dwt_setchannel(MASTER_CHANNEL);
  
  // Enable LEDs for debugging
  dwt_setleds(DWT_LEDS_ENABLE | DWT_LEDS_INIT_BLINK);
}

void loop() {
  sendSyncSignal();
  // Sleep or perform other tasks
}

void sendSyncSignal() {
  // Prepare the sync signal packet
  uint8_t sync_signal_packet[] = {/* packet data */};

  // Write data to DW3000's TX buffer
  dwt_writetxdata(sizeof(sync_signal_packet), sync_signal_packet, 0);
  
  // Configure the TX settings and enable immediate transmission
  dwt_writetxfctrl(sizeof(sync_signal_packet), 0, 1);
  
  // Start the transmission
  dwt_starttx(DWT_START_TX_IMMEDIATE);

  // Poll DW IC until the TX frame sent event is set
  while (!(dwt_read32bitreg(DW_SYS_STATUS_ID) & DWT_BIT_MASK(SYS_STATUS_TXFRS_BIT))) {};

  // Clear the TX frame sent event
  dwt_write32bitreg(DW_SYS_STATUS_ID, DWT_BIT_MASK(SYS_STATUS_TXFRS_BIT));
}


