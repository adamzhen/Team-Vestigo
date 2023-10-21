/****************************************************************
 * Example6_DMP_Quat9_Orientation.ino
 * ICM 20948 Arduino Library Demo
 * Initialize the DMP based on the TDK InvenSense ICM20948_eMD_nucleo_1.0 example-icm20948
 * Paul Clark, April 25th, 2021
 * Based on original code by:
 * Owen Lyke @ SparkFun Electronics
 * Original Creation Date: April 17 2019
 * 
 * ** This example is based on InvenSense's _confidential_ Application Note "Programming Sequence for DMP Hardware Functions".
 * ** We are grateful to InvenSense for sharing this with us.
 * 
 * ** Important note: by default the DMP functionality is disabled in the library
 * ** as the DMP firmware takes up 14301 Bytes of program memory.
 * ** To use the DMP, you will need to:
 * ** Edit ICM_20948_C.h
 * ** Uncomment line 29: #define ICM_20948_USE_DMP
 * ** Save changes
 * ** If you are using Windows, you can find ICM_20948_C.h in:
 * ** Documents\Arduino\libraries\SparkFun_ICM-20948_ArduinoLibrary\src\util
 *
 * Please see License.md for the license information.
 *
 * Distributed as-is; no warranty is given.
 ***************************************************************/

#include "ICM_20948.h" // Click here to get the library: http://librarymanager/All#SparkFun_ICM_20948_IMU
#include <ArduinoJson.h>
#include <WiFiUdp.h>
#include <vector>
#include <math.h>
#include <WiFi.h>
#include <Wire.h> // Needed for I2C

#include <SparkFun_MAX1704x_Fuel_Gauge_Arduino_Library.h> // Click here to get the library: http://librarymanager/All#SparkFun_MAX1704x_Fuel_Gauge_Arduino_Library
SFE_MAX1704X lipo(MAX1704X_MAX17048); // Allow access to all the 17048 features

//#define USE_SPI       // Uncomment this to use SPI

#define QUAT_ANIMATION // Uncomment this line to output data in the correct format for ZaneL's Node.js Quaternion animation tool: https://github.com/ZaneL/quaternion_sensor_3d_nodejs
#define SERIAL_PORT Serial

#define SPI_PORT SPI // Your desired SPI port.       Used only when "USE_SPI" is defined
#define CS_PIN 2     // Which pin you connect CS to. Used only when "USE_SPI" is defined

#define WIRE_PORT Wire // Your desired Wire port.      Used when "USE_SPI" is not defined
// The value of the last bit of the I2C address.
// On the SparkFun 9DoF IMU breakout the default is 1, and when the ADR jumper is closed the value becomes 0
#define AD0_VAL 1
#define LED_PIN 13

//#define DEBUG // used for debugging
#define TRANSMIT
#define PRINT
//#define FUELGAUGE

#ifdef USE_SPI
ICM_20948_SPI myICM; // If using SPI create an ICM_20948_SPI object
#else
ICM_20948_I2C myICM; // Otherwise create an ICM_20948_I2C object
#endif

// Wifi creds
const char *ssid;
const char *password;
const char *host;  

const int network = 1; // 0 = Router, 1 = Adam's hotspot, 2 = Aiden's hotspot 
const int port = 1234;

