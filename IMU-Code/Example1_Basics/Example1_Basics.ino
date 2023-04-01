/****************************************************************
 * Example1_Basics.ino
 * ICM 20948 Arduino Library Demo
 * Use the default configuration to stream 9-axis IMU data
 * Owen Lyke @ SparkFun Electronics
 * Original Creation Date: April 17 2019
 *
 * Please see License.md for the license information.
 *
 * Distributed as-is; no warranty is given.
 ***************************************************************/
#include "ICM_20948.h" // Click here to get the library: http://librarymanager/All#SparkFun_ICM_20948_IMU

//#define USE_SPI       // Uncomment this to use SPI

#define SERIAL_PORT Serial

#define SPI_PORT SPI // Your desired SPI port.       Used only when "USE_SPI" is defined
#define CS_PIN 2     // Which pin you connect CS to. Used only when "USE_SPI" is defined

#define WIRE_PORT Wire // Your desired Wire port.      Used when "USE_SPI" is not defined
// The value of the last bit of the I2C address.
// On the SparkFun 9DoF IMU breakout the default is 1, and when the ADR jumper is closed the value becomes 0
#define AD0_VAL 1

#ifdef USE_SPI
ICM_20948_SPI myICM; // If using SPI create an ICM_20948_SPI object
#else
ICM_20948_I2C myICM; // Otherwise create an ICM_20948_I2C object
#endif

void setup()
{

  SERIAL_PORT.begin(9600);
  while (!SERIAL_PORT)
  {
  };

#ifdef USE_SPI
  SPI_PORT.begin();
#else
  WIRE_PORT.begin();
  WIRE_PORT.setClock(400000);
#endif

  //myICM.enableDebugging(); // Uncomment this line to enable helpful debug messages on Serial

  bool initialized = false;
  while (!initialized)
  {

#ifdef USE_SPI
    myICM.begin(CS_PIN, SPI_PORT);
#else
    myICM.begin(WIRE_PORT, AD0_VAL);
#endif

    SERIAL_PORT.print(F("Initialization of the sensor returned: "));
    SERIAL_PORT.println(myICM.statusString());
    if (myICM.status != ICM_20948_Stat_Ok)
    {
      SERIAL_PORT.println("Trying again...");
      delay(500);
    }
    else
    {
      initialized = true;
    }
  }
    bool success = true;
    int32_t biasAccelX = 0;
    int32_t biasAccelY = 0;
    int32_t biasAccelZ = 500;
    success &= (myICM.setBiasAccelX(biasAccelX) == ICM_20948_Stat_Ok);
    success &= (myICM.setBiasAccelY(biasAccelY) == ICM_20948_Stat_Ok);
    success &= (myICM.setBiasAccelZ(biasAccelZ) == ICM_20948_Stat_Ok);
}

void loop()
{

  if (myICM.dataReady())
  {
    myICM.getAGMT();         // The values are only updated when you call 'getAGMT'
                             //    printRawAGMT( myICM.agmt );     // Uncomment this to see the raw values, taken directly from the agmt structure
    printScaledAGMT(&myICM, false); // This function takes into account the scale settings from when the measurement was made to calculate the values with units
    delay(0.01);
  }
  else
  {
    SERIAL_PORT.println("Waiting for data");
    delay(100);
  }
}

// Below here are some helper functions to print the data nicely!

void printPaddedInt16b(int16_t val)
{
  if (val > 0)
  {
    SERIAL_PORT.print(" ");
    if (val < 10000)
    {
      SERIAL_PORT.print("0");
    }
    if (val < 1000)
    {
      SERIAL_PORT.print("0");
    }
    if (val < 100)
    {
      SERIAL_PORT.print("0");
    }
    if (val < 10)
    {
      SERIAL_PORT.print("0");
    }
  }
  else
  {
    SERIAL_PORT.print("-");
    if (abs(val) < 10000)
    {
      SERIAL_PORT.print("0");
    }
    if (abs(val) < 1000)
    {
      SERIAL_PORT.print("0");
    }
    if (abs(val) < 100)
    {
      SERIAL_PORT.print("0");
    }
    if (abs(val) < 10)
    {
      SERIAL_PORT.print("0");
    }
  }
  SERIAL_PORT.print(abs(val));
}

void printRawAGMT(ICM_20948_AGMT_t agmt)
{
  SERIAL_PORT.print("RAW. Acc [ ");
  printPaddedInt16b(agmt.acc.axes.x);
  SERIAL_PORT.print(", ");
  printPaddedInt16b(agmt.acc.axes.y);
  SERIAL_PORT.print(", ");
  printPaddedInt16b(agmt.acc.axes.z);
  SERIAL_PORT.print(" ], Gyr [ ");
  printPaddedInt16b(agmt.gyr.axes.x);
  SERIAL_PORT.print(", ");
  printPaddedInt16b(agmt.gyr.axes.y);
  SERIAL_PORT.print(", ");
  printPaddedInt16b(agmt.gyr.axes.z);
  SERIAL_PORT.print(" ], Mag [ ");
  printPaddedInt16b(agmt.mag.axes.x);
  SERIAL_PORT.print(", ");
  printPaddedInt16b(agmt.mag.axes.y);
  SERIAL_PORT.print(", ");
  printPaddedInt16b(agmt.mag.axes.z);
  SERIAL_PORT.print(" ], Tmp [ ");
  printPaddedInt16b(agmt.tmp.val);
  SERIAL_PORT.print(" ]");
  SERIAL_PORT.println();
}

