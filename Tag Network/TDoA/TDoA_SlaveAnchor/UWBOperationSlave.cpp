#include "UWBOperationSlave.h"
#include <SPI.h>

uint64_t masterTime, slaveTime;
uint64_t timeOffset;
int timeOffsetSign = 1;
uint8_t rx_sync_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'M', 'A', 0xE0, 0, 0, 0, 0, 0, 0, 0, 0};
uint8_t rx_buffer[20];
uint32_t status = 0;

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
  while (!((status = dwt_read32bitreg(SYS_STATUS_ID)) & (SYS_STATUS_RXFCG_BIT_MASK | SYS_STATUS_ALL_RX_ERR))) {};

  if (status & SYS_STATUS_RXFCG_BIT_MASK)
  {
    // Clear the RXFCG event
    dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG_BIT_MASK);

    // Read the received packet to extract master's timestamp
    uint32_t frame_len = dwt_read32bitreg(RX_FINFO_ID) & RXFLEN_MASK;
    if (frame_len <= sizeof(rx_buffer)) 
    {
      dwt_readrxdata(rx_buffer, frame_len, 0);

      if (memcmp(rx_buffer, rx_sync_msg, sizeof(SYNC_MSG_TS_IDX)) == 0) 
      {
        slaveTime = get_rx_timestamp_u64();

        // Extract the master's timestamp and adjust the slave's internal clock
        uint32_t masterTime32bit;
        resp_msg_get_ts(&rx_buffer[SYNC_MSG_TS_IDX], &masterTime32bit);
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

//////////////////// WIP ////////////////////
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
//////////////////// WIP ////////////////////