void setup()
{
  SERIAL_PORT.begin(9600); // Start the serial console

const char *Aiden_laptop = "192.168.8.101";
const char *Evan_laptop = "192.168.8.162";
const char *Adam_laptop = "192.168.8.203";
const char *Adam_laptop_LAN = "192.168.8.148";
const char *Aiden_PC = "192.168.8.122";
const char *Aiden_laptop_LAN = "192.168.8.219";
const char *Aiden_PC_LAN = "192.168.8.132";

#ifdef TRANSMIT
  // Setting WiFi
  if (network == 0) {
    ssid = "Vestigo-Router";
    password = "Vestigo&2023";
    //host = Aiden_laptop; 
    // host = Evan_laptop;
    // host = Adam_laptop;
    // host = Aiden_PC;
    // host = Adam_laptop_LAN; 
    // host = Aiden_laptop_LAN;
    host = Aiden_PC_LAN;
  } 
  else if (network == 1){
    ssid = "UMAT_WiFi";
    password = "andito21";
    host = "192.168.31.177"; 
  
  } 
  else if (network == 2){
    ssid = "FREE WIFI NO MALWARE";
    password = "Playadel2005?";
    host = "192.168."; 
  }

  // Connecting to WiFi
  SERIAL_PORT.print("Connecting to ");
  SERIAL_PORT.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    SERIAL_PORT.println("Connecting to Wifi... ");
  }
  SERIAL_PORT.println("Wifi connected");
#endif
#ifndef QUAT_ANIMATION
  SERIAL_PORT.println(F("ICM-20948 Example"));
#endif

#ifndef QUAT_ANIMATION
  while (SERIAL_PORT.available()) // Make sure the serial RX buffer is empty
    SERIAL_PORT.read();

  SERIAL_PORT.println(F("Press any key to continue..."));

  while (!SERIAL_PORT.available()) // Wait for the user to press a key (send any serial character)
    ;
#endif

#ifdef USE_SPI
  SPI_PORT.begin();
#else
  WIRE_PORT.begin();
  WIRE_PORT.setClock(400000);
#endif

#ifndef QUAT_ANIMATION
  //myICM.enableDebugging(); // Uncomment this line to enable helpful debug messages on Serial
#endif

  bool initialized = false;
  while (!initialized)
  {

    // Initialize the ICM-20948
    // If the DMP is enabled, .begin performs a minimal startup. We need to configure the sample mode etc. manually.
#ifdef USE_SPI
    myICM.begin(CS_PIN, SPI_PORT);
#else
    myICM.begin(WIRE_PORT, AD0_VAL);
#endif

#ifndef QUAT_ANIMATION
    SERIAL_PORT.print(F("Initialization of the sensor returned: "));
    SERIAL_PORT.println(myICM.statusString());
#endif
    if (myICM.status != ICM_20948_Stat_Ok)
    {
#ifndef QUAT_ANIMATION
      SERIAL_PORT.println(F("Trying again..."));
#endif
      delay(500);
    }
    else
    {
      initialized = true;
    }
  }

#ifndef QUAT_ANIMATION
  SERIAL_PORT.println(F("Device connected!"));
