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

//#define USE_SPI       // Uncomment this to use SPI

#define QUAT_ANIMATION // Uncomment this line to output data in the correct format for ZaneL's Node.js Quaternion animation tool: https://github.com/ZaneL/quaternion_sensor_3d_nodejs
#define SERIAL_PORT Serial

#define SPI_PORT SPI // Your desired SPI port.       Used only when "USE_SPI" is defined
#define CS_PIN 2     // Which pin you connect CS to. Used only when "USE_SPI" is defined

#define WIRE_PORT Wire // Your desired Wire port.      Used when "USE_SPI" is not defined
// The value of the last bit of the I2C address.
// On the SparkFun 9DoF IMU breakout the default is 1, and when the ADR jumper is closed the value becomes 0
#define AD0_VAL 1

//#define DEBUG // used for debugging
#define TRANSMIT
//#define PRINT

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
const int port = 1233;

void setup()
{
  SERIAL_PORT.begin(9600); // Start the serial console

#ifdef TRANSMIT
  // Setting WiFi
  if (network == 0) {
    ssid = "";
    password = "";
    host = "192.168.";   
  } 
  else if (network == 1){
    ssid = "UMAT_WiFi";
    password = "andito21";
    host = "192.168.106.177"; 
  
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
}

float roll = 0;
float pitch = 0;
float yaw = 0;
float accX;
float accY;
float accZ;
unsigned long dt;

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
        //SERIAL_PORT.println(ERROR);
#ifdef DEBUG
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

  if (myICM.dataReady()) {
    myICM.getAGMT();         // The values are only updated when you call 'getAGMT'
                            //    printRawAGMT( myICM.agmt );     // Uncomment this to see the raw values, taken directly from the agmt structure
    //SERIAL_PORT.print(F("Scaled. Acc (mg): "));

    float biasAccX = 0;
    float biasAccY = 0;
    float biasAccZ = 20;

    ICM_20948_I2C *sensor = &myICM;
    accX = sensor->accX() - biasAccX;
    accY = sensor->accY() - biasAccY;
    accZ = sensor->accZ() - biasAccZ;
    // float gyrX = sensor->gyrX();
    // float gyrY = sensor->gyrY();
    // float gyrZ = sensor->gyrZ();

    // calculating gravity components
    float gx = -1000 * sin(pitch * PI/180);      
    float gy = 1000 * cos(pitch * PI/180) * sin(roll * PI/180);  
    float gz = 1000 * cos(pitch * PI/180) * cos(roll * PI/180);

    // calculating orientation
    float zerodir = 180.0; // compass direction (degrees from North) where yaw is 0
    float compass = zerodir - yaw;
    if (compass < 0){
      compass += 360;
    }
    float angle1 = 90-roll; // tilt up = positive, tilt down = negative
    float angle2 = -pitch; // right = positive, left = negative

    //SERIAL_PORT.printf("%f, %f, %f, %f, %f, %f", roll, pitch, yaw, accX-gx, accY-gy, accZ-gz);
    //SERIAL_PORT.printf("%f, %f, %f, %f, %f, %f, %f, %f, %f", roll, pitch, yaw, gx, gy, gz, accX, accY, accZ);
    //SERIAL_PORT.println();
      
#ifdef PRINT
    bool formatted = true;
    if (formatted){
      SERIAL_PORT.print(F("Roll:"));
      SERIAL_PORT.print(roll, 1);
      SERIAL_PORT.print(F(" Pitch:"));
      SERIAL_PORT.print(pitch, 1);
      SERIAL_PORT.print(F(" Yaw:"));
      SERIAL_PORT.println(yaw, 1);
      // SERIAL_PORT.print(F(" Compass:"));
      // SERIAL_PORT.println(compass, 1);
      // SERIAL_PORT.print(F(" Angle 1:"));
      // SERIAL_PORT.println(angle1, 1);
      // SERIAL_PORT.print(F(" Angle 2:"));
      // SERIAL_PORT.println(angle2, 1);
    } else {
      char sep = ',';
      SERIAL_PORT.print(roll, 1);
      SERIAL_PORT.print(F(sep));
      SERIAL_PORT.print(pitch, 1);
      SERIAL_PORT.print(F(sep));
      SERIAL_PORT.println(yaw, 1);   
      SERIAL_PORT.print(F(sep));
      SERIAL_PORT.print(accX-gx, 2);
      SERIAL_PORT.print(F(sep));
      SERIAL_PORT.print(accY-gy, 2);
      SERIAL_PORT.print(F(sep));
      SERIAL_PORT.print(accZ-gz, 2);
      SERIAL_PORT.print(F(sep));
    }
    // SERIAL_PORT.printf(" Ax:%f", accX-gx);
    // SERIAL_PORT.printf(" Ay:%f", sep, accY-gy);
    // SERIAL_PORT.printf(" Az:%f", sep, accZ-gz);
    // //SERIAL_PORT.print(", Gyr (DPS): Gx: ");
    // // SERIAL_PORT.printf("%c gx: %f ", sep, gx);
    // // SERIAL_PORT.printf("%c gy: %f ", sep, gy);
    // // SERIAL_PORT.printf("%c gz: %f ", sep, gz);
    // //SERIAL_PORT.print(", Mag (uT): Mx: ");
    // // SERIAL_PORT.printf("%c Mx:", sep);
    // // SERIAL_PORT.print(sensor->magX()); //printFormattedFloat(sensor->magX(), 5, 2);
    // // SERIAL_PORT.printf("%c My:", sep);
    // // SERIAL_PORT.print(sensor->magY()); //printFormattedFloat(sensor->magY(), 5, 2);
    // // SERIAL_PORT.printf("%c Mz:", sep);
    // // SERIAL_PORT.print(sensor->magZ()); //printFormattedFloat(sensor->magZ(), 5, 2);
    // // SERIAL_PORT.print(" ], Tmp (C) [ ");
#endif
  }
#ifdef DEBUG
  else {
    SERIAL_PORT.println("Waiting for data");
    delay(0.1); }
#endif

#ifdef TRANSMIT
  std::vector<float> imu_data;
  // roll, pitch, yaw, ax, ay, az, dt
  imu_data.push_back(roll);
  imu_data.push_back(pitch);
  imu_data.push_back(yaw);
  imu_data.push_back(accX);
  imu_data.push_back(accY);
  imu_data.push_back(accZ);
  imu_data.push_back(dt);

  // convert vector into Json array
  const size_t capacity = JSON_ARRAY_SIZE(7);
  DynamicJsonDocument doc(capacity);
  JsonArray angle_data = doc.to<JsonArray>();
  for (int i = 0; i < imu_data.size(); i++){
    //SERIAL_PORT.print("Added Point: ");
    //SERIAL_PORT.println(imu_data[i]);
    angle_data.add(imu_data[i]);
  }

  // convert the Json array to a string
  String jsonString;
  serializeJson(doc, jsonString);

  // Create a UDP connection to the laptop
  WiFiUDP udp;
  udp.begin(port);
  IPAddress ip;
  if (WiFi.hostByName(host, ip)) {
    // Send the Json data over the socket connection
    udp.beginPacket(ip, port);
    udp.write((uint8_t*)jsonString.c_str(), jsonString.length());
    udp.endPacket();
    SERIAL_PORT.println("Json data sent");
    SERIAL_PORT.println(jsonString.length());
  } else {
    SERIAL_PORT.println("Unable to resolve hostname");
  }

  imu_data.clear();
#endif 

  if (myICM.status != ICM_20948_Stat_FIFOMoreDataAvail) // If more data is available then we should read it right away, otherwise wait
  {
    delay(0.1);
  } 

  dt = micros() - startTime;
  //Serial.println(dt); // print the elapsed time in milliseconds
}

