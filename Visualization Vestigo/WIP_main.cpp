#include <iostream>
#include <Eigen/Dense>
#include <string>
#include <cstring>
#include <ws2tcpip.h>
#include <cmath>
#include <algorithm>
#define BUF_SIZE 1024
#define WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#pragma comment(lib, "ws2_32.lib")
#include <WinSock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#include <SDL.h>
#include <fstream>
#include <vector>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <thread>

#undef main


using std::cout;
using std::cin;
using std::endl;

// Definitions
const float inch_to_meter = 0.0254;
std::vector<Eigen::Vector3d> UWB_previous = { 
    {0.0, 0.0, 0.0} , {0.0, 0.0, 0.0} , {0.0, 0.0, 0.0} , {0.0, 0.0, 0.0} 
};
std::vector<std::vector<float>> previous_IMU_data = {
    {0.0, 0.0, 0.0, 0.0} , {0.0, 0.0, 0.0, 0.0} , {0.0, 0.0, 0.0, 0.0} , {0.0, 0.0, 0.0, 0.0}
};

/***********************************
*********** LM ALGORITHM ***********
***********************************/


// Function to compute the residuals
Eigen::VectorXd computeResiduals(const std::vector<Eigen::Vector3d>& points, const std::vector<float>& distances, const Eigen::Vector3d& estimate)
{
    Eigen::VectorXd residuals(points.size());

    for (size_t i = 0; i < points.size(); ++i)
    {
        residuals(i) = (estimate - points[i]).norm() - distances[i];
    }

    return residuals;
}

// Levenberg-Marquardt algorithm for trilateration
Eigen::Vector3d multilateration(const std::vector<Eigen::Vector3d>& points, const std::vector<float>& distances, const Eigen::Vector3d& initial_guess)
{
    // Use the initial guess
    Eigen::Vector3d estimate = initial_guess;

    // Levenberg-Marquardt parameters
    double lambda = 0.001;
    double updateNorm;
    int maxIterations = 1000;  // Maximum number of iterations
    int iterations = 0;  // Current iteration count

    // Filter out points and distances where distance is zero
    std::vector<Eigen::Vector3d> filtered_points;
    std::vector<float> filtered_distances;
    for (size_t i = 0; i < distances.size(); ++i) {
        if (distances[i] >= 0.1) {
            filtered_points.push_back(points[i]);
            filtered_distances.push_back(distances[i]);
            /*std::cout << "Distance " << i + 1 << " Filtered: " << distances[i] << std::endl;
            std::cout << "Point  " << i + 1 << " Filtered: " << points[i] << std::endl;*/
        }
        else {
            std::cout << "SKIPPED: " << i + 1 << std::endl;
        }
    }

    do
    {
        Eigen::VectorXd residuals = computeResiduals(filtered_points, filtered_distances, estimate);

        // Compute the Jacobian matrix
        Eigen::MatrixXd J(residuals.size(), 3);
        for (size_t i = 0; i < filtered_points.size(); ++i)
        {
            Eigen::Vector3d diff = estimate - filtered_points[i];
            double dist = diff.norm();
            J.row(i) = diff / dist;
        }

        // Levenberg-Marquardt update
        Eigen::MatrixXd A = J.transpose() * J;
        A.diagonal() += lambda * A.diagonal();
        Eigen::VectorXd g = J.transpose() * residuals;
        Eigen::VectorXd delta = A.ldlt().solve(-g);

        // Check if the cost function has decreased
        Eigen::Vector3d new_estimate = estimate + delta;
        if (computeResiduals(filtered_points, filtered_distances, new_estimate).squaredNorm() < residuals.squaredNorm())
        {
            // If the cost function has decreased, accept the new estimate and decrease lambda
            estimate = new_estimate;
            lambda /= 10;
        }
        else
        {
            // If the cost function has increased, reject the new estimate and increase lambda
            lambda *= 10;
        }

        updateNorm = delta.norm();

        // Increment the iteration count
        iterations++;
    } while (updateNorm > 1e-7 && iterations < maxIterations);

    if (iterations == maxIterations)
    {
        throw std::runtime_error("Levenberg-Marquardt algorithm did not converge");
    }

    std::cout << "Iterations: " << iterations << std::endl;
    return estimate;
}


