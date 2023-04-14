#include <iostream>
#include <Eigen/Dense>
#include <string>
#include <cstring>
#include <ws2tcpip.h>
#include <cmath>

#define BUF_SIZE 1024
#define WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#pragma comment(lib, "ws2_32.lib")

#include <WinSock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#include <SDL.h>
#undef main
#include <fstream>
#include <vector>
#include <tuple>

// Kalman includes
#include "SystemModel.hpp"
#include "PositionMeasurementModel.hpp"
#include <kalman/ExtendedKalmanFilter.hpp>
#include <kalman/UnscentedKalmanFilter.hpp>

typedef float T;
// Some type shortcuts
typedef KalmanExamples::Vestigo::State<T> State;
typedef KalmanExamples::Vestigo::Control<T> Control;
typedef KalmanExamples::Vestigo::SystemModel<T> SystemModel;

typedef KalmanExamples::Vestigo::PositionMeasurement<T> PositionMeasurement;
typedef KalmanExamples::Vestigo::PositionMeasurementModel<T> PositionModel;

using std::cout;
using std::cin;
using std::endl;

// Definitions
const double inch_to_meter = 0.0254;
std::vector<float> previous_UWB_position{ 0, 0, 0 };
std::vector<float> UWB_x_storage{ 0 };
std::vector<float> UWB_y_storage{ 0 };
float UWB_mean_x;
float UWB_mean_y;
float UWB_x_back;
float UWB_y_back;
int strike_count;
float radius_limit = 0.25;

// Function to get dimensions of room from user input
double* getDimensions()
{
    double length;
    double width;

    std::cout << "Enter the length of the room in inches: ";
    std::cin >> length;


    std::cout << "Enter the width of the room in inches: ";
    std::cin >> width;

    std::cout << std::endl;

    double* result = new double[2];
    result[0] = (length * inch_to_meter);
    result[1] = (width * inch_to_meter);

    return result;
}

// Function to get anchor locations within room from user input
std::vector<Eigen::Vector3d> getAnchors()
{
    // Ask for dimensions of the anchor points in inches
    std::cout << "Enter the x, y, and z position of four anchor points in inches: " << std::endl;
    double x, y, z;

    // Convert each anchor point from inches to meters and replace variables
    std::cout << "Anchor Point 1: ";
    std::cin >> x >> y >> z;
    Eigen::Vector3d point_1(x * inch_to_meter, y * inch_to_meter, z * inch_to_meter);

    std::cout << "Anchor Point 2: ";
    std::cin >> x >> y >> z;
    Eigen::Vector3d point_2(x * inch_to_meter, y * inch_to_meter, z * inch_to_meter);

    std::cout << "Anchor Point 3: ";
    std::cin >> x >> y >> z;
    Eigen::Vector3d point_3(x * inch_to_meter, y * inch_to_meter, z * inch_to_meter);

    std::cout << "Anchor Point 4: ";
    std::cin >> x >> y >> z;
    Eigen::Vector3d point_4(x * inch_to_meter, y * inch_to_meter, z * inch_to_meter);

    std::vector<Eigen::Vector3d> result;
    result.push_back(point_1);
    result.push_back(point_2);
    result.push_back(point_3);
    result.push_back(point_4);

    return result;
}

// Function to read dimensions from a text file
void readDimensions(std::string filename, double& length, double& width, std::vector<Eigen::Vector3d>& anchor_positions)
{
    std::ifstream inputFile;
    inputFile.open(filename);

    if (inputFile.is_open())
    {
        std::string line;
        std::getline(inputFile, line);
        sscanf_s(line.c_str(), "Room Dimensions: %lf, %lf", &length, &width);

        for (int i = 0; i < 5; i++)
        {
            Eigen::Vector3d point;
            std::getline(inputFile, line);
            sscanf_s(line.c_str(), "Anchor %*d: %lf, %lf, %lf", &point[0], &point[1], &point[2]);
            anchor_positions.push_back(point);
        }

        std::cout << "Dimensions read from " << filename << std::endl;
    }
    else
    {
        std::cout << "Error: Unable to open file" << std::endl;
    }

    inputFile.close();
}


// Function to save dimensions to a text file
void saveDimensions(double length, double width, std::vector<Eigen::Vector3d> anchor_positions, std::string filename)
{
    std::ofstream outputFile;
    outputFile.open(filename);

    Eigen::Vector3d point_1 = anchor_positions[0];
    Eigen::Vector3d point_2 = anchor_positions[1];
    Eigen::Vector3d point_3 = anchor_positions[2];
    Eigen::Vector3d point_4 = anchor_positions[3];

    if (outputFile.is_open())
    {
        outputFile << "Room Dimensions: " << length << ", " << width << std::endl;
        outputFile << "Anchor 1: " << point_1[0] << ", " << point_1[1] << ", " << point_1[2] << std::endl;
        outputFile << "Anchor 2: " << point_2[0] << ", " << point_2[1] << ", " << point_2[2] << std::endl;
        outputFile << "Anchor 3: " << point_3[0] << ", " << point_3[1] << ", " << point_3[2] << std::endl;
        outputFile << "Anchor 4: " << point_4[0] << ", " << point_4[1] << ", " << point_4[2] << std::endl;
        std::cout << "Dimensions saved to " << filename << std::endl;
    }
    else
    {
        std::cout << "Error: Unable to open file" << std::endl;
    }

    outputFile.close();
}

// Function to draw the tag
void drawTag(SDL_Renderer* renderer, float x, float y)
{
    // Draw a red square at the tag's position
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_Rect rect = { (int)x, (int)y, 10, 10 };
    SDL_RenderFillRect(renderer, &rect);
}

// calculates the primes points of intersection
Eigen::Vector3d trilateration(float distance_1, float distance_2, float distance_3, Eigen::Vector3d point_1, Eigen::Vector3d point_2, Eigen::Vector3d point_3)
{

    // renames the anchor locations
    Eigen::Vector3d p1 = point_1;
    Eigen::Vector3d p2 = point_2;
    Eigen::Vector3d p3 = point_3;

    // renames the distances
    double d1 = distance_1;
    double d2 = distance_2;
    double d3 = distance_3;

    // defines locations and distances into matrices
    Eigen::MatrixXd A(3, 3);
    Eigen::VectorXd B(3);

    // performs matrix calculations to determine prime point
    A << 2 * (p2(0) - p1(0)), 2 * (p2(1) - p1(1)), 2 * (p2(2) - p1(2)),
        2 * (p3(0) - p1(0)), 2 * (p3(1) - p1(1)), 2 * (p3(2) - p1(2)),
        2 * (p3(0) - p2(0)), 2 * (p3(1) - p2(1)), 2 * (p3(2) - p2(2));

    B << d1 * d1 - d2 * d2 - p1(0) * p1(0) + p2(0) * p2(0) - p1(1) * p1(1) + p2(1) * p2(1) - p1(2) * p1(2) + p2(2) * p2(2),
        d1* d1 - d3 * d3 - p1(0) * p1(0) + p3(0) * p3(0) - p1(1) * p1(1) + p3(1) * p3(1) - p1(2) * p1(2) + p3(2) * p3(2),
        d2* d2 - d3 * d3 - p2(0) * p2(0) + p3(0) * p3(0) - p2(1) * p2(1) + p3(1) * p3(1) - p2(2) * p2(2) + p3(2) * p3(2);

    Eigen::Vector3d x = A.colPivHouseholderQr().solve(B);

    // returns prime point
    return x;
}

// Incoming Data Processing
std::vector<float> dataProcessing(std::string str)
{
    std::vector<float> data;

    size_t start = str.find("[") + 1;
    size_t end = str.find(",");

    while (str.find("]", start) != -1) {
        data.push_back(std::stof(str.substr(start, end - start)));
        start = end + 1;
        end = str.find(",", start);
        if (end == -1) {
            end = str.find("]", start);
        }
    }
    /*for (int i = 0; i < data.size(); i++) {
        std::cout << data[i] << ", ";
    }
    std::cout << std::endl;*/

    return data;
}