// initializeDMP is a weak function. Let's overwrite it so we can increase the sample rate
// ICM_20948_Status_e ICM_20948::initializeDMP(void)
// {
//   // The ICM-20948 is awake and ready but hasn't been configured. Let's step through the configuration
//   // sequence from InvenSense's _confidential_ Application Note "Programming Sequence for DMP Hardware Functions".

//   ICM_20948_Status_e  result = ICM_20948_Stat_Ok; // Use result and worstResult to show if the configuration was successful
//   ICM_20948_Status_e  worstResult = ICM_20948_Stat_Ok;

//   // Normally, when the DMP is not enabled, startupMagnetometer (called by startupDefault, which is called by begin) configures the AK09916 magnetometer
//   // to run at 100Hz by setting the CNTL2 register (0x31) to 0x08. Then the ICM20948's I2C_SLV0 is configured to read
//   // nine bytes from the mag every sample, starting from the STATUS1 register (0x10). ST1 includes the DRDY (Data Ready) bit.
//   // Next are the six magnetometer readings (little endian). After a dummy byte, the STATUS2 register (0x18) contains the HOFL (Overflow) bit.
//   //
//   // But looking very closely at the InvenSense example code, we can see in inv_icm20948_resume_akm (in Icm20948AuxCompassAkm.c) that,
//   // when the DMP is running, the magnetometer is set to Single Measurement (SM) mode and that ten bytes are read, starting from the reserved
//   // RSV2 register (0x03). The datasheet does not define what registers 0x04 to 0x0C contain. There is definitely some secret sauce in here...
//   // The magnetometer data appears to be big endian (not little endian like the HX/Y/Z registers) and starts at register 0x04.
//   // We had to examine the I2C traffic between the master and the AK09916 on the AUX_DA and AUX_CL pins to discover this...
//   //
//   // So, we need to set up I2C_SLV0 to do the ten byte reading. The parameters passed to i2cControllerConfigurePeripheral are:
//   // 0: use I2C_SLV0
//   // MAG_AK09916_I2C_ADDR: the I2C address of the AK09916 magnetometer (0x0C unshifted)
//   // AK09916_REG_RSV2: we start reading here (0x03). Secret sauce...
//   // 10: we read 10 bytes each cycle
//   // true: set the I2C_SLV0_RNW ReadNotWrite bit so we read the 10 bytes (not write them)
//   // true: set the I2C_SLV0_CTRL I2C_SLV0_EN bit to enable reading from the peripheral at the sample rate
//   // false: clear the I2C_SLV0_CTRL I2C_SLV0_REG_DIS (we want to write the register value)
//   // true: set the I2C_SLV0_CTRL I2C_SLV0_GRP bit to show the register pairing starts at byte 1+2 (copied from inv_icm20948_resume_akm)
//   // true: set the I2C_SLV0_CTRL I2C_SLV0_BYTE_SW to byte-swap the data from the mag (copied from inv_icm20948_resume_akm)
//   result = i2cControllerConfigurePeripheral(0, MAG_AK09916_I2C_ADDR, AK09916_REG_RSV2, 10, true, true, false, true, true); if (result > worstResult) worstResult = result;
//   //
//   // We also need to set up I2C_SLV1 to do the Single Measurement triggering:
//   // 1: use I2C_SLV1
//   // MAG_AK09916_I2C_ADDR: the I2C address of the AK09916 magnetometer (0x0C unshifted)
//   // AK09916_REG_CNTL2: we start writing here (0x31)
//   // 1: not sure why, but the write does not happen if this is set to zero
//   // false: clear the I2C_SLV0_RNW ReadNotWrite bit so we write the dataOut byte
//   // true: set the I2C_SLV0_CTRL I2C_SLV0_EN bit. Not sure why, but the write does not happen if this is clear
//   // false: clear the I2C_SLV0_CTRL I2C_SLV0_REG_DIS (we want to write the register value)
//   // false: clear the I2C_SLV0_CTRL I2C_SLV0_GRP bit
//   // false: clear the I2C_SLV0_CTRL I2C_SLV0_BYTE_SW bit
//   // AK09916_mode_single: tell I2C_SLV1 to write the Single Measurement command each sample
//   result = i2cControllerConfigurePeripheral(1, MAG_AK09916_I2C_ADDR, AK09916_REG_CNTL2, 1, false, true, false, false, false, AK09916_mode_single); if (result > worstResult) worstResult = result;