void printFormattedFloat(float val, uint8_t leading, uint8_t decimals)
{
  float aval = abs(val);
  if (val < 0)
  {
    SERIAL_PORT.print("-");
  }
  else
  {
    SERIAL_PORT.print(" ");
  }
  for (uint8_t indi = 0; indi < leading; indi++)
  {
    uint32_t tenpow = 0;
    if (indi < (leading - 1))
    {
      tenpow = 1;
    }
    for (uint8_t c = 0; c < (leading - 1 - indi); c++)
    {
      tenpow *= 10;
    }
    if (aval < tenpow)
    {
      SERIAL_PORT.print("0");
    }
    else
    {
      break;
    }
  }
  if (val < 0)
  {
    SERIAL_PORT.print(-val, decimals);
  }
  else
  {
    SERIAL_PORT.print(val, decimals);
  }
}

#ifdef USE_SPI
void printScaledAGMT(ICM_20948_SPI *sensor, bool formatted)
{
#else
void printScaledAGMT(ICM_20948_I2C *sensor, bool formatted)
{
#endif
  char sep = ',';
  if (formatted){
    //SERIAL_PORT.print(F("Scaled. Acc (mg): "));
    SERIAL_PORT.print(F(" Ax:"));
    SERIAL_PORT.print(sensor->accX()); //printFormattedFloat(sensor->accX(), 5, 2);
    SERIAL_PORT.printf("%c Ay:", sep);
    SERIAL_PORT.print(sensor->accY()); //printFormattedFloat(sensor->accY(), 5, 2);
    SERIAL_PORT.printf("%c Az:", sep);
    SERIAL_PORT.print(sensor->accZ()); //printFormattedFloat(sensor->accZ(), 5, 2);
    //SERIAL_PORT.print(", Gyr (DPS): Gx: ");
    SERIAL_PORT.printf("%c Gx:", sep);
    SERIAL_PORT.print(sensor->gyrX()); //printFormattedFloat(sensor->gyrX(), 5, 2);
    SERIAL_PORT.printf("%c Gy:", sep);
    SERIAL_PORT.print(sensor->gyrY()); //printFormattedFloat(sensor->gyrY(), 5, 2);
    SERIAL_PORT.printf("%c Gz:", sep);
    SERIAL_PORT.print(sensor->gyrZ()); //printFormattedFloat(sensor->gyrZ(), 5, 2);
    //SERIAL_PORT.print(", Mag (uT): Mx: ");
    SERIAL_PORT.printf("%c Mx:", sep);
    SERIAL_PORT.print(sensor->magX()); //printFormattedFloat(sensor->magX(), 5, 2);
    SERIAL_PORT.printf("%c My:", sep);
    SERIAL_PORT.print(sensor->magY()); //printFormattedFloat(sensor->magY(), 5, 2);
    SERIAL_PORT.printf("%c Mz:", sep);
    SERIAL_PORT.print(sensor->magZ()); //printFormattedFloat(sensor->magZ(), 5, 2);
    // SERIAL_PORT.print(" ], Tmp (C) [ ");
    // printFormattedFloat(sensor->temp(), 5, 2);
    SERIAL_PORT.println();
  } else {
    //SERIAL_PORT.print(F("Scaled. Acc (mg): "));
    SERIAL_PORT.print(F(""));
    SERIAL_PORT.print(sensor->accX()); //printFormattedFloat(sensor->accX(), 5, 2);
    SERIAL_PORT.printf("%c ", sep);
    SERIAL_PORT.print(sensor->accY()); //printFormattedFloat(sensor->accY(), 5, 2);
    SERIAL_PORT.printf("%c ", sep);
    SERIAL_PORT.print(sensor->accZ()); //printFormattedFloat(sensor->accZ(), 5, 2);
    //SERIAL_PORT.print(", Gyr (DPS): Gx: ");
    SERIAL_PORT.printf("%c ", sep);
    SERIAL_PORT.print(sensor->gyrX()); //printFormattedFloat(sensor->gyrX(), 5, 2);
    SERIAL_PORT.printf("%c ", sep);
    SERIAL_PORT.print(sensor->gyrY()); //printFormattedFloat(sensor->gyrY(), 5, 2);
    SERIAL_PORT.printf("%c ", sep);
    SERIAL_PORT.print(sensor->gyrZ()); //printFormattedFloat(sensor->gyrZ(), 5, 2);
    //SERIAL_PORT.print(", Mag (uT): Mx: ");
    SERIAL_PORT.printf("%c ", sep);
    SERIAL_PORT.print(sensor->magX()); //printFormattedFloat(sensor->magX(), 5, 2);
    SERIAL_PORT.printf("%c ", sep);
    SERIAL_PORT.print(sensor->magY()); //printFormattedFloat(sensor->magY(), 5, 2);
    SERIAL_PORT.printf("%c ", sep);
    SERIAL_PORT.print(sensor->magZ()); //printFormattedFloat(sensor->magZ(), 5, 2);
    // SERIAL_PORT.print(" ], Tmp (C) [ ");
    // printFormattedFloat(sensor->temp(), 5, 2);
    SERIAL_PORT.println();
  }
}
