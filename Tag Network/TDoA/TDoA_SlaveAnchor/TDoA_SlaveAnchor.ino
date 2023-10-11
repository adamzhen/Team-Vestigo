#include <dw3000.h>
#include <SPI.h>
#include "UWBOperations.h"
//#include "ESP-NOWOperations.h"

// Function prototypes
extern SPISettings _fastSPI;

// Global variables
uint8_t anchorId;  // To be set to the Slave Anchor's ID

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

void setup() 
{
  Serial.begin(115200);
  
  // Initialize SPI settings
  _fastSPI = SPISettings(16000000L, MSBFIRST, SPI_MODE0);
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

  receiveSyncSignal();
}

void loop() 
{
  receiveSyncSignal();
  // receiveTagSignal();
}