//   // Set the I2C Master ODR configuration
//   // It is not clear why we need to do this... But it appears to be essential! From the datasheet:
//   // "I2C_MST_ODR_CONFIG[3:0]: ODR configuration for external sensor when gyroscope and accelerometer are disabled.
//   //  ODR is computed as follows: 1.1 kHz/(2^((odr_config[3:0])) )
//   //  When gyroscope is enabled, all sensors (including I2C_MASTER) use the gyroscope ODR.
//   //  If gyroscope is disabled, then all sensors (including I2C_MASTER) use the accelerometer ODR."
//   // Since both gyro and accel are running, setting this register should have no effect. But it does. Maybe because the Gyro and Accel are placed in Low Power Mode (cycled)?
//   // You can see by monitoring the Aux I2C pins that the next three lines reduce the bus traffic (magnetometer reads) from 1125Hz to the chosen rate: 68.75Hz in this case.
//   result = setBank(3); if (result > worstResult) worstResult = result; // Select Bank 3
//   uint8_t mstODRconfig = 0x04; // Set the ODR configuration to 1100/2^4 = 68.75Hz
//   result = write(AGB3_REG_I2C_MST_ODR_CONFIG, &mstODRconfig, 1); if (result > worstResult) worstResult = result; // Write one byte to the I2C_MST_ODR_CONFIG register  

