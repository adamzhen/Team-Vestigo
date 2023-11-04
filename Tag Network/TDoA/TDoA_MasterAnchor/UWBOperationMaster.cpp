#include <SPI.h>
#include "UWBOperationMaster.h"
#include "SharedVariables.h"

// Time Variables
uint64_t transmissionDelay = 1500 * UUS_TO_DWT_TIME;
uint64_t unitsPerSecond = static_cast<uint64_t>(1.0 / DWT_TIME_UNITS);
double frequencyOffset = 0.0;

// UWB Messages
static uint8_t sync_signal_packet[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'M', 'A', 0xE0, 0, 0, 0, 0, 0, 0, 0};
static uint8_t tx_poll_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'W', 'A', 'V', 'E', 0xE0, 0, 0};
static uint8_t rx_resp_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'V', 'E', 'W', 'A', 0xE1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// UWB Variables
static uint8_t TWR_rx_buffer[20];
static uint32_t statusTWR = 0;

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

// UWB Operation Functions
void sendSyncSignal() 
{
  // Set Transmission Time
  uint32_t masterTime32bit = dwt_readsystimestamphi32();
  uint32_t transmissionTime= (uint32_t)(((((uint64_t)masterTime32bit) << 8) + transmissionDelay) >> 8);
  dwt_setdelayedtrxtime(transmissionTime);

  // Calculate Sync Time
  uint64_t syncedTime = (((uint64_t)(transmissionTime)) << 8) + TX_ANT_DLY;

  // Copy Sync Time to Sync Packet
  memcpy(&sync_signal_packet[SYNC_MSG_TS_IDX], &syncedTime, SYNC_MSG_TS_LEN);

  dwt_writetxdata(sizeof(sync_signal_packet), sync_signal_packet, 0);
  dwt_writetxfctrl(sizeof(sync_signal_packet), 0, 1);
  
  // Start the transmission
  int ret = dwt_starttx(DWT_START_TX_DELAYED);

  // Error Handling
  if (ret == DWT_SUCCESS) {
    while (!(dwt_read32bitreg(SYS_STATUS_ID) & SYS_STATUS_TXFRS_BIT_MASK)) {};
    dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG_BIT_MASK);
  } else {
    Serial.println("Abandoned");
  }
}

uint64_t gatherSlaveToF()
{
  dwt_setrxaftertxdelay(POLL_TX_TO_RESP_RX_DLY_UUS);
  dwt_setrxtimeout(RESP_RX_TIMEOUT_UUS);

  dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS_BIT_MASK);
  dwt_writetxdata(sizeof(tx_poll_msg), tx_poll_msg, 0);
  dwt_writetxfctrl(sizeof(tx_poll_msg), 0, 1);

  dwt_starttx(DWT_START_TX_IMMEDIATE | DWT_RESPONSE_EXPECTED);

  while (!((statusTWR = dwt_read32bitreg(SYS_STATUS_ID)) & (SYS_STATUS_RXFCG_BIT_MASK | SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR))) {};

  if (statusTWR & SYS_STATUS_RXFCG_BIT_MASK)
  {
    uint32_t frame_len;

    dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG_BIT_MASK);

    frame_len = dwt_read32bitreg(RX_FINFO_ID) & RXFLEN_MASK;
    if (frame_len <= sizeof(TWR_rx_buffer))
    {
      dwt_readrxdata(TWR_rx_buffer, frame_len, 0);

      if (memcmp(TWR_rx_buffer, rx_resp_msg, ALL_MSG_COMMON_LEN) == 0)
      {
        // Compute reception and transmission times
        uint32_t poll_tx_ts, resp_rx_ts, poll_rx_ts, resp_tx_ts;
        int32_t rtd_init, rtd_resp;
        double clockOffsetRatio;

        poll_tx_ts = dwt_readtxtimestamplo32();
        resp_rx_ts = dwt_readrxtimestamplo32();

        /* Read carrier integrator value and calculate clock offset ratio. See NOTE 11 below. */
        clockOffsetRatio = ((double)dwt_readclockoffset()) / (uint32_t)(1 << 26);
        Serial.print("ClockOffsetRatio: ");
        Serial.println(clockOffsetRatio, 12);

        /* Get timestamps embedded in response message. */
        resp_msg_get_ts(&TWR_rx_buffer[RESP_MSG_POLL_RX_TS_IDX], &poll_rx_ts);
        resp_msg_get_ts(&TWR_rx_buffer[RESP_MSG_RESP_TX_TS_IDX], &resp_tx_ts);

        rtd_init = resp_rx_ts - poll_tx_ts;
        rtd_resp = resp_tx_ts - poll_rx_ts;

        frequencyOffset = clockOffsetRatio;

        // Return final ToF
        return (rtd_init - rtd_resp * (1 - clockOffsetRatio)) / 2.0;
      }
    }
  }
  else
  {
    // Error Handling
    dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR);
    return 0;
  }
}