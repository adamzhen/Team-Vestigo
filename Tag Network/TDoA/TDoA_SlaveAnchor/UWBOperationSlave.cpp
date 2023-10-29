#include <SPI.h>
#include "UWBOperationSlave.h"
#include "ESPNOWOperation.h"
#include "SharedVariables.h"

// Time Variables
uint64_t masterTime = 0, slaveTime = 0, tagTime = 0, syncTime = 0, TWRTime = 0, timeOffset = 0;
uint64_t lastReceivedMasterTime = 0, lastReceivedSlaveTime = 0, lastReceivedTagTime = 0, lastReceivedSyncTime = 0, lastReceivedTWRTime = 0; 
uint64_t overflowCounterMaster = 0, overflowCounterSlave = 0, overflowCounterTag = 0, overflowCounterSync = 0, overflowCounterTWR = 0;
uint64_t filteredTimeOffset = 0;
int timeOffsetSign = 1;

// History Variables
std::deque<uint64_t> timeDiffHistory;
std::deque<uint64_t> startupSlaveOffsetTimes;
std::deque<int64_t> startupPhaseOffsets;
const size_t historySize = 20;

// UWB Messages
static uint8_t rx_sync_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'M', 'A', 0xE0, 0, 0, 0, 0, 0, 0, 0};
static uint8_t blink_msg[]   = {0x41, 0x88, 0, 0xCA, 0xDE, 'T', 'G', 0xE0, 0, 0, 0, 0, 0, 0, 0, 0};
static uint8_t rx_poll_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'W', 'A', 'V', 'E', 0xE0, 0, 0};
static uint8_t tx_resp_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'V', 'E', 'W', 'A', 0xE1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// UWB Variables
uint8_t sync_rx_buffer[15], TWR_rx_buffer[20], blink_rx_buffer[16];
uint32_t statusTDoA = 0, statusTWR = 0;
uint32_t frame_len = 0;
static uint64_t poll_rx_ts, resp_tx_ts;
uint64_t unitsPerSecond = static_cast<uint64_t>(1.0 / DWT_TIME_UNITS);

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

// Time Functions
uint64_t getUWBTime(uint64_t time, uint8_t buffer[], uint8_t index) 
{
  for (int i = 7; i >= 0; i--) 
  {
    time <<= 8;
    time |= buffer[index + i];
  }
  return time;
}

uint64_t adjustTo64bitTime(uint64_t curTime, uint64_t lastTime, uint64_t& overflowCounter) 
{
  if (curTime < lastTime) overflowCounter++;
  return (overflowCounter << 40) | curTime;
}

void updateTimeOffsets() 
{
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

  // Store time difference
  timeDiffHistory.push_back(timeOffset * timeOffsetSign);
  if (timeDiffHistory.size() > historySize) 
  {
    timeDiffHistory.pop_front();
  }
}

// UWB Operation Functions
void processSyncSignal()
{
  dwt_readrxdata(sync_rx_buffer, frame_len, 0);

  if (memcmp(sync_rx_buffer, rx_sync_msg, sizeof(SYNC_MSG_TS_IDX)) == 0) 
  {
    // Compute time offset of sync clock and hardware clock
    uint64_t receivedMasterTime = 0, receivedSyncTime = 0;
    receivedSyncTime = get_rx_timestamp_u64() & 0xFFFFFFFFFF;      
    receivedMasterTime = getUWBTime(receivedMasterTime, sync_rx_buffer, SYNC_MSG_TS_IDX) & 0xFFFFFFFFFF;

    masterTime = adjustTo64bitTime(receivedMasterTime, lastReceivedMasterTime, overflowCounterMaster) + TWRData.ToF;
    syncTime = adjustTo64bitTime(receivedSyncTime, lastReceivedSyncTime, overflowCounterSync);
    // updateTimeOffsets(); // Basically Not Used RN

    lastReceivedMasterTime = receivedMasterTime;
    lastReceivedSyncTime = receivedSyncTime;
  }
}