//   // Configure clock source through PWR_MGMT_1
//   // ICM_20948_Clock_Auto selects the best available clock source – PLL if ready, else use the Internal oscillator
//   result = setClockSource(ICM_20948_Clock_Auto); if (result > worstResult) worstResult = result; // This is shorthand: success will be set to false if setClockSource fails

//   // Enable accel and gyro sensors through PWR_MGMT_2
//   // Enable Accelerometer (all axes) and Gyroscope (all axes) by writing zero to PWR_MGMT_2
//   result = setBank(0); if (result > worstResult) worstResult = result;                               // Select Bank 0
//   uint8_t pwrMgmt2 = 0x40;                                                          // Set the reserved bit 6 (pressure sensor disable?)
//   result = write(AGB0_REG_PWR_MGMT_2, &pwrMgmt2, 1); if (result > worstResult) worstResult = result; // Write one byte to the PWR_MGMT_2 register

//   // Place _only_ I2C_Master in Low Power Mode (cycled) via LP_CONFIG
//   // The InvenSense Nucleo example initially puts the accel and gyro into low power mode too, but then later updates LP_CONFIG so only the I2C_Master is in Low Power Mode
//   result = setSampleMode(ICM_20948_Internal_Mst, ICM_20948_Sample_Mode_Cycled); if (result > worstResult) worstResult = result;

//   // Disable the FIFO
//   result = enableFIFO(false); if (result > worstResult) worstResult = result;

//   // Disable the DMP
//   result = enableDMP(false); if (result > worstResult) worstResult = result;

//   // Set Gyro FSR (Full scale range) to 2000dps through GYRO_CONFIG_1
//   // Set Accel FSR (Full scale range) to 4g through ACCEL_CONFIG
//   ICM_20948_fss_t myFSS; // This uses a "Full Scale Settings" structure that can contain values for all configurable sensors
//   myFSS.a = gpm4;        // (ICM_20948_ACCEL_CONFIG_FS_SEL_e)
//                          // gpm2
//                          // gpm4
//                          // gpm8
//                          // gpm16
//   myFSS.g = dps2000;     // (ICM_20948_GYRO_CONFIG_1_FS_SEL_e)
//                          // dps250
//                          // dps500
//                          // dps1000
//                          // dps2000
//   result = setFullScale((ICM_20948_Internal_Acc | ICM_20948_Internal_Gyr), myFSS); if (result > worstResult) worstResult = result;

//   // The InvenSense Nucleo code also enables the gyro DLPF (but leaves GYRO_DLPFCFG set to zero = 196.6Hz (3dB))
//   // We found this by going through the SPI data generated by ZaneL's Teensy-ICM-20948 library byte by byte...
//   // The gyro DLPF is enabled by default (GYRO_CONFIG_1 = 0x01) so the following line should have no effect, but we'll include it anyway
//   result = enableDLPF(ICM_20948_Internal_Gyr, true); if (result > worstResult) worstResult = result;

//   // Enable interrupt for FIFO overflow from FIFOs through INT_ENABLE_2
//   // If we see this interrupt, we'll need to reset the FIFO
//   //result = intEnableOverflowFIFO( 0x1F ); if (result > worstResult) worstResult = result; // Enable the interrupt on all FIFOs

