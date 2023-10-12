#include "UWBOperationMaster.h"
#include <SPI.h>

uint8_t sync_signal_packet[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'M', 'A', 0xE0, 0, 0, 0, 0, 0, 0, 0, 0};
extern uint64_t knownToF;

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
extern SPISettings _fastSPI;

void configUWB()
{
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

  if (dwt_configure(&config) == DWT_ERROR) 
  {
    // Configuration failed
    // Handle the error
  }

  dwt_configuretxrf(&txconfig_options);

  dwt_setrxantennadelay(RX_ANT_DLY);
  dwt_settxantennadelay(TX_ANT_DLY);
  
  // Enable LEDs for debugging
  dwt_setleds(DWT_LEDS_ENABLE | DWT_LEDS_INIT_BLINK);
}

void sendSyncSignal() 
{
  // Prepare the sync signal packet
  Serial.println("Sending Sync");
  uint32_t masterTime32bit = dwt_readsystimestamphi32();
  Serial.print("Master Time: ");
  Serial.println((double)((uint64_t)masterTime32bit << 8) * DWT_TIME_UNITS, 12);
  memcpy(&sync_signal_packet[SYNC_MSG_TS_IDX], &masterTime32bit, SYNC_MSG_TS_LEN);

  // Write data to DW3000's TX buffer
  dwt_writetxdata(sizeof(sync_signal_packet), sync_signal_packet, 0);
  
  // Configure the TX settings and enable immediate transmission
  dwt_writetxfctrl(sizeof(sync_signal_packet), 0, 1);
  
  // Start the transmission
  dwt_starttx(DWT_START_TX_IMMEDIATE);

  Serial.println("Sync Sent");

  // Poll DW IC until the TX frame sent event is set
  while (!(dwt_read32bitreg(SYS_STATUS_ID) & SYS_STATUS_TXFRS_BIT_MASK)) {};

  // Clear the TX frame sent event
  dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG_BIT_MASK);
}

//////////////////// WIP ////////////////////
void gatherSlaveToF()
{
  Serial.println("TWR");
}
//////////////////// WIP ////////////////////