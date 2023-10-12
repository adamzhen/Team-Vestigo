#include "UWBOperationSlave.h"
#include <SPI.h>

uint64_t masterTime, slaveTime, timeOffset;
int timeOffsetSign = 1;

uint8_t rx_sync_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'M', 'A', 0xE0, 0, 0, 0, 0, 0, 0, 0, 0};
static uint8_t rx_poll_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'W', 'A', 'V', 'E', 0xE0, 0, 0};
static uint8_t tx_resp_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'V', 'E', 'W', 'A', 0xE1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

uint8_t TDoA_rx_buffer[20], TWR_rx_buffer[20];
uint32_t statusTDoA = 0, statusTWR = 0;
static uint64_t poll_rx_ts, resp_tx_ts;

uint64_t unitsPerSecond = static_cast<uint64_t>(1.0 / DWT_TIME_UNITS);

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

void receiveSyncSignal() 
{
  dwt_rxenable(DWT_START_RX_IMMEDIATE);

  // Poll for reception of a frame or error/timeout
  while (!((statusTDoA = dwt_read32bitreg(SYS_STATUS_ID)) & (SYS_STATUS_RXFCG_BIT_MASK | SYS_STATUS_ALL_RX_ERR))) {};

  if (statusTDoA & SYS_STATUS_RXFCG_BIT_MASK)
  {
    // Clear the RXFCG event
    dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG_BIT_MASK);

    // Read the received packet to extract master's timestamp
    uint32_t frame_len = dwt_read32bitreg(RX_FINFO_ID) & RXFLEN_MASK;
    if (frame_len <= sizeof(TDoA_rx_buffer)) 
    {
      dwt_readrxdata(TDoA_rx_buffer, frame_len, 0);

      if (memcmp(TDoA_rx_buffer, rx_sync_msg, sizeof(SYNC_MSG_TS_IDX)) == 0) 
      {
        slaveTime = get_rx_timestamp_u64();

        // Extract the master's timestamp and adjust the slave's internal clock
        uint32_t masterTime32bit;
        resp_msg_get_ts(&TDoA_rx_buffer[SYNC_MSG_TS_IDX], &masterTime32bit);
        masterTime = (uint64_t)masterTime32bit << 8;

        uint64_t fracMasterTime = masterTime % unitsPerSecond;
        uint64_t fracSlaveTime = slaveTime % unitsPerSecond;

        // Calculate the absolute time difference
        uint64_t absTimeDiff = (fracMasterTime >= fracSlaveTime) ? (fracMasterTime - fracSlaveTime) : (fracSlaveTime - fracMasterTime);

        // Check for wrap-around
        if (absTimeDiff > unitsPerSecond / 2) 
        {
            timeOffset = unitsPerSecond - absTimeDiff;
            timeOffsetSign = (fracMasterTime >= fracSlaveTime) ? -1 : 1;
        } else {
            timeOffset = absTimeDiff;
            timeOffsetSign = (fracMasterTime >= fracSlaveTime) ? 1 : -1;
        }

        Serial.print("Master Time Received: ");
        Serial.println((double)masterTime * DWT_TIME_UNITS, 12);
        Serial.print("Time Offset: ");
        Serial.println((double)timeOffset * timeOffsetSign * DWT_TIME_UNITS, 12);
      }
    }
  }
  else
  {
    dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_ERR);
  }
}

void sendSlaveToF()
{
  /* Activate reception immediately. */
  dwt_rxenable(DWT_START_RX_IMMEDIATE);

  /* Poll for reception of a frame or error/timeout. See NOTE 6 below. */
  while (!((statusTWR = dwt_read32bitreg(SYS_STATUS_ID)) & (SYS_STATUS_RXFCG_BIT_MASK | SYS_STATUS_ALL_RX_ERR)))
  {
  };

  if (statusTWR & SYS_STATUS_RXFCG_BIT_MASK)
  {
    uint32_t frame_len;

    /* Clear good RX frame event in the DW IC status register. */
    dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG_BIT_MASK);

    /* A frame has been received, read it into the local buffer. */
    frame_len = dwt_read32bitreg(RX_FINFO_ID) & RXFLEN_MASK;
    if (frame_len <= sizeof(TWR_rx_buffer))
    {
      dwt_readrxdata(TWR_rx_buffer, frame_len, 0);

      if (memcmp(TWR_rx_buffer, rx_poll_msg, ALL_MSG_COMMON_LEN) == 0)
      {
        uint32_t resp_tx_time;
        int ret;

        /* Retrieve poll reception timestamp. */
        poll_rx_ts = get_rx_timestamp_u64();

        /* Compute response message transmission time. See NOTE 7 below. */
        resp_tx_time = (poll_rx_ts + (POLL_RX_TO_RESP_TX_DLY_UUS * UUS_TO_DWT_TIME)) >> 8;
        dwt_setdelayedtrxtime(resp_tx_time);

        /* Response TX timestamp is the transmission time we programmed plus the antenna delay. */
        resp_tx_ts = (((uint64_t)(resp_tx_time & 0xFFFFFFFEUL)) << 8) + TX_ANT_DLY;

        resp_msg_set_ts(&tx_resp_msg[RESP_MSG_POLL_RX_TS_IDX], poll_rx_ts);
        resp_msg_set_ts(&tx_resp_msg[RESP_MSG_RESP_TX_TS_IDX], resp_tx_ts);

        dwt_writetxdata(sizeof(tx_resp_msg), tx_resp_msg, 0); /* Zero offset in TX buffer. */
        dwt_writetxfctrl(sizeof(tx_resp_msg), 0, 1);          /* Zero offset in TX buffer, ranging. */
        ret = dwt_starttx(DWT_START_TX_DELAYED);

        if (ret == DWT_SUCCESS)
        {
          while (!(dwt_read32bitreg(SYS_STATUS_ID) & SYS_STATUS_TXFRS_BIT_MASK)){};

          dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS_BIT_MASK);
        }
      }
    }
  }
  else
  {
    dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_ERR);
  }
}

//////////////////// WIP ////////////////////
// void receiveTagSignal() 
// {
//   // Activate reception on channel 5
//   // dwt_setchannel(TAG_CHANNEL); work on this later
//   dwt_rxenable(DWT_START_RX_IMMEDIATE);

//   // Poll for reception of tag signal
//   uint32_t status;
//   while (!((status = dwt_read32bitreg(SYS_STATUS_ID)) & (SYS_STATUS_RXFCG_BIT_MASK | SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR))) {};

//   if (status & SYS_STATUS_RXFCG_BIT_MASK) 
//   {
//     // Capture the time of tag signal receipt
//     // t_tag_signal = dwt_readsystime(); // Fix later
//   }
// }
//////////////////// WIP ////////////////////