//   // Turn off what goes into the FIFO through FIFO_EN_1, FIFO_EN_2
//   // Stop the peripheral data from being written to the FIFO by writing zero to FIFO_EN_1
//   result = setBank(0); if (result > worstResult) worstResult = result; // Select Bank 0
//   uint8_t zero = 0;
//   result = write(AGB0_REG_FIFO_EN_1, &zero, 1); if (result > worstResult) worstResult = result;
//   // Stop the accelerometer, gyro and temperature data from being written to the FIFO by writing zero to FIFO_EN_2
//   result = write(AGB0_REG_FIFO_EN_2, &zero, 1); if (result > worstResult) worstResult = result;

//   // Turn off data ready interrupt through INT_ENABLE_1
//   result = intEnableRawDataReady(false); if (result > worstResult) worstResult = result;

//   // Reset FIFO through FIFO_RST
//   result = resetFIFO(); if (result > worstResult) worstResult = result;

//   // Set gyro sample rate divider with GYRO_SMPLRT_DIV
//   // Set accel sample rate divider with ACCEL_SMPLRT_DIV_2
//   ICM_20948_smplrt_t mySmplrt;
//   //mySmplrt.g = 19; // ODR is computed as follows: 1.1 kHz/(1+GYRO_SMPLRT_DIV[7:0]). 19 = 55Hz. InvenSense Nucleo example uses 19 (0x13).
//   //mySmplrt.a = 19; // ODR is computed as follows: 1.125 kHz/(1+ACCEL_SMPLRT_DIV[11:0]). 19 = 56.25Hz. InvenSense Nucleo example uses 19 (0x13).
//   mySmplrt.g = 4; // 225Hz
//   mySmplrt.a = 4; // 225Hz
//   //mySmplrt.g = 8; // 112Hz
//   //mySmplrt.a = 8; // 112Hz
//   result = setSampleRate((ICM_20948_Internal_Acc | ICM_20948_Internal_Gyr), mySmplrt); if (result > worstResult) worstResult = result;

//   // Setup DMP start address through PRGM_STRT_ADDRH/PRGM_STRT_ADDRL
//   result = setDMPstartAddress(); if (result > worstResult) worstResult = result; // Defaults to DMP_START_ADDRESS

//   // Now load the DMP firmware
//   result = loadDMPFirmware(); if (result > worstResult) worstResult = result;

//   // Write the 2 byte Firmware Start Value to ICM PRGM_STRT_ADDRH/PRGM_STRT_ADDRL
//   result = setDMPstartAddress(); if (result > worstResult) worstResult = result; // Defaults to DMP_START_ADDRESS

//   // Set the Hardware Fix Disable register to 0x48
//   result = setBank(0); if (result > worstResult) worstResult = result; // Select Bank 0
//   uint8_t fix = 0x48;
//   result = write(AGB0_REG_HW_FIX_DISABLE, &fix, 1); if (result > worstResult) worstResult = result;

//   // Set the Single FIFO Priority Select register to 0xE4
//   result = setBank(0); if (result > worstResult) worstResult = result; // Select Bank 0
//   uint8_t fifoPrio = 0xE4;
//   result = write(AGB0_REG_SINGLE_FIFO_PRIORITY_SEL, &fifoPrio, 1); if (result > worstResult) worstResult = result;

//   // Configure Accel scaling to DMP
//   // The DMP scales accel raw data internally to align 1g as 2^25
//   // In order to align internal accel raw data 2^25 = 1g write 0x04000000 when FSR is 4g
//   const unsigned char accScale[4] = {0x04, 0x00, 0x00, 0x00};
//   result = writeDMPmems(ACC_SCALE, 4, &accScale[0]); if (result > worstResult) worstResult = result; // Write accScale to ACC_SCALE DMP register
//   // In order to output hardware unit data as configured FSR write 0x00040000 when FSR is 4g
//   const unsigned char accScale2[4] = {0x00, 0x04, 0x00, 0x00};
//   result = writeDMPmems(ACC_SCALE2, 4, &accScale2[0]); if (result > worstResult) worstResult = result; // Write accScale2 to ACC_SCALE2 DMP register