void processTagSignal()
{
  dwt_readrxdata(blink_rx_buffer, frame_len, 0);

  if (memcmp(blink_rx_buffer, blink_msg, sizeof(BLINK_MSG_ID_IDX)) == 0) 
  {
    // Compute time offset of sync clock and hardware clock
    uint64_t receivedTagTime = 0, receivedSlaveTime = 0;
    receivedSlaveTime = get_rx_timestamp_u64() & 0xFFFFFFFFFF;      
    receivedTagTime = getUWBTime(receivedTagTime, blink_rx_buffer, BLINK_MSG_TS_IDX) & 0xFFFFFFFFFF;

    tagTime = adjustTo64bitTime(receivedTagTime, lastReceivedTagTime, overflowCounterTag);
    slaveTime = adjustTo64bitTime(receivedSlaveTime, lastReceivedSlaveTime, overflowCounterSlave);

    lastReceivedTagTime = receivedTagTime;
    lastReceivedSlaveTime = receivedSlaveTime;


    uint64_t deltaTime = slaveTime - syncTime;
    uint64_t estimatedCurrentSlaveTime = static_cast<uint64_t>(static_cast<int64_t>(syncTime) + lastPhaseOffset + static_cast<int64_t>(medianFrequencyOffset * static_cast<double>(deltaTime)));
    uint64_t correctedTimeDifference = tagTime - estimatedCurrentSlaveTime;

    // Debug
    Serial.print("Delta Time: ");
    Serial.println(deltaTime * DWT_TIME_UNITS, 12);
    Serial.print("Estimated Current Slave Time: ");
    Serial.println(estimatedCurrentSlaveTime * DWT_TIME_UNITS, 12);
    Serial.print("Corrected Time Difference: ");
    Serial.println(correctedTimeDifference * DWT_TIME_UNITS, 12);

    // Send time difference to MIO via ESP-NOW
    TDoAData.tag_id = blink_rx_buffer[BLINK_MSG_ID_IDX];
    TDoAData.anchor_id = SLAVE_ID;
    TDoAData.difference = (double) (correctedTimeDifference * DWT_TIME_UNITS);
    sendToPeer(MIOMac, &TDoAData, sizeof(TDoAData));
  }
}

void receiveTDoASignal() 
{
  dwt_rxenable(DWT_START_RX_IMMEDIATE);

  // Loop until sync or tag UWB message received
  while (!((statusTDoA = dwt_read32bitreg(SYS_STATUS_ID)) & (SYS_STATUS_RXFCG_BIT_MASK | SYS_STATUS_ALL_RX_ERR))) {};

  if (statusTDoA & SYS_STATUS_RXFCG_BIT_MASK)
  {
    dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG_BIT_MASK);

    frame_len = dwt_read32bitreg(RX_FINFO_ID) & RXFLEN_MASK;

    // Process UWB Signal
    if (frame_len <= sizeof(sync_rx_buffer)) 
    {
      processSyncSignal();
    }
    else if (frame_len <= sizeof(blink_rx_buffer))
    {
      processTagSignal();
    }
  }
  else
  {
    // Error Handling
    dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_ERR);
  }
}

void sendSlaveToF()
{
  dwt_rxenable(DWT_START_RX_IMMEDIATE);

  // Loop until UWB message received
  while (!((statusTWR = dwt_read32bitreg(SYS_STATUS_ID)) & (SYS_STATUS_RXFCG_BIT_MASK | SYS_STATUS_ALL_RX_ERR))) {};

  if (statusTWR & SYS_STATUS_RXFCG_BIT_MASK)
  {
    uint32_t frame_len;
    dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG_BIT_MASK);

    frame_len = dwt_read32bitreg(RX_FINFO_ID) & RXFLEN_MASK;
    if (frame_len <= sizeof(TWR_rx_buffer))
    {
      dwt_readrxdata(TWR_rx_buffer, frame_len, 0);

      if (memcmp(TWR_rx_buffer, rx_poll_msg, ALL_MSG_COMMON_LEN) == 0)
      {
        // Compute reception and transmission times
        uint32_t resp_tx_time;
        int ret;
        poll_rx_ts = get_rx_timestamp_u64();
        resp_tx_time = (poll_rx_ts + (POLL_RX_TO_RESP_TX_DLY_UUS * UUS_TO_DWT_TIME)) >> 8;
        dwt_setdelayedtrxtime(resp_tx_time);

        resp_tx_ts = (((uint64_t)(resp_tx_time & 0xFFFFFFFEUL)) << 8) + TX_ANT_DLY;

        resp_msg_set_ts(&tx_resp_msg[RESP_MSG_POLL_RX_TS_IDX], poll_rx_ts);
        resp_msg_set_ts(&tx_resp_msg[RESP_MSG_RESP_TX_TS_IDX], resp_tx_ts);

        // Transmit response message
        dwt_writetxdata(sizeof(tx_resp_msg), tx_resp_msg, 0);
        dwt_writetxfctrl(sizeof(tx_resp_msg), 0, 1);
        ret = dwt_starttx(DWT_START_TX_DELAYED);

        // Current TWR Time
        uint64_t receivedTWRTime = 0;
        receivedTWRTime = dwt_readsystimestamphi32();
        TWRTime = adjustTo64bitTime(receivedTWRTime, lastReceivedTWRTime, overflowCounterTWR);
        lastReceivedTWRTime = receivedTWRTime;        

        // Add phase and frequency offset data to history
        int64_t phaseOffset = static_cast<int64_t> (TWRTime - poll_rx_ts);
        startupPhaseOffsets.push_back(phaseOffset);
        startupSlaveOffsetTimes.push_back(poll_rx_ts);
        
        // Limit the size of the offset history to avoid excessive memory usage
        if (startupPhaseOffsets.size() > 100) {
          startupPhaseOffsets.pop_front();
          startupSlaveOffsetTimes.pop_front();
        }

        // Error Handling
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