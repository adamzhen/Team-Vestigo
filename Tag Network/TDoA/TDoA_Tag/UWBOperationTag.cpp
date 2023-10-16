#include <SPI.h>
#include "UWBOperationTag.h"

// UWB Messages
static uint8_t blink[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'T', 'G', 0xE0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// UWB Configs
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
  _fastSPI = SPISettings(16000000L, MSBFIRST, SPI_MODE0);
  spiBegin(PIN_IRQ, PIN_RST);
  spiSelect(PIN_SS);
  
  if (dwt_initialise(DWT_DW_INIT) == DWT_ERROR || dwt_configure(&config) == DWT_ERROR) return;
  dwt_configuretxrf(&txconfig_options);
  dwt_setrxantennadelay(RX_ANT_DLY);
  dwt_settxantennadelay(TX_ANT_DLY);
  
  // Enable LEDs for debugging
  dwt_setleds(DWT_LEDS_ENABLE | DWT_LEDS_INIT_BLINK);
}

void sendBlink()
{
  uint32_t tagTime32bit = dwt_readsystimestamphi32();
  uint64_t tagTime64bit = (((uint64_t)(tagTime32bit)) << 8) + TX_ANT_DLY;

  // Copy Sync Time to Sync Packet
  memcpy(&blink[SYNC_MSG_TS_IDX], &tagTime64bit, SYNC_MSG_TS_LEN);

  dwt_writetxdata(sizeof(blink), blink, 0);
  dwt_writetxfctrl(sizeof(blink), 0, 1);
  dwt_starttx(DWT_START_TX_IMMEDIATE);

  while (!(dwt_read32bitreg(SYS_STATUS_ID) & SYS_STATUS_TXFRS_BIT_MASK)) {};

  dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG_BIT_MASK);
}