#endif

  bool success = false; // Use success to show if the DMP configuration was successful

  while (!success){
    success = true;
    // Initialize the DMP. initializeDMP is a weak function. You can overwrite it if you want to e.g. to change the sample rate
    success &= (myICM.initializeDMP() == ICM_20948_Stat_Ok);

    // DMP sensor options are defined in ICM_20948_DMP.h
    //    INV_ICM20948_SENSOR_ACCELEROMETER               (16-bit accel)
    //    INV_ICM20948_SENSOR_GYROSCOPE                   (16-bit gyro + 32-bit calibrated gyro)
    //    INV_ICM20948_SENSOR_RAW_ACCELEROMETER           (16-bit accel)
    //    INV_ICM20948_SENSOR_RAW_GYROSCOPE               (16-bit gyro + 32-bit calibrated gyro)
    //    INV_ICM20948_SENSOR_MAGNETIC_FIELD_UNCALIBRATED (16-bit compass)
    //    INV_ICM20948_SENSOR_GYROSCOPE_UNCALIBRATED      (16-bit gyro)
    //    INV_ICM20948_SENSOR_STEP_DETECTOR               (Pedometer Step Detector)
    //    INV_ICM20948_SENSOR_STEP_COUNTER                (Pedometer Step Detector)
    //    INV_ICM20948_SENSOR_GAME_ROTATION_VECTOR        (32-bit 6-axis quaternion)
    //    INV_ICM20948_SENSOR_ROTATION_VECTOR             (32-bit 9-axis quaternion + heading accuracy)
    //    INV_ICM20948_SENSOR_GEOMAGNETIC_ROTATION_VECTOR (32-bit Geomag RV + heading accuracy)
    //    INV_ICM20948_SENSOR_GEOMAGNETIC_FIELD           (32-bit calibrated compass)
    //    INV_ICM20948_SENSOR_GRAVITY                     (32-bit 6-axis quaternion)
    //    INV_ICM20948_SENSOR_LINEAR_ACCELERATION         (16-bit accel + 32-bit 6-axis quaternion)
    //    INV_ICM20948_SENSOR_ORIENTATION                 (32-bit 9-axis quaternion + heading accuracy)

    // Enable the DMP orientation sensor
    success &= (myICM.enableDMPSensor(INV_ICM20948_SENSOR_ORIENTATION) == ICM_20948_Stat_Ok);

    // Enable any additional sensors / features
    //success &= (myICM.enableDMPSensor(INV_ICM20948_SENSOR_RAW_GYROSCOPE) == ICM_20948_Stat_Ok);
    //success &= (myICM.enableDMPSensor(INV_ICM20948_SENSOR_RAW_ACCELEROMETER) == ICM_20948_Stat_Ok);
    //success &= (myICM.enableDMPSensor(INV_ICM20948_SENSOR_MAGNETIC_FIELD_UNCALIBRATED) == ICM_20948_Stat_Ok);

    // Configuring DMP to output data at multiple ODRs:
    // DMP is capable of outputting multiple sensor data at different rates to FIFO.
    // Setting value can be calculated as follows:
    // Value = (DMP running rate / ODR ) - 1
    // E.g. For a 5Hz ODR rate when DMP is running at 55Hz, value = (55/5) - 1 = 10.
    success &= (myICM.setDMPODRrate(DMP_ODR_Reg_Quat6, 0) == ICM_20948_Stat_Ok); // Set to the maximum // WHY THIS LINE!?
    //success &= (myICM.setDMPODRrate(DMP_ODR_Reg_Accel, 0) == ICM_20948_Stat_Ok); // Set to the maximum
    //success &= (myICM.setDMPODRrate(DMP_ODR_Reg_Gyro, 0) == ICM_20948_Stat_Ok); // Set to the maximum
    //success &= (myICM.setDMPODRrate(DMP_ODR_Reg_Gyro_Calibr, 0) == ICM_20948_Stat_Ok); // Set to the maximum
    //success &= (myICM.setDMPODRrate(DMP_ODR_Reg_Cpass, 0) == ICM_20948_Stat_Ok); // Set to the maximum
    //success &= (myICM.setDMPODRrate(DMP_ODR_Reg_Cpass_Calibr, 0) == ICM_20948_Stat_Ok); // Set to the maximum

    // Enable the FIFO
    success &= (myICM.enableFIFO() == ICM_20948_Stat_Ok);

    // Enable the DMP
    success &= (myICM.enableDMP() == ICM_20948_Stat_Ok);

    // Reset DMP
    success &= (myICM.resetDMP() == ICM_20948_Stat_Ok);

    // Reset FIFO
    success &= (myICM.resetFIFO() == ICM_20948_Stat_Ok);

    // Check success
    if (success)
    {
  #ifndef QUAT_ANIMATION
      SERIAL_PORT.println(F("DMP enabled!"));
  #endif
    }
    else
    {
      SERIAL_PORT.println(F("Enable DMP failed!"));
      SERIAL_PORT.println(F("Trying again..."));
      delay(500);
    }
  }

  // WAIT FOR SENSOR TO CALIBRATE (LOCK ONTO CORRECT ORIENTATION)
  int elapsedCalibration = 0;
  int calibrationTime = 3000;
  pinMode(LED_PIN, OUTPUT);
  while (elapsedCalibration <= calibrationTime){
    digitalWrite(LED_PIN, HIGH); // turn the LED on   
    delay(500);
    digitalWrite(LED_PIN, LOW); // turn the LED off  
    delay(500); 
    SERIAL_PORT.println(elapsedCalibration/1000);     
    elapsedCalibration += 1000;
  }