//   // Configure Compass mount matrix and scale to DMP
//   // The mount matrix write to DMP register is used to align the compass axes with accel/gyro.
//   // This mechanism is also used to convert hardware unit to uT. The value is expressed as 1uT = 2^30.
//   // Each compass axis will be converted as below:
//   // X = raw_x * CPASS_MTX_00 + raw_y * CPASS_MTX_01 + raw_z * CPASS_MTX_02
//   // Y = raw_x * CPASS_MTX_10 + raw_y * CPASS_MTX_11 + raw_z * CPASS_MTX_12
//   // Z = raw_x * CPASS_MTX_20 + raw_y * CPASS_MTX_21 + raw_z * CPASS_MTX_22
//   // The AK09916 produces a 16-bit signed output in the range +/-32752 corresponding to +/-4912uT. 1uT = 6.66 ADU.
//   // 2^30 / 6.66666 = 161061273 = 0x9999999
//   const unsigned char mountMultiplierZero[4] = {0x00, 0x00, 0x00, 0x00};
//   const unsigned char mountMultiplierPlus[4] = {0x09, 0x99, 0x99, 0x99};  // Value taken from InvenSense Nucleo example
//   const unsigned char mountMultiplierMinus[4] = {0xF6, 0x66, 0x66, 0x67}; // Value taken from InvenSense Nucleo example
//   result = writeDMPmems(CPASS_MTX_00, 4, &mountMultiplierPlus[0]); if (result > worstResult) worstResult = result;
//   result = writeDMPmems(CPASS_MTX_01, 4, &mountMultiplierZero[0]); if (result > worstResult) worstResult = result;
//   result = writeDMPmems(CPASS_MTX_02, 4, &mountMultiplierZero[0]); if (result > worstResult) worstResult = result;
//   result = writeDMPmems(CPASS_MTX_10, 4, &mountMultiplierZero[0]); if (result > worstResult) worstResult = result;
//   result = writeDMPmems(CPASS_MTX_11, 4, &mountMultiplierMinus[0]); if (result > worstResult) worstResult = result;
//   result = writeDMPmems(CPASS_MTX_12, 4, &mountMultiplierZero[0]); if (result > worstResult) worstResult = result;
//   result = writeDMPmems(CPASS_MTX_20, 4, &mountMultiplierZero[0]); if (result > worstResult) worstResult = result;
//   result = writeDMPmems(CPASS_MTX_21, 4, &mountMultiplierZero[0]); if (result > worstResult) worstResult = result;
//   result = writeDMPmems(CPASS_MTX_22, 4, &mountMultiplierMinus[0]); if (result > worstResult) worstResult = result;

//   // Configure the B2S Mounting Matrix
//   const unsigned char b2sMountMultiplierZero[4] = {0x00, 0x00, 0x00, 0x00};
//   const unsigned char b2sMountMultiplierPlus[4] = {0x40, 0x00, 0x00, 0x00}; // Value taken from InvenSense Nucleo example
//   result = writeDMPmems(B2S_MTX_00, 4, &b2sMountMultiplierPlus[0]); if (result > worstResult) worstResult = result;
//   result = writeDMPmems(B2S_MTX_01, 4, &b2sMountMultiplierZero[0]); if (result > worstResult) worstResult = result;
//   result = writeDMPmems(B2S_MTX_02, 4, &b2sMountMultiplierZero[0]); if (result > worstResult) worstResult = result;
//   result = writeDMPmems(B2S_MTX_10, 4, &b2sMountMultiplierZero[0]); if (result > worstResult) worstResult = result;
//   result = writeDMPmems(B2S_MTX_11, 4, &b2sMountMultiplierPlus[0]); if (result > worstResult) worstResult = result;
//   result = writeDMPmems(B2S_MTX_12, 4, &b2sMountMultiplierZero[0]); if (result > worstResult) worstResult = result;
//   result = writeDMPmems(B2S_MTX_20, 4, &b2sMountMultiplierZero[0]); if (result > worstResult) worstResult = result;
//   result = writeDMPmems(B2S_MTX_21, 4, &b2sMountMultiplierZero[0]); if (result > worstResult) worstResult = result;
//   result = writeDMPmems(B2S_MTX_22, 4, &b2sMountMultiplierPlus[0]); if (result > worstResult) worstResult = result;

