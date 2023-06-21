//#include <Kalman.h>
#include "Kalman.h"
//#include <BasicLinearAlgebra.h>
using namespace BLA;
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
double dataRate = 100000.0;
double delayMS = 1000/dataRate;
double delta_t = delayMS/1000.0;
double xPos = 0;
double yPos = 0;
double zPos = 0;
double xVel = 0;
double yVel = 0;
double zVel = 0;

void FindVel(double xacc, double yacc, double zacc, double& xVel, double& yVel, double& zVel,double t) {
    xVel += (xacc+0)*t/1000.0;
    yVel += (yacc+0)*t/1000.0;
    zVel += 0*zacc*t/1000.0;
}
void FindPos(double xVel, double yVel, double zVel, double& xPos, double& yPos, double& zPos, double t) {
    xPos += xVel*t;
    yPos += yVel*t;
    zPos += 0*zVel*t;
}
//------------------------------------
/****       KALMAN PARAMETERS    ****/
//------------------------------------

// Dimensions of the matrices
#define Nstate 3 // length of the state vector
#define Nobs 3   // length of the measurement vector

// measurement std (to be characterized from your sensors)
#define n1 0.1 // noise on the 1st measurement component
#define n2 0.1 // noise on the 2nd measurement component 
#define n3 0.1 // noise on the 2nd measurement component 

// model std (~1/inertia). Freedom you give to relieve your evolution equation
#define m1 0.01
#define m2 0.02
#define m3 0.02

KALMAN<Nstate,Nobs> K; // your Kalman filter
BLA::Matrix<Nobs> obs; // observation vector

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
  // example of evolution matrix. Size is <Nstate,Nstate>
  K.F = {1.0, delta_t, 0.5 * delta_t * delta_t,
         0.0, 1.0, delta_t,
         0.0, 0.0, 1.0};
  // example of measurement matrix. Size is <Nobs,Nstate>
  K.H = {1.0, 0.0,
         0.0, 1.0};
  // example of measurement covariance matrix. Size is <Nobs,Nobs>
  K.R = {n1*n1,   0.0, 0.0,
           0.0, n2*n2, 0.0,
           0.0, 0.0, n3*n3};
  // example of model covariance matrix. Size is <Nstate,Nstate>
  K.Q = {m1*m1,   0.0, 0.0,
           0.0, m2*m2, 0.0,
           0.0, 0.0, m3*m3};

    // Initialize the process noise covariance matrix

  SERIAL_PORT.begin(115200);
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
}

void loop()
{

  if (myICM.dataReady())
  {
    myICM.getAGMT();         // The values are only updated when you call 'getAGMT'
                             //    printRawAGMT( myICM.agmt );     // Uncomment this to see the raw values, taken directly from the agmt structure
    printScaledAGMT(&myICM); // This function takes into account the scale settings from when the measurement was made to calculate the values with units
    
    
    delay(delayMS); // originally 30.0
  }
  else
  {
    SERIAL_PORT.println("Waiting for data");
    delay(500);
  }
  

}

// Below here are some helper functions to print the data nicely!


//dude.VelocityUpdate(sensor->accX,sensor->accY,sensor->accZ);
//dude.PositionUpdate(dude.xVel, dude.yVel, dude.zVel);


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
void printScaledAGMT(ICM_20948_SPI *sensor)

#else
void printScaledAGMT(ICM_20948_I2C *sensor)
//Position Readings
{
#endif
  // SERIAL_PORT.print("Scaled. Acc (mg) [ ");
  // printFormattedFloat(sensor->accX(), 5, 2);
  // SERIAL_PORT.print(", ");
  // printFormattedFloat(sensor->accY(), 5, 2);
  // SERIAL_PORT.print(", ");
  // printFormattedFloat(sensor->accZ(), 5, 2);
  // SERIAL_PORT.print(" ], Gyr (DPS) [ ");
  // printFormattedFloat(sensor->gyrX(), 5, 2);
  // SERIAL_PORT.print(", ");
  // printFormattedFloat(sensor->gyrY(), 5, 2);
  // SERIAL_PORT.print(", ");
  // printFormattedFloat(sensor->gyrZ(), 5, 2);
  // SERIAL_PORT.print(" ], Mag (uT) [ ");
  // printFormattedFloat(sensor->magX(), 5, 2);
  // SERIAL_PORT.print(", ");
  // printFormattedFloat(sensor->magY(), 5, 2);
  // SERIAL_PORT.print(", ");
  // printFormattedFloat(sensor->magZ(), 5, 2);
  // //SERIAL_PORT.print(" ], Tmp (C) [ ");
  // //printFormattedFloat(sensor->temp(), 5, 2);
  // SERIAL_PORT.print(" ], Pos (cm) [ ");

  //velocty values and acceliration values
// eventually update your evolution matrix inside the loop
  K.F = {1.0, delta_t, 0.5 * delta_t * delta_t,
         0.0, 1.0, delta_t,
         0.0, 0.0, 1.0};
  
  // GRAB MEASUREMENT and WRITE IT INTO 'obs'
  obs(0) = sensor->accX(); // x acceleration (mg) 
  obs(1) = sensor->accY(); // y acceleration (mg) 
  obs(2) = sensor->accZ(); // z acceleration (mg) 
  
  // APPLY KALMAN FILTER
  K.update(obs);

  SERIAL_PORT.print(obs(0));
  SERIAL_PORT.print(", ");
  SERIAL_PORT.print(obs(1));
  SERIAL_PORT.print(", ");
  SERIAL_PORT.println(obs(2));

  FindVel(obs(0), obs(1), obs(2), xVel, yVel, zVel, delta_t);
  // K.update(xVel);
   //K.update(yVel);
   //K.update(zVel);

  FindPos(xVel, yVel, zVel, xPos, yPos, zPos, delta_t);

  // APPLY KALMAN FILTER
  //K.update(xPos);
 // K.update(yPos);
 // K.update(zPos);

// print position
  SERIAL_PORT.print(xPos*100);
  SERIAL_PORT.print(", ");
  SERIAL_PORT.print(yPos*100);
  SERIAL_PORT.print(", ");
  SERIAL_PORT.print(zPos*100);
  SERIAL_PORT.println();

}