#ifdef FUELGAUGE
  // Set up the MAX17048 LiPo fuel gauge:
  if (lipo.begin() == false) // Connect to the MAX17048 using the default wire port
  {
    Serial.println(F("MAX17048 not detected. Please check wiring. Freezing."));
    while (1)
      ;
  }
#endif
}

// Simulating random movement of tags
std::vector<float> update_loc(const std::vector<float>& loc){ 
  std::vector<float> nloc;
  float mx = 20; // Maximum movement
  float d = 5.08; // Dimension of simulated cube room
  float dc; // Displacement from center
  
  for (const auto& el : loc){
    // Calculate random movement based on the element's proximity to the center
    float random_movement = random(-mx, mx) / 100.0;
    
    // Add random movement to the current element
    float nel = el + random_movement; // new element
    if (nel<0 || nel>d){
      nel = el - random_movement;
    }
    nloc.push_back(nel);
  }

  return nloc;
}


float roll = 0;
float pitch = 0;
float yaw = 0;
float accX;
float accY;
float accZ;
unsigned long dt;
std::vector<float> loc_1{1,1,1};
std::vector<float> loc_2{4,4,1};
std::vector<float> loc_3{1,4,1};
std::vector<float> loc_4{4,1,1};

void loop()
{
  unsigned long startTime = micros(); // declare a variable to hold the start time

  // Read any DMP data waiting in the FIFO
  // Note:
  //    readDMPdataFromFIFO will return ICM_20948_Stat_FIFONoDataAvail if no data is available.
  //    If data is available, readDMPdataFromFIFO will attempt to read _one_ frame of DMP data.
  //    readDMPdataFromFIFO will return ICM_20948_Stat_FIFOIncompleteData if a frame was present but was incomplete
  //    readDMPdataFromFIFO will return ICM_20948_Stat_Ok if a valid frame was read.
  //    readDMPdataFromFIFO will return ICM_20948_Stat_FIFOMoreDataAvail if a valid frame was read _and_ the FIFO contains more (unread) data.
  icm_20948_DMP_data_t data;
  myICM.readDMPdataFromFIFO(&data);

  int ERROR = 0;

  if ((myICM.status == ICM_20948_Stat_Ok) || (myICM.status == ICM_20948_Stat_FIFOMoreDataAvail)) // Was valid data available?
  {
#ifdef DEBUG
    SERIAL_PORT.print(F("Received data! Header: 0x")); // Print the header in HEX so we can see what data is arriving in the FIFO
    // if ( data.header < 0x1000) SERIAL_PORT.print( "0" ); // Pad the zeros
    // if ( data.header < 0x100) SERIAL_PORT.print( "0" );
    // if ( data.header < 0x10) SERIAL_PORT.print( "0" );
    // SERIAL_PORT.println(data.header, HEX);
    SERIAL_PORT.print(data.header, BIN);
    SERIAL_PORT.print(F(" & "));
    SERIAL_PORT.print(DMP_header_bitmap_Quat9, BIN);
    SERIAL_PORT.print(F(" = "));
    SERIAL_PORT.println((data.header & DMP_header_bitmap_Quat9));
#endif

    if ((data.header & DMP_header_bitmap_Quat9) > 0) // We have asked for orientation data so we should receive Quat9
    {
      // Q0 value is computed from this equation: Q0^2 + Q1^2 + Q2^2 + Q3^2 = 1.
      // In case of drift, the sum will not add to 1, therefore, quaternion data need to be corrected with right bias values.
      // The quaternion data is scaled by 2^30.

      // Scale to +/- 1
      double q1 = ((double)data.Quat9.Data.Q1) / 1073741824.0; // Convert to double. Divide by 2^30
      double q2 = ((double)data.Quat9.Data.Q2) / 1073741824.0; // Convert to double. Divide by 2^30
      double q3 = ((double)data.Quat9.Data.Q3) / 1073741824.0; // Convert to double. Divide by 2^30
      double q0 = 0.0;
      if (((q1 * q1) + (q2 * q2) + (q3 * q3)) < 1){
        q0 = sqrt(1.0 - ((q1 * q1) + (q2 * q2) + (q3 * q3)));
      } else {
        ERROR = 1;
      }
      double one = (q1 * q1) + (q2 * q2) + (q3 * q3) + (q0 * q0);

#ifdef DEBUG
      SERIAL_PORT.printf("Quat9 data is: Q1:%f Q2:%f Q3:%f Q0:%f Total:%f\r\n", q1, q2, q3, q0, one);
#endif

#ifndef QUAT_ANIMATION
      // Output the Quaternion data in the format expected by ZaneL's Node.js Quaternion animation tool
      SERIAL_PORT.print(F("{\"quat_w\":"));
      SERIAL_PORT.print(q0, 3);
      SERIAL_PORT.print(F(", \"quat_x\":"));
      SERIAL_PORT.print(q1, 3);
      SERIAL_PORT.print(F(", \"quat_y\":"));
      SERIAL_PORT.print(q2, 3);
      SERIAL_PORT.print(F(", \"quat_z\":"));
      SERIAL_PORT.print(q3, 3);
      SERIAL_PORT.println(F("}"));

#else
      if (ERROR){ // if quaternions cannot add to 1
#ifdef DEBUG
        SERIAL_PORT.println(ERROR);
        SERIAL_PORT.print(F("~ Error: ["));
        SERIAL_PORT.print(ERROR);
        SERIAL_PORT.print(F("] -> "));        
        SERIAL_PORT.println(((q1 * q1) + (q2 * q2) + (q3 * q3)), 5);
#endif
      } else {
        double q2sqr = q2 * q2;

        // roll (x-axis rotation)
        double t0 = +2.0 * (q0 * q1 + q2 * q3);
        double t1 = +1.0 - 2.0 * (q1 * q1 + q2sqr);
        roll = atan2(t0, t1) * 180.0 / PI;

        // pitch (y-axis rotation)
        double t2 = +2.0 * (q0 * q2 - q3 * q1);
        t2 = t2 > 1.0 ? 1.0 : t2;
        t2 = t2 < -1.0 ? -1.0 : t2;
        pitch = asin(t2) * 180.0 / PI;

        // yaw (z-axis rotation)
        double t3 = +2.0 * (q0 * q3 + q1 * q2);
        double t4 = +1.0 - 2.0 * (q2sqr + q3 * q3);
        yaw = atan2(t3, t4) * 180.0 / PI;
      }
#endif
    } else {
#ifdef DEBUG
      SERIAL_PORT.print(F("- - ERROR: NOT Quat9 DATA - - "));
      // print out the data
      double q1 = ((double)data.Quat9.Data.Q1) / 1073741824.0; // Convert to double. Divide by 2^30
      double q2 = ((double)data.Quat9.Data.Q2) / 1073741824.0; // Convert to double. Divide by 2^30
      double q3 = ((double)data.Quat9.Data.Q3) / 1073741824.0; // Convert to double. Divide by 2^30
      double q0 = 1.0;
      SERIAL_PORT.printf("Quat9 data is: Q1:%f Q2:%f Q3:%f\r\n", q1, q2, q3);
#endif
    }
  } else {
#ifdef DEBUG
    SERIAL_PORT.println(F("_ ERROR: NO DATA RECEIVED _"));    
#endif
  }
/*
  if (myICM.dataReady()) {
    myICM.getAGMT();         // The values are only updated when you call 'getAGMT'
                            //    printRawAGMT( myICM.agmt );     // Uncomment this to see the raw values, taken directly from the agmt structure
    //SERIAL_PORT.print(F("Scaled. Acc (mg): "));

    ICM_20948_I2C *sensor = &myICM;
    accX = sensor->accX();
    accY = sensor->accY();
    accZ = sensor->accZ();

    // correcting biases
    float biasAccX = 0;
    float biasAccY = 0;
    float biasAccZ = 0;
    if (abs(accY-1000) < 50){
      biasAccY = -12;      
    } 
    if (abs(accZ-1000) < 50){
      biasAccZ = 22;      
    } 
    accX -= biasAccX;
    accY -= biasAccY;
    accZ -= biasAccZ;

    // float gyrX = sensor->gyrX();
    // float gyrY = sensor->gyrY();
    // float gyrZ = sensor->gyrZ();

    // calculating gravity components
    // float gx = -1000 * sin(pitch * PI/180);      
    // float gy = 1000 * cos(pitch * PI/180) * sin(roll * PI/180);  
    // float gz = 1000 * cos(pitch * PI/180) * cos(roll * PI/180);

    // calculating orientation
    // float zerodir = 180.0; // compass direction (degrees from North) where yaw is 0
    // float compass = zerodir - yaw;
    // if (compass < 0){
    //   compass += 360;
    // }
    // float angle1 = 90-roll; // tilt up = positive, tilt down = negative
    // float angle2 = -pitch; // right = positive, left = negative

    //SERIAL_PORT.printf("%f, %f, %f, %f, %f, %f", roll, pitch, yaw, accX-gx, accY-gy, accZ-gz);
    //SERIAL_PORT.printf("%f, %f, %f, %f, %f, %f, %f, %f, %f", roll, pitch, yaw, gx, gy, gz, accX, accY, accZ);
    //SERIAL_PORT.println();
}
*/
#ifdef PRINT
    bool formatted = false;
    if (formatted){
      SERIAL_PORT.print(F("Roll:"));
      SERIAL_PORT.print(roll, 1);
      SERIAL_PORT.print(F(" Pitch:"));
      SERIAL_PORT.print(pitch, 1);
      SERIAL_PORT.print(F(" Yaw:"));
      SERIAL_PORT.println(yaw, 1);
      SERIAL_PORT.print(F("Ax:"));
      // SERIAL_PORT.print(accX-gx, 1);
      // SERIAL_PORT.print(F(" Ay:"));
      // SERIAL_PORT.print(accY-gy, 1);
      // SERIAL_PORT.print(F(" Az:"));
      // SERIAL_PORT.println(accZ-gz, 1);

    } else {
      char sep = ',';
      // SERIAL_PORT.print(roll, 2);
      // SERIAL_PORT.print(sep);
      // SERIAL_PORT.print(pitch, 2);
      // SERIAL_PORT.print(sep);
      // SERIAL_PORT.print(yaw, 2);
      // SERIAL_PORT.println(sep);
      // SERIAL_PORT.print(accX-gx, 2);
      // SERIAL_PORT.print(sep);
      // SERIAL_PORT.print(accY-gy, 2);
      // SERIAL_PORT.print(sep);
      // SERIAL_PORT.println(accZ-gz, 2);
    }
    // //SERIAL_PORT.print(", Mag (uT): Mx: ");
    // // SERIAL_PORT.printf("%c Mx:", sep);
    // // SERIAL_PORT.print(sensor->magX()); //printFormattedFloat(sensor->magX(), 5, 2);
    // // SERIAL_PORT.printf("%c My:", sep);
    // // SERIAL_PORT.print(sensor->magY()); //printFormattedFloat(sensor->magY(), 5, 2);
    // // SERIAL_PORT.printf("%c Mz:", sep);
    // // SERIAL_PORT.print(sensor->magZ()); //printFormattedFloat(sensor->magZ(), 5, 2);
    // // SERIAL_PORT.print(" ], Tmp (C) [ ");
#endif

#ifdef DEBUG
  else {
    SERIAL_PORT.println("Waiting for data");
    delay(0.1); }
#endif


#ifdef TRANSMIT
  // Simulates position of tags and anchors
  std::vector<std::vector<float>> anchors{{0, 0, 0}, {0, 5.08, 0}, {5.08, 0, 0}, {5.08, 5.08, 0}, {2.54, 0, 0}, {0, 2.54, 0}, {0, 0, 2.54}, {0, 5.08, 2.54}, {5.08, 0, 2.54}, {5.08, 5.08, 2.54}, {2.54, 0, 2.54}, {0, 2.54, 2.54}};
  loc_1 = update_loc(loc_1);
  loc_2 = update_loc(loc_2);
  loc_3 = update_loc(loc_3);
  loc_4 = update_loc(loc_4);
  // Initializes tag data vectors
  std::vector<float> tag_1{1,1,1,1,1,1,1,1,1,1,1,1,yaw};
  std::vector<float> tag_2{1,1,1,1,1,1,1,1,1,1,1,1,yaw};
  std::vector<float> tag_3{1,1,1,1,1,1,1,1,1,1,1,1,yaw};
  std::vector<float> tag_4{1,1,1,1,1,1,1,1,1,1,1,1,yaw};
  // Calculates simulated distances
  for (int i=0; i<12; i++){
    tag_1[i] = sqrt( (anchors[i][0]-loc_1[0])*(anchors[i][0]-loc_1[0]) + (anchors[i][1]-loc_1[1])*(anchors[i][1]-loc_1[1]) + (anchors[i][2]-loc_1[2])*(anchors[i][2]-loc_1[2]) );
    tag_2[i] = sqrt( (anchors[i][0]-loc_2[0])*(anchors[i][0]-loc_2[0]) + (anchors[i][1]-loc_2[1])*(anchors[i][1]-loc_2[1]) + (anchors[i][2]-loc_2[2])*(anchors[i][2]-loc_2[2]) );
    tag_3[i] = sqrt( (anchors[i][0]-loc_3[0])*(anchors[i][0]-loc_3[0]) + (anchors[i][1]-loc_3[1])*(anchors[i][1]-loc_3[1]) + (anchors[i][2]-loc_3[2])*(anchors[i][2]-loc_3[2]) );
    tag_4[i] = sqrt( (anchors[i][0]-loc_4[0])*(anchors[i][0]-loc_4[0]) + (anchors[i][1]-loc_4[1])*(anchors[i][1]-loc_4[1]) + (anchors[i][2]-loc_4[2])*(anchors[i][2]-loc_4[2]) );
  }
  // Creates matrix to be transmitted 
  std::vector<std::vector<float>> all_data;
  all_data.push_back(tag_1);
  all_data.push_back(tag_2);
  all_data.push_back(tag_3);
  all_data.push_back(tag_4);

  // Serial write all data
  StaticJsonDocument<1024> doc;
  int tag_id = 0;
  for (const auto& row : all_data) {
    JsonArray data = doc.createNestedArray(String(++tag_id));
    for (const auto& element : row) {
      data.add(element);
    }
  }
  
  byte buffer[1024];
  size_t nBytes = serializeJson(doc, buffer, sizeof(buffer));

  Serial.write('<'); // Start delimiter
  Serial.write(buffer, nBytes); // Write the raw JSON data
  Serial.write('>'); // End delimiter

  all_data.clear();
#endif 

  if (myICM.status != ICM_20948_Stat_FIFOMoreDataAvail) // If more data is available then we should read it right away, otherwise wait
  {
    delay(0.1);
  } 

  dt = (micros() - startTime);
  // Serial.println(dt); // print the elapsed time in milliseconds
#ifdef FUELGAUGE
    // Print the variables:
  Serial.print("Voltage: ");
  Serial.print(lipo.getVoltage());  // Print the battery voltage
  Serial.print("V");

  Serial.print(" Percentage: ");
  Serial.print(lipo.getSOC(), 2); // Print the battery state of charge with 2 decimal places
  Serial.print("%");

  Serial.print(" Change Rate: ");
  Serial.print(lipo.getChangeRate(), 2); // Print the battery change rate with 2 decimal places
  Serial.print("%/hr");

  Serial.println();
#endif

}