/*********************************************
*********** ROOM AND ANCHOR CONFIG ***********
*********************************************/

// Function to get dimensions of room from user input
float* getDimensions()
{
    float length;
    float width;

    std::cout << "Room Setup Menu" << std::endl;
    std::cout << "Input the length of the room in inches and press enter: ";
    std::cin >> length;


    std::cout << "Input the width of the room in inches and press enter: ";
    std::cin >> width;

    std::cout << std::endl;

    float* result = new float[2];
    result[0] = (length * inch_to_meter);
    result[1] = (width * inch_to_meter);

    return result;
}


// Function to get anchor locations within room from user input
std::vector<Eigen::Vector3d> getAnchors()
{
    // Ask for dimensions of the anchor points in inches
    std::cout << "Input the x, y, and z position of four anchor points in inches with spaces in between each and press enter: " << std::endl;
    float x, y, z;

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

    std::cout << "Anchor Point 5: ";
    std::cin >> x >> y >> z;
    Eigen::Vector3d point_5(x * inch_to_meter, y * inch_to_meter, z * inch_to_meter);

    std::cout << "Anchor Point 6: ";
    std::cin >> x >> y >> z;
    Eigen::Vector3d point_6(x * inch_to_meter, y * inch_to_meter, z * inch_to_meter);

    std::cout << "Anchor Point 7: ";
    std::cin >> x >> y >> z;
    Eigen::Vector3d point_7(x * inch_to_meter, y * inch_to_meter, z * inch_to_meter);

    std::cout << "Anchor Point 8: ";
    std::cin >> x >> y >> z;
    Eigen::Vector3d point_8(x * inch_to_meter, y * inch_to_meter, z * inch_to_meter);

    std::cout << "Anchor Point 9: ";
    std::cin >> x >> y >> z;
    Eigen::Vector3d point_9(x * inch_to_meter, y * inch_to_meter, z * inch_to_meter);

    std::cout << "Anchor Point 10: ";
    std::cin >> x >> y >> z;
    Eigen::Vector3d point_10(x * inch_to_meter, y * inch_to_meter, z * inch_to_meter);

    std::cout << "Anchor Point 11: ";
    std::cin >> x >> y >> z;
    Eigen::Vector3d point_11(x * inch_to_meter, y * inch_to_meter, z * inch_to_meter);

    std::cout << "Anchor Point 12: ";
    std::cin >> x >> y >> z;
    Eigen::Vector3d point_12(x * inch_to_meter, y * inch_to_meter, z * inch_to_meter);

    std::vector<Eigen::Vector3d> result;
    result.push_back(point_1);
    result.push_back(point_2);
    result.push_back(point_3);
    result.push_back(point_4);
    result.push_back(point_5);
    result.push_back(point_6);
    result.push_back(point_7);
    result.push_back(point_8);
    result.push_back(point_9);
    result.push_back(point_10);
    result.push_back(point_11);
    result.push_back(point_12);

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

        for (int i = 0; i < 7; i++)
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
void saveDimensions(float length, float width, std::vector<Eigen::Vector3d> anchor_positions, std::string filename)
{
    std::ofstream outputFile;
    outputFile.open(filename);

    Eigen::Vector3d point_1 = anchor_positions[0];
    Eigen::Vector3d point_2 = anchor_positions[1];
    Eigen::Vector3d point_3 = anchor_positions[2];
    Eigen::Vector3d point_4 = anchor_positions[3];
    Eigen::Vector3d point_5 = anchor_positions[4];
    Eigen::Vector3d point_6 = anchor_positions[5];
    Eigen::Vector3d point_7 = anchor_positions[6];
    Eigen::Vector3d point_8 = anchor_positions[7];
    Eigen::Vector3d point_9 = anchor_positions[8];
    Eigen::Vector3d point_10 = anchor_positions[9];
    Eigen::Vector3d point_11 = anchor_positions[10];
    Eigen::Vector3d point_12 = anchor_positions[11];

    if (outputFile.is_open())
    {
        outputFile << "Room Dimensions: " << length << ", " << width << std::endl;
        outputFile << "Anchor 1: " << point_1[0] << ", " << point_1[1] << ", " << point_1[2] << std::endl;
        outputFile << "Anchor 2: " << point_2[0] << ", " << point_2[1] << ", " << point_2[2] << std::endl;
        outputFile << "Anchor 3: " << point_3[0] << ", " << point_3[1] << ", " << point_3[2] << std::endl;
        outputFile << "Anchor 4: " << point_4[0] << ", " << point_4[1] << ", " << point_4[2] << std::endl;
        outputFile << "Anchor 5: " << point_5[0] << ", " << point_5[1] << ", " << point_5[2] << std::endl;
        outputFile << "Anchor 6: " << point_6[0] << ", " << point_6[1] << ", " << point_6[2] << std::endl;
        outputFile << "Anchor 7: " << point_7[0] << ", " << point_7[1] << ", " << point_7[2] << std::endl;
        outputFile << "Anchor 8: " << point_8[0] << ", " << point_8[1] << ", " << point_8[2] << std::endl;
        outputFile << "Anchor 9: " << point_9[0] << ", " << point_9[1] << ", " << point_9[2] << std::endl;
        outputFile << "Anchor 10: " << point_10[0] << ", " << point_10[1] << ", " << point_10[2] << std::endl;
        outputFile << "Anchor 11: " << point_11[0] << ", " << point_11[1] << ", " << point_11[2] << std::endl;
        outputFile << "Anchor 12: " << point_12[0] << ", " << point_12[1] << ", " << point_12[2] << std::endl;
        std::cout << "Dimensions saved to " << filename << std::endl;
    }
    else
    {
        std::cout << "Error: Unable to open file" << std::endl;
    }

    outputFile.close();
}

/*****************************************
*********** GENERAL FUNCTIONS  ***********
*****************************************/

// Function to draw the tag
//void drawTag(SDL_Renderer* renderer, float x, float y)
//{
//    // Draw a red square at the tag's position
//    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
//    SDL_Rect rect = { (int)x, (int)y, 10, 10 };
//    SDL_RenderFillRect(renderer, &rect);
//}

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

    return data;
}

// Average of a Vector
float calculate_average(std::vector<float> vector)
{
    float sum = 0.0;
    for (int i = 0; i < vector.size(); i++) {
        sum += vector[i];
    }
    return sum / vector.size();
}

/***********************************
*********** MAIN PROGRAM ***********
***********************************/

int main()
{
    /***********************************
    *********** ROOM CONFIG ***********
    ***********************************/

    // Anchor Positions in x,y,z
    Eigen::Vector3d point_1;
    Eigen::Vector3d point_2;
    Eigen::Vector3d point_3;
    Eigen::Vector3d point_4;
    Eigen::Vector3d point_5;
    Eigen::Vector3d point_6;
    Eigen::Vector3d point_7;
    Eigen::Vector3d point_8;
    Eigen::Vector3d point_9;
    Eigen::Vector3d point_10;
    Eigen::Vector3d point_11;
    Eigen::Vector3d point_12;

    // Room Dimensions
    double length;
    double width;

    std::cout << "Tracking Startup Menu:" << std::endl;
    std::cout << "Do you want to set up a new room? (Y/N)? ";
    char choice_new_room;
    std::cin >> choice_new_room;

    if (choice_new_room == 'Y' || choice_new_room == 'y') {

        // Ask for room dimensions from user
        float* room_dimensions = getDimensions();
        length = room_dimensions[0];
        width = room_dimensions[1];

        // Ask for the anchor location from user
        std::vector<Eigen::Vector3d> anchor_positions = getAnchors();
        point_1 = anchor_positions[0];
        point_2 = anchor_positions[1];
        point_3 = anchor_positions[2];
        point_4 = anchor_positions[3];
        point_5 = anchor_positions[4];
        point_6 = anchor_positions[5];
        point_7 = anchor_positions[6];
        point_8 = anchor_positions[7];
        point_9 = anchor_positions[8];
        point_10 = anchor_positions[9];
        point_11 = anchor_positions[10];
        point_12 = anchor_positions[11];

        std::cout << "Do you want to save the new room configuration (Y/N)?";
        char choice_save;
        std::cin >> choice_save;

        if (choice_save == 'Y' || choice_save == 'y')
        {
            std::cout << "Enter the name of the new room: ";
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
    else if (choice_new_room == 'n' || choice_new_room == 'N') {

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
        point_5 = anchor_position[4];
        point_6 = anchor_position[5];
        point_7 = anchor_position[6];
        point_8 = anchor_position[7];
        point_9 = anchor_position[8];
        point_10 = anchor_position[9];
        point_11 = anchor_position[10];
        point_12 = anchor_position[11];

    }
    else {

        std::cout << "Invalid Input";

        return 0;
    }

    // Convert dimensions from inches to meters and set screen size
    const float screen_scale = 150.0;
    float screen_width = width * screen_scale;
    float screen_height = length * screen_scale;

    // Print out results
    std::cout << "Room dimensions (m): " << length << " x " << width << std::endl;
    std::cout << "Screen size (pixels): " << screen_height << " x " << screen_width << std::endl;
    std::cout << "Anchor points (m): " << std::endl;
    std::cout << "  Point 1: " << point_1.transpose() << std::endl;
    std::cout << "  Point 2: " << point_2.transpose() << std::endl;
    std::cout << "  Point 3: " << point_3.transpose() << std::endl;
    std::cout << "  Point 4: " << point_4.transpose() << std::endl;
    std::cout << "  Point 5: " << point_5.transpose() << std::endl;
    std::cout << "  Point 6: " << point_6.transpose() << std::endl;
    std::cout << "  Point 7: " << point_7.transpose() << std::endl;
    std::cout << "  Point 8: " << point_8.transpose() << std::endl;
    std::cout << "  Point 9: " << point_9.transpose() << std::endl;
    std::cout << "  Point 10: " << point_10.transpose() << std::endl;
    std::cout << "  Point 11: " << point_11.transpose() << std::endl;
    std::cout << "  Point 12: " << point_12.transpose() << std::endl;

    // IMU Orientation Startup Delay
    /*std::cout << "Wait 30 seconds for IMU Calibration" << std::endl;
    int i = 30;
    while (i != 0) {
        std::cout << i << " seconds" << std::endl;
        i -= 5;
        Sleep(5000);
    }*/

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


    /************************************
    *********** SOCKETS SETUP ***********
    ************************************/

    const int num_ports = 4;
    int ports[num_ports] = { 1234, 1235, 1236, 1237 };    
    SOCKET socks[num_ports];
    sockaddr_in serverAddrs[num_ports];
    int recvLens[num_ports];
    sockaddr_in clientAddrs[num_ports];
    int clientAddrLens[num_ports] = { sizeof(sockaddr_in), sizeof(sockaddr_in), sizeof(sockaddr_in), sizeof(sockaddr_in) };

    for (int i = 0; i < num_ports; ++i) {
        // check if UWB socket connection is good
        socks[i] = socket(AF_INET, SOCK_DGRAM, 0);
        u_long mode = 1;
        int result = ioctlsocket(socks[i], FIONBIO, &mode);
        if (result != NO_ERROR) {
            std::cerr << "Error setting socket to non-blocking mode: " << result << std::endl;
        }
        if (socks[i] == INVALID_SOCKET) {
            std::cout << "socket failed with error: " << WSAGetLastError() << std::endl;
            WSACleanup();
            return 1;
        }

        // defines port and ip address for UWB
        serverAddrs[i].sin_family = AF_INET;
        serverAddrs[i].sin_port = htons(ports[i]);
        serverAddrs[i].sin_addr.s_addr = INADDR_ANY;

        // checks if port binded properly
        if (bind(socks[i], (sockaddr*)&serverAddrs[i], sizeof(serverAddrs[i])) == SOCKET_ERROR) {
            std::cout << "bind failed with error: " << WSAGetLastError() << std::endl;
            closesocket(socks[i]);
            WSACleanup();
            return 1;
        }
    }

    // defines buffer size and client address
    char buffer[1024];


    /***********************************
    *********** SDL2 STARTUP ***********
    ***********************************/

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
    SDL_Color colors[] = {
    { 255, 0, 0, 255 },   // Red
    { 0, 255, 0, 255 },   // Green
    { 0, 0, 255, 255 },   // Blue
    { 255, 255, 0, 255 }  // Yellow
    };

    /**************************************
    *********** DATA PROCESSING ***********
    **************************************/

    bool quit = false;
    while (!quit) {

        // get the current timestamp
        auto now = std::chrono::system_clock::now();

        // convert the timestamp to a time_t object
        std::time_t timestamp = std::chrono::system_clock::to_time_t(now);

        // set the desired timezone
        std::tm timeinfo;
        localtime_s(&timeinfo, &timestamp);

        int hours = timeinfo.tm_hour;
        int minutes = timeinfo.tm_min;
        int seconds = timeinfo.tm_sec;

        // Defines variables for location coordinates
        float UWB_x[num_ports]; 
        float UWB_y[num_ports];
        float UWB_z[num_ports];

        float roll;
        float pitch;
        float yaw;
        float dt;

        std::vector<float> esp32_data;
        std::vector<float> distances;

        for (int i = 0; i < num_ports; ++i)
        {   
            /*std::cout << "Port Num: " << i << std::endl;*/

            // pulls UWB data from first port
            recvLens[i] = recvfrom(socks[i], buffer, sizeof(buffer), 0, (sockaddr*)&clientAddrs[i], &clientAddrLens[i]);

            // checks if data is received on port
            if (recvLens[i] <= 0) {
                continue;
            }

            

            std::string data_str(buffer, recvLens[i]);
            esp32_data = dataProcessing(data_str);

            for (int j = 0; j < 13; ++j) {
                distances.push_back(esp32_data[j]);
            }

            //roll[i] = esp32_data[12]; // degrees
            //pitch[i] = -esp32_data[13]; // degrees
            //yaw[i] = esp32_data[14]; // degrees
            //dt[i] = esp32_data[15] / 1000000; // s

            roll = 0; 
            pitch = 0; 
            yaw = 0; 
            dt = 0; 


            /*************************************
            *********** UWB PROCESSING ***********
            *************************************/

            
            // Define the anchor points
            std::vector<Eigen::Vector3d> points = {
                Eigen::Vector3d(point_1), Eigen::Vector3d(point_2), Eigen::Vector3d(point_3),
                Eigen::Vector3d(point_4), Eigen::Vector3d(point_5), Eigen::Vector3d(point_6),
                Eigen::Vector3d(point_7), Eigen::Vector3d(point_8), Eigen::Vector3d(point_9),
                Eigen::Vector3d(point_10), Eigen::Vector3d(point_11), Eigen::Vector3d(point_12)
            };


            // Call the multilateration function
            Eigen::Vector3d result;
            try {
                result = multilateration(points, distances, UWB_previous[i]);
                // Update the previous position
                UWB_previous[i] = result;
            }
            catch (std::exception& e) {
                // Use the previous position if a unique solution is not found
                result = UWB_previous[i];
            }

            // Checks for Outliers
            Eigen::Vector3d UWB_current = { result[0] , result[1] , result[2] };
            if (abs(UWB_current.norm() - UWB_previous[i].norm()) < 0.25) {
                UWB_x[i] = UWB_current[0];
                UWB_y[i] = UWB_current[1];
                UWB_z[i] = UWB_current[2];
            }
            else {
                UWB_x[i] = UWB_previous[i][0];
                UWB_y[i] = UWB_previous[i][1];
                UWB_z[i] = UWB_previous[i][2];
                std::cout << "PREVIOUS USED" << std::endl;
            }
            

            // writes the location data to the console
            std::cout << "x: " << UWB_x[i] << ", y: " << UWB_y[i] << ", z: " << UWB_z[i] << ", Time: " << hours << ":" << minutes << ":" << seconds << std::endl;

            // Handle events (such as window close)
            SDL_Event event;
            while (SDL_PollEvent(&event))
            {
                if (event.type == SDL_QUIT)
                {
                    quit = true;
                }
            }


            /*************************************
            *********** IMU PROCESSING ***********
            *************************************/

            double PI = M_PI;

            // calculating orientation
            float zerodir = 180.0; // compass direction (degrees from North) where yaw is 0
            float compass = zerodir - yaw;
            if (compass < 0) {
                compass += 360;
            }

            float room_orientation = 20; // compass direction of positive x-axis of the room
            float theta = compass - room_orientation;
            if (theta < 0) {
                theta += 360;
            }

            float rtheta = theta * PI / 180;


            

            // Write location data to file
            outFile << UWB_x[i] << ", " << UWB_y[i] << ", " << UWB_z[i] << ", " << theta << ", " << hours << ", " << minutes << ", " << seconds << std::endl;

            /***************************************
            *********** REALTIME DISPLAY ***********
            ***************************************/

            // Clear the screen
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);

            for (int i = 0; i < num_ports; ++i)
            {
                if (i != 0) {
                    continue;
                }

                // Convert the object position from meters to pixels
                int UWB_x_pixel = static_cast<int>(UWB_x[i] * screen_scale);
                int UWB_y_pixel = static_cast<int>(UWB_y[i] * screen_scale);

                // Calculate line for orientation, currently assuming that North is in the positive x direction
                int draw_length = 75;
                int x_top = UWB_x_pixel + draw_length * cos(rtheta);
                int y_top = UWB_y_pixel + draw_length * sin(rtheta);
                int x_side_left = UWB_x_pixel + draw_length * cos(rtheta - PI / 4);
                int y_side_left = UWB_y_pixel + draw_length * sin(rtheta - PI / 4);
                int x_side_right = UWB_x_pixel + draw_length * cos(rtheta + PI / 4);
                int y_side_right = UWB_y_pixel + draw_length * sin(rtheta + PI / 4);

                // Draw the object
                SDL_Rect object_rect = { UWB_x_pixel - 4, UWB_y_pixel - 4, 8, 8 };
                SDL_SetRenderDrawColor(renderer, colors[i].r, colors[i].g, colors[i].b, colors[i].a);
                SDL_RenderFillRect(renderer, &object_rect);

                // Draw line for orientation
                SDL_RenderDrawLine(renderer, UWB_x_pixel, UWB_y_pixel, x_side_left, y_side_left);
                SDL_RenderDrawLine(renderer, UWB_x_pixel, UWB_y_pixel, x_side_right, y_side_right);
                SDL_RenderDrawLine(renderer, x_side_left, y_side_left, x_side_right, y_side_right);
            }

            // Presents the rendereer to the screen
            SDL_RenderPresent(renderer);
        }
    
    
  
    }

    // Close the file
    outFile.close();

    // Clean up resources
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