//   // Configure the DMP Gyro Scaling Factor
//   // @param[in] gyro_div Value written to GYRO_SMPLRT_DIV register, where
//   //            0=1125Hz sample rate, 1=562.5Hz sample rate, ... 4=225Hz sample rate, ...
//   //            10=102.2727Hz sample rate, ... etc.
//   // @param[in] gyro_level 0=250 dps, 1=500 dps, 2=1000 dps, 3=2000 dps
//   result = setGyroSF(4, 3); if (result > worstResult) worstResult = result; // 4 = 225Hz (see above), 3 = 2000dps (see above)

//   // Configure the Gyro full scale
//   // 2000dps : 2^28
//   // 1000dps : 2^27
//   //  500dps : 2^26
//   //  250dps : 2^25
//   const unsigned char gyroFullScale[4] = {0x10, 0x00, 0x00, 0x00}; // 2000dps : 2^28
//   result = writeDMPmems(GYRO_FULLSCALE, 4, &gyroFullScale[0]); if (result > worstResult) worstResult = result;

//   // Configure the Accel Only Gain: 15252014 (225Hz) 30504029 (112Hz) 61117001 (56Hz)
//   //const unsigned char accelOnlyGain[4] = {0x03, 0xA4, 0x92, 0x49}; // 56Hz
//   const unsigned char accelOnlyGain[4] = {0x00, 0xE8, 0xBA, 0x2E}; // 225Hz
//   //const unsigned char accelOnlyGain[4] = {0x01, 0xD1, 0x74, 0x5D}; // 112Hz
//   result = writeDMPmems(ACCEL_ONLY_GAIN, 4, &accelOnlyGain[0]); if (result > worstResult) worstResult = result;

//   // Configure the Accel Alpha Var: 1026019965 (225Hz) 977872018 (112Hz) 882002213 (56Hz)
//   //const unsigned char accelAlphaVar[4] = {0x34, 0x92, 0x49, 0x25}; // 56Hz
//   const unsigned char accelAlphaVar[4] = {0x3D, 0x27, 0xD2, 0x7D}; // 225Hz
//   //const unsigned char accelAlphaVar[4] = {0x3A, 0x49, 0x24, 0x92}; // 112Hz
//   result = writeDMPmems(ACCEL_ALPHA_VAR, 4, &accelAlphaVar[0]); if (result > worstResult) worstResult = result;

//   // Configure the Accel A Var: 47721859 (225Hz) 95869806 (112Hz) 191739611 (56Hz)
//   //const unsigned char accelAVar[4] = {0x0B, 0x6D, 0xB6, 0xDB}; // 56Hz
//   const unsigned char accelAVar[4] = {0x02, 0xD8, 0x2D, 0x83}; // 225Hz
//   //const unsigned char accelAVar[4] = {0x05, 0xB6, 0xDB, 0x6E}; // 112Hz
//   result = writeDMPmems(ACCEL_A_VAR, 4, &accelAVar[0]); if (result > worstResult) worstResult = result;

//   // Configure the Accel Cal Rate
//   const unsigned char accelCalRate[4] = {0x00, 0x00}; // Value taken from InvenSense Nucleo example
//   result = writeDMPmems(ACCEL_CAL_RATE, 2, &accelCalRate[0]); if (result > worstResult) worstResult = result;

//   // Configure the Compass Time Buffer. The I2C Master ODR Configuration (see above) sets the magnetometer read rate to 68.75Hz.
//   // Let's set the Compass Time Buffer to 69 (Hz).
//   const unsigned char compassRate[2] = {0x00, 0x45}; // 69Hz
//   result = writeDMPmems(CPASS_TIME_BUFFER, 2, &compassRate[0]); if (result > worstResult) worstResult = result;

//   // Enable DMP interrupt
//   // This would be the most efficient way of getting the DMP data, instead of polling the FIFO
//   //result = intEnableDMP(true); if (result > worstResult) worstResult = result;

//   return worstResult;
// }