// Average of a vector
float calculate_average(std::vector<float> vector)
{
    float sum = 0.0;
    for (int i = 0; i < vector.size(); i++) {
        sum += vector[i];
    }
    return sum / vector.size();
}

// Stationary IMU calibration
void IMUcalibration(float UWB_x, float UWB_y, float& imuX, float& imuY, float& imuXv, float& imuYv, float& imuZv)
{
    // checks for exit conditions
    if (UWB_x == UWB_x_back and UWB_y == UWB_y_back) {
        UWB_x_back = UWB_x;
        UWB_y_back = UWB_y;
        return;
    }
    else if (strike_count == 3) {
        UWB_mean_x = 0;
        UWB_mean_y = 0;
        UWB_x_storage.clear();
        UWB_y_storage.clear();

        strike_count = 0;

        return;
    }
    else if (UWB_x_storage.size() == 0) {
        UWB_mean_x = UWB_x;
        UWB_mean_y = UWB_y;

        UWB_x_storage.push_back(UWB_x);
        UWB_y_storage.push_back(UWB_y);

        UWB_x_back = UWB_x;
        UWB_y_back = UWB_y;

        return;
    }
    else if (UWB_x_storage.size() == 8) {
        std::cout << "RESET RESET RESET IMU RESET RESET RESET" << std::endl;
        std::cout << "OLD IMU LOCATION: " << imuX << ", " << imuY << std::endl;

        imuX = UWB_mean_x;
        imuY = UWB_mean_y;
        imuXv = 0;
        imuYv = 0;
        imuZv = 0;

        std::cout << "NEW IMU LOCATION: " << imuX << ", " << imuY << std::endl;

        UWB_mean_x = 0;
        UWB_mean_y = 0;
        UWB_x_storage.clear();
        UWB_y_storage.clear();

        UWB_x_back = UWB_x;
        UWB_y_back = UWB_y;

        return;
    }

    float distance = sqrt(pow((UWB_x - UWB_mean_x), 2) + pow((UWB_y - UWB_mean_y), 2));


    if (distance > radius_limit) {
        /* std::cout << "OUT OF BOUNDS" << std::endl;*/
        strike_count += 1;

        UWB_x_back = UWB_x;
        UWB_y_back = UWB_y;

        return;
    }

    UWB_x_storage.push_back(UWB_x);
    UWB_y_storage.push_back(UWB_y);

    UWB_mean_x = calculate_average(UWB_x_storage);
    UWB_mean_y = calculate_average(UWB_y_storage);

    UWB_x_back = UWB_x;
    UWB_y_back = UWB_y;

}


