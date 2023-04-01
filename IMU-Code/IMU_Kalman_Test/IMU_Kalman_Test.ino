#include "Wire.h"
#include "SparkFun_ICM_20948_AK09916.h"
#include "BLAS.h"

// Define the state vector and measurement vector sizes
const int Nstate = 3;   // State vector size (position, velocity, acceleration)
const int Nobs = 1;     // Measurement vector size (position only)

// Define the ICM 20948 object and the observation matrix
ICM_20948_I2C myICM;    // ICM 20948 object
BLA::Matrix<Nobs, Nstate> H;                // Observation matrix (maps state vector to measurement vector)

// Initialize the state vector and covariance matrix
BLA::Matrix<Nstate, 1> x_hat;               // Estimated state vector
BLA::Matrix<Nstate, Nstate> P;              // Estimated state covariance matrix

// Initialize the system dynamics matrix and process noise covariance matrix
double delta_t = 0.01;                      // Time step between measurements
BLA::Matrix<Nstate, Nstate> F;              // System dynamics matrix
BLA::Matrix<Nstate, Nstate> Q;              // Process noise covariance matrix

// Initialize the measurement covariance matrix and Kalman gain matrix
BLA::Matrix<Nobs, Nobs> R;                  // Measurement covariance matrix
BLA::Matrix<Nstate, Nobs> K;                // Kalman gain matrix

// Initialize the measurement vector
BLA::Matrix<Nobs, 1> z;                     // Measurement vector

void setup() {
  // Initialize the ICM 20948 sensor
  myICM.begin_I2C();
  myICM.initICM20948();

  // Initialize the observation matrix
  H = BLA::Zeros<Nobs, Nstate>();           // Initialize observation matrix to zeros
  H(0, 0) = 1;                              // Set observation matrix to only measure position

  // Initialize the state vector and covariance matrix
  x_hat = BLA::Zeros<Nstate, 1>();             // Initialize state to all zeros
  P = BLA::Identity<Nstate, Nstate>() * 0.01;  // Initialize covariance to diagonal matrix with small values

  // Initialize the system dynamics matrix
  F = BLA::Identity<Nstate, Nstate>();        // Constant acceleration motion model
  F(0, 1) = delta_t;
  F(0, 2) = 0.5 * delta_t * delta_t;
  F(1, 2) = delta_t;

  // Initialize the process noise covariance matrix
  Q = BLA::Zeros<Nstate, Nstate>();           // Zero process noise

  // Initialize the measurement covariance matrix and Kalman gain matrix
  R = BLA::Identity<Nobs, Nobs>() * 0.1;       // Initialize measurement covariance matrix with small value
  K = BLA::Zeros<Nstate, Nobs>();             // Initialize Kalman gain matrix to zeros

  // Initialize the measurement vector
  z = BLA::Zeros<Nobs, 1>();                  // Initialize measurement vector to zeros
}

void loop() {
  // Read acceleration data from the ICM 20948 sensor
  myICM.readSensor();

  // Extract the acceleration values from the sensor object
  double ax = myICM.ax / 16384.0;
  double ay = myICM.ay / 16384.0;
  double az = my
}