// main code
int main()
{
    /********* Variable *********
     ******* Declarations *******/

     // Anchor Positions in x,y,z
    Eigen::Vector3d point_1;
    Eigen::Vector3d point_2;
    Eigen::Vector3d point_3;
    Eigen::Vector3d point_4;

    // Room Dimensions
    double length;
    double width;

    // Defines variables for location coordinates
    double x_location = 0;
    double y_location = 0;
    double z_location = 0;

    // Defines position and velocity variables
    float imuX = 0;
    float imuY = 0;
    float imuZ = 0;
    float vx = 0;
    float vy = 0;
    float vz = 0;

    /********* Room *********
     ******** Config *******/

    std::cout << "Do you want to set up a new room? (Y/N)? ";
    char choice_new_room;
    std::cin >> choice_new_room;

    if (choice_new_room == 'Y' || choice_new_room == 'y') {

        // Ask for room dimensions from user
        double* room_dimensions = getDimensions();
        length = room_dimensions[0];
        width = room_dimensions[1];

        // Ask for the anchor location from user
        std::vector<Eigen::Vector3d> anchor_positions = getAnchors();
        point_1 = anchor_positions[0];
        point_2 = anchor_positions[1];
        point_3 = anchor_positions[2];
        point_4 = anchor_positions[3];

        std::cout << "Do you want to save the room configuration (Y/N)?";
        char choice_save;
        std::cin >> choice_save;

        if (choice_save == 'Y' || choice_save == 'y')
        {
            std::cout << "Enter the name of the room: ";
            std::string save_filename;
            std::cin >> save_filename;

            // Check for existing file
            struct stat buffer;
            if (stat(save_filename.c_str(), &buffer) == 0) {

                std::cout << "File already exists, try again";

                return 0;
            }

            saveDimensions(length, width, anchor_positions, save_filename);

        }
    }
    else if (choice_new_room == 'N' || choice_new_room == 'n') {

        std::cout << "Enter the name of an existing room: ";
        std::string read_filename;
        std::cin >> read_filename;

        // Checks for existing file
        struct stat buffer;
        if (stat(read_filename.c_str(), &buffer) != 0) {

            std::cout << "File does not exist, try again";

            return 0;
        }

        std::vector<Eigen::Vector3d> anchor_position;
        readDimensions(read_filename, length, width, anchor_position);

        point_1 = anchor_position[0];
        point_2 = anchor_position[1];
        point_3 = anchor_position[2];
        point_4 = anchor_position[3];

    }
    else {

        std::cout << "Invalid Input";

        return 0;
    }

    // Convert dimensions from inches to meters and set screen size
    const double screen_scale = 150.0;
    double screen_width = width * screen_scale;
    double screen_height = length * screen_scale;

    // Print out results
    std::cout << "Room dimensions (m): " << length << " x " << width << std::endl;
    std::cout << "Screen size (pixels): " << screen_height << " x " << screen_width << std::endl;
    std::cout << "Anchor points (m): " << std::endl;
    std::cout << "  Point 1: " << point_1.transpose() << std::endl;
    std::cout << "  Point 2: " << point_2.transpose() << std::endl;
    std::cout << "  Point 3: " << point_3.transpose() << std::endl;
    std::cout << "  Point 4: " << point_4.transpose() << std::endl;

    // IMU Orientation Startup Delay
    //std::cout << "Wait 30 seconds for IMU Calibration" << std::endl;
    //int i = 30;
    //while (i != 0) {
    //    std::cout << i << " seconds" << std::endl;
    //    i -= 5;
    //    Sleep(5000);
    //}

    /***** Tracking File ****
     ******** Startup *******/

     // Open the file for writing
    std::ofstream outFile("Tracked Location", std::ios::app);

    // Check if the file was opened successfully
    if (!outFile) {
        std::cerr << "Error opening file." << std::endl;
        return 1;
    }

    // Wifi Startup Check
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        std::cout << "WSAStartup failed with error: " << iResult << std::endl;
        return 1;
    }

    /********* UWB *********
     ******** Socket *******/

     // check if UWB socket connection is good
    SOCKET sock_1 = socket(AF_INET, SOCK_DGRAM, 0);
    u_long mode = 1;
    int result = ioctlsocket(sock_1, FIONBIO, &mode);
    if (result != NO_ERROR) {
        std::cerr << "Error setting socket to non-blocking mode: " << result << std::endl;
    }
    if (sock_1 == INVALID_SOCKET) {
        std::cout << "socket failed with error: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    // defines port and ip address for UWB
    sockaddr_in serverAddr_1;
    serverAddr_1.sin_family = AF_INET;
    serverAddr_1.sin_port = htons(1234);
    serverAddr_1.sin_addr.s_addr = INADDR_ANY;

    // checks if port binded properly
    if (bind(sock_1, (sockaddr*)&serverAddr_1, sizeof(serverAddr_1)) == SOCKET_ERROR) {
        std::cout << "bind failed with error: " << WSAGetLastError() << std::endl;
        closesocket(sock_1);
        WSACleanup();
        return 1;
    }

    // defines buffer size and client address
    char buffer[1024];
    int recvLen_1;
    sockaddr_in clientAddr_1;
    int clientAddrLen_1 = sizeof(clientAddr_1);

    /********* IMU *********
     ******** Socket *******/

     // checks if IMU socket connection is good
    SOCKET sock_2 = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock_2 == INVALID_SOCKET) {
        std::cout << "socket failed with error: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    // defines port and ip address for IMU
    sockaddr_in serverAddr_2;
    serverAddr_2.sin_family = AF_INET;
    serverAddr_2.sin_port = htons(1235);
    serverAddr_2.sin_addr.s_addr = INADDR_ANY;

    // checks if port binded properly
    if (bind(sock_2, (sockaddr*)&serverAddr_2, sizeof(serverAddr_2)) == SOCKET_ERROR) {
        std::cout << "bind failed with error: " << WSAGetLastError() << std::endl;
        closesocket(sock_2);
        WSACleanup();
        return 1;
    }

    // defines buffer size and client address
    int recvLen_2;
    sockaddr_in clientAddr_2;
    int clientAddrLen_2 = sizeof(clientAddr_2);

    /********** SDL *********
     ******** Startup *******/

     // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cout << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Create a window and renderer
    SDL_Window* window = SDL_CreateWindow("Object Movement", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, screen_height, screen_width, SDL_WINDOW_SHOWN);

    // Create a renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    // Set the size and color of the object
    int object_width = 7;
    int object_height = 7;
    SDL_Color object_color = { 210, 0 , 0, 255 };

    // data collection and display loop
    bool quit = false;
    while (!quit) {

        // reads incoming string into data
        std::vector<float> UWB_data;
        std::vector<float> IMU_data;
        double distance_1;
        float distance_2;
        float distance_3;
        float distance_4;
        float roll = 0;
        float pitch = 0;
        float yaw = 0;
        float rawAx = 0;
        float rawAy = 0;
        float rawAz = 0;
        float dt = 0;

        // pulls UWB data from first port
        recvLen_1 = recvfrom(sock_1, buffer, sizeof(buffer), 0, (sockaddr*)&clientAddr_1, &clientAddrLen_1);

        // checks if data is received on port
        if (recvLen_1 > 0) {
            std::string str_1(buffer, recvLen_1);
            UWB_data = dataProcessing(str_1);
            distance_1 = UWB_data[0];
            distance_2 = UWB_data[1];
            distance_3 = UWB_data[2];
            distance_4 = UWB_data[3];
        }


        // pulls IMU data from second port
        recvLen_2 = recvfrom(sock_2, buffer, sizeof(buffer), 0, (sockaddr*)&clientAddr_2, &clientAddrLen_2);

        if (recvLen_2 <= 0) {
            return 1;
        }

        std::string str_2(buffer, recvLen_2);
        IMU_data = dataProcessing(str_2);
        roll = IMU_data[0]; // degrees
        pitch = IMU_data[1]; // degrees
        yaw = IMU_data[2]; // degrees
        rawAx = IMU_data[3]; // mg
        rawAy = IMU_data[4]; // mg
        rawAz = IMU_data[5]; // mg
        dt = IMU_data[6] / 1000000; // s


        /***************/
        /***** UWB *****/
        /***************/

        // checks if UWB received data this pass
        if (recvLen_1 <= 0) {
            x_location = previous_UWB_position[0];
            y_location = previous_UWB_position[1];
        }
        else if (distance_1 != 0 and distance_2 != 0 and distance_3 != 0 and distance_4 != 0) {

            /********QUADLATERATION********/

            // calculates the prime points to determine the location given overestimates
            Eigen::Vector3d point_1_prime = trilateration(distance_1, distance_2, distance_3, point_1, point_2, point_3);
            Eigen::Vector3d point_2_prime = trilateration(distance_1, distance_2, distance_4, point_1, point_2, point_4);
            Eigen::Vector3d point_3_prime = trilateration(distance_4, distance_2, distance_3, point_4, point_2, point_3);
            Eigen::Vector3d point_4_prime = trilateration(distance_1, distance_4, distance_3, point_1, point_4, point_3);

            // finds location of tag in (x,y,z)
            x_location = (point_1_prime(0) + point_2_prime(0) + point_3_prime(0) + point_4_prime(0)) / 4;
            y_location = (point_1_prime(1) + point_2_prime(1) + point_3_prime(1) + point_4_prime(1)) / 4;
            z_location = (point_1_prime(2) + point_2_prime(2) + point_3_prime(2) + point_4_prime(2)) / 4;
        }
        else {

            /********TRILATERATION********/

            /*std::cout << "TRILATERATION ATTEMPT" << std::endl;*/

            if (distance_1 == 0) {
                Eigen::Vector3d prime = trilateration(distance_4, distance_2, distance_3, point_4, point_2, point_3);

                x_location = prime(0);
                y_location = prime(1);
                z_location = prime(2);
            }
            else if (distance_2 == 0) {
                Eigen::Vector3d prime = trilateration(distance_1, distance_4, distance_3, point_1, point_4, point_3);

                x_location = prime(0);
                y_location = prime(1);
                z_location = prime(2);
            }
            else if (distance_3 == 0) {
                Eigen::Vector3d prime = trilateration(distance_1, distance_2, distance_4, point_1, point_2, point_4);

                x_location = prime(0);
                y_location = prime(1);
                z_location = prime(2);
            }
            else if (distance_4 == 0) {
                Eigen::Vector3d prime = trilateration(distance_1, distance_2, distance_3, point_1, point_2, point_3);

                x_location = prime(0);
                y_location = prime(1);
                z_location = prime(2);
            }
            else {
                std::cout << "ERROR, ALL DATA ACQUIRED" << std::endl;
            }
        }

        // writes the location data to the console
        /*std::cout << "x: " << x_location << ", y: " << y_location << ", z: " << z_location << std::endl;*/

        // Handle events (such as window close)
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                quit = true;
            }
        }

        // Storing current position
        if (x_location - previous_UWB_position[0] != 0) {
            previous_UWB_position[0] = x_location;
            previous_UWB_position[1] = y_location;
        }

        /***************/
        /***** IMU *****/
        /***************/

        double PI = M_PI;

        // calculating orientation
        float zerodir = 180.0; // compass direction (degrees from North) where yaw is 0
        float compass = zerodir - yaw;
        if (compass < 0) {
            compass += 360;
        }
        float angle1 = 90 - roll; // tilt up = positive, tilt down = negative
        float angle2 = -pitch; // right = positive, left = negative

        float room_orientation = 150; // compass direction of positive x-axis of the room
        float theta = room_orientation - compass;
        if (theta < 0) {
            theta += 360;
        }
        float rtheta = theta * PI / 180; // convert from 
        float rpitch = angle2 * PI / 180;

        /*std::cout << "  Compass: " << compass << std::endl;
        std::cout << "  Angle 1: " << angle1 << std::endl;
        std::cout << "  Angle 2: " << angle2 << std::endl;
        std::cout << "  dt: " << dt << std::endl;*/

        // calculating gravity components (mg)
        float gx = 1000 * sin(pitch * PI / 180);
        float gy = 1000 * cos(pitch * PI / 180) * sin(roll * PI / 180);
        float gz = 1000 * cos(pitch * PI / 180) * cos(roll * PI / 180);

        // calculating acceleration without gravity, converting from mg to m/s^2, & switching coordinate system
        float ax = (rawAx - gx) / 9807;
        float ay = -(rawAz - gz) / 9807;
        float az = (rawAy - gy) / 9807;

        // update velocity (m/s)
        vx += ax * dt;
        vy += ay * dt;
        vz += az * dt;

        // update position (m)
        imuX += (cos(rtheta) * vy + cos(rtheta - PI / 2) * vx) * cos(rpitch) * dt;
        imuY += (sin(rtheta) * vy + sin(rtheta - PI / 2) * vx) * cos(rpitch) * dt;

        IMUcalibration(x_location, y_location, imuX, imuY, vx, vy, vz);

        std::cout << "  Ax: " << ax << "  Ay: " << ay << "  Az: " << az << std::endl;
        std::cout << "  Vx: " << vx << "  Vy: " << vy << "  Vz: " << vz << std::endl;
        std::cout << "  X: " << imuX << "  Y: " << imuY << std::endl;

        /***************/
        /***** EKF *****/
        /***************/

        // Simulated (true) system state
        State x;
        x.setZero();

        // Control input
        Control u;
        // System
        SystemModel sys;

        // Measurement models
        // Set position landmarks at (0, 0) and (30, 75)
        PositionModel pm(0, 0);

        // Some filters for estimation
        // Pure predictor without measurement updates
        Kalman::ExtendedKalmanFilter<State> predictor;
        // Extended Kalman Filter
        Kalman::ExtendedKalmanFilter<State> ekf;
        // Unscented Kalman Filter
        Kalman::UnscentedKalmanFilter<State> ukf(1);

        // Init filters with true system state
        predictor.init(x);
        ekf.init(x);
        ukf.init(x);

        // Sets control input
        u.vx() = vx;
        u.vy() = vy;
        u.dt() = dt;

        // System model
        x.theta() = theta * PI / 180;
        x.pitch() = rpitch;
        x = sys.f(x, u);

        // Predict state for current time-step using the filters
        auto x_pred = predictor.predict(sys, u);
        auto x_ekf = ekf.predict(sys, u);
        auto x_ukf = ukf.predict(sys, u);

        // Position measurement
        // We can measure the position every 10th step
        PositionMeasurement position = pm.h(x); // pm.h(x)

        // Update EKF
        x_ekf = ekf.update(pm, position);

        // Update UKF
        x_ukf = ukf.update(pm, position);

        // Print to stdout as csv format
        //std::cout << x.x() << "," << x.y() << "," << x.theta() << " | "
        //    << x_pred.x() << "," << x_pred.y() << "," << x_pred.theta() << " | "
        //    << x_ekf.x() << "," << x_ekf.y() << "," << x_ekf.theta() << " | "
        //    << x_ukf.x() << "," << x_ukf.y() << "," << x_ukf.theta()
        //    << std::endl;

        // Write location data to file
        outFile << "" << x_location << ", " << y_location << ", " << theta << ", " << ax << ", " << ay << ", " << az << ", " << x.x() << ", " << x.y() << std::endl;

        /***************/
        /***** Vis *****/
        /***************/

        // Clear the screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        //x_location = x.x();
        //y_location = x.y();

        // Convert the object position from meters to pixels
        int x_location_pixel = static_cast<int>(x_location * screen_scale);
        int y_location_pixel = static_cast<int>(y_location * screen_scale);
        int imux_location_pixel = static_cast<int>(imuX * screen_scale);
        int imuy_location_pixel = static_cast<int>(imuY * screen_scale);

        // Calculate line for orientation, currently assuming that North is in the positive x direction
        int length = 75;
        int x_top = x_location_pixel + length * cos(theta * PI / 180);
        int y_top = y_location_pixel + length * sin(theta * PI / 180);
        int x_side_left = x_location_pixel + length * cos(theta * PI / 180 - PI / 4);
        int y_side_left = y_location_pixel + length * sin(theta * PI / 180 - PI / 4);
        int x_side_right = x_location_pixel + length * cos(theta * PI / 180 + PI / 4);
        int y_side_right = y_location_pixel + length * sin(theta * PI / 180 + PI / 4);

        // Draw the object
        SDL_Rect object_rect = { x_location_pixel - (8 / 2), y_location_pixel - (8 / 2), 8, 8 };
        SDL_SetRenderDrawColor(renderer, object_color.r, object_color.g, object_color.b, object_color.a);
        SDL_RenderFillRect(renderer, &object_rect);

        // Draw line for orientation
        SDL_RenderDrawLine(renderer, x_location_pixel, y_location_pixel, x_side_left, y_side_left);
        SDL_RenderDrawLine(renderer, x_location_pixel, y_location_pixel, x_side_right, y_side_right);
        SDL_RenderDrawLine(renderer, x_side_left, y_side_left, x_side_right, y_side_right);

        // Draw IMU position
        SDL_Rect object_rect2 = { imux_location_pixel, imuy_location_pixel, 7, 7 };
        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
        SDL_RenderFillRect(renderer, &object_rect2);

        // Presents the rendereer to the screen
        SDL_RenderPresent(renderer);

    }

    // Close the file
    outFile.close();

    // Clean up resources
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
