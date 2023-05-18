#include <iostream>
#include <Eigen/Dense>
#include <unsupported/Eigen/NonLinearOptimization>
#include <unsupported/Eigen/NumericalDiff>
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
#undef main
#include <fstream>
#include <vector>
#include <chrono>
#include <ctime>
#include <iomanip>


using std::cout;
using std::cin;
using std::endl;

// Definitions
const float inch_to_meter = 0.0254;
Eigen::Vector3d previous_UWB_position(0.0, 0.0, 0.0);

/*********************************
*********** GPT IS GOD ***********
*********************************/


// Function to compute the residuals
Eigen::VectorXd computeResiduals(const std::vector<Eigen::Vector3d>& points, const std::vector<double>& distances, const Eigen::Vector3d& estimate)
{
    size_t size = points.size();
    Eigen::VectorXd residuals(size);

    for (size_t i = 0; i < size; ++i)
    {
        residuals(i) = (estimate - points[i]).norm() - distances[i];
    }

    return residuals;
}

// Levenberg-Marquardt algorithm for trilateration
Eigen::Vector3d multilateration(const std::vector<Eigen::Vector3d>& points, const std::vector<double>& distances, const Eigen::Vector3d& initial_guess)
{
    // Use the initial guess
    Eigen::Vector3d estimate = initial_guess;

    // Levenberg-Marquardt parameters
    double lambda = 0.001;
    double updateNorm;
    int maxIterations = 1000;  // Maximum number of iterations
    int iterations = 0;  // Current iteration count

    do
    {
        Eigen::VectorXd residuals = computeResiduals(points, distances, estimate);

        // Compute the Jacobian matrix
        Eigen::MatrixXd J(residuals.size(), 3);
        for (size_t i = 0; i < points.size(); ++i)
        {
            Eigen::Vector3d diff = estimate - points[i];
            double dist = diff.norm();
            J.row(i) = diff / dist;
        }

        // Levenberg-Marquardt update
        Eigen::MatrixXd A = J.transpose() * J;
        A.diagonal() += lambda * A.diagonal();
        Eigen::VectorXd g = J.transpose() * residuals;
        Eigen::VectorXd delta = A.ldlt().solve(-g);
        estimate += delta;
        updateNorm = delta.norm();

        // Update lambda
        if (computeResiduals(points, distances, estimate).squaredNorm() < residuals.squaredNorm())
        {
            lambda /= 10;
        }
        else
        {
            lambda *= 10;
        }

        // Increment the iteration count
        iterations++;
    } while (updateNorm > 1e-6 && iterations < maxIterations);

    if (iterations == maxIterations)
    {
        throw std::runtime_error("Levenberg-Marquardt algorithm did not converge");
    }

    return estimate;
}


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
void saveDimensions(float length, float width, std::vector<Eigen::Vector3d> anchor_positions, std::string filename)
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
    float d1 = distance_1;
    float d2 = distance_2;
    float d3 = distance_3;

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
    float UWB_x = 0;
    float UWB_y = 0;
    float UWB_z = 0;

    /********* Room *********
     ******** Config *******/

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

    // IMU Orientation Startup Delay
    /*std::cout << "Wait 30 seconds for IMU Calibration" << std::endl;
    int i = 30;
    while (i != 0) {
        std::cout << i << " seconds" << std::endl;
        i -= 5;
        Sleep(5000);
    }*/

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

        /******** Time ********
        ******** Data ********/

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


        // reads incoming string into data
        std::vector<float> UWB_data;
        std::vector<float> IMU_data;
        float distance_1;
        float distance_2;
        float distance_3;
        float distance_4;
        float roll = 0;
        float pitch = 0;
        float yaw = 0;
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
        pitch = -IMU_data[1]; // degrees
        yaw = IMU_data[2]; // degrees
        dt = IMU_data[6] / 1000000; // s


        /***************/
        /***** UWB *****/
        /***************/

        // checks if UWB received data this pass
        if (recvLen_1 <= 0) {
            std::cout << "No Data Received" << std::endl;
            UWB_x = previous_UWB_position[0];
            UWB_y = previous_UWB_position[1];
            UWB_z = previous_UWB_position[2];
        }
        else if (distance_1 != 0 and distance_2 != 0 and distance_3 != 0 and distance_4 != 0) 
        {
            std::cout << "All Distances" << std::endl;
            // Define the anchor points
            std::vector<Eigen::Vector3d> points = {
                Eigen::Vector3d(point_1[0], point_1[1], point_1[2]),
                Eigen::Vector3d(point_2[0], point_2[1], point_2[2]),
                Eigen::Vector3d(point_3[0], point_3[1], point_3[2]),
                Eigen::Vector3d(point_4[0], point_4[1], point_4[2])
            };

            // Define the distances
            std::vector<double> distances = { distance_1, distance_2, distance_3, distance_4 };

            // Call the multilateration function
            Eigen::Vector3d result;
            try {
                result = multilateration(points, distances, previous_UWB_position);
                // Update the previous position
                previous_UWB_position = result;
            }
            catch (std::exception& e) {
                // Use the previous position if a unique solution is not found
                result = previous_UWB_position;
            }

            UWB_x = result[0];
            UWB_y = result[1];
            UWB_z = result[2];

        } 
        else 
        {
            std::cout << "Missing Information" << std::endl;
            continue;
        }

        // writes the location data to the console
        std::cout << "x: " << UWB_x << ", y: " << UWB_y << ", z: " << UWB_z << ", Time: " << hours << ":" << minutes << ":" << seconds << std::endl;

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
        if (UWB_x - previous_UWB_position[0] != 0 || UWB_y - previous_UWB_position[1] != 0 || UWB_z - previous_UWB_position[2] != 0) {
            previous_UWB_position[0] = UWB_x;
            previous_UWB_position[1] = UWB_y;
            previous_UWB_position[2] = UWB_z;
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

        float room_orientation = 20; // compass direction of positive x-axis of the room
        float theta = compass - room_orientation;
        if (theta < 0) {
            theta += 360;
        }

        float rtheta = theta * PI / 180;


        /***************/
        /***** Vis *****/
        /***************/

        // Write location data to file
        outFile << UWB_x << ", " << UWB_y << ", " << theta << ", " << hours << ", " << minutes << ", " << seconds << std::endl;

        // Clear the screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Convert the object position from meters to pixels
        int UWB_x_pixel = static_cast<int>(UWB_x * screen_scale);
        int UWB_y_pixel = static_cast<int>(UWB_y * screen_scale);

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
        SDL_SetRenderDrawColor(renderer, object_color.r, object_color.g, object_color.b, object_color.a);
        SDL_RenderFillRect(renderer, &object_rect);

        // Draw line for orientation
        SDL_RenderDrawLine(renderer, UWB_x_pixel, UWB_y_pixel, x_side_left, y_side_left);
        SDL_RenderDrawLine(renderer, UWB_x_pixel, UWB_y_pixel, x_side_right, y_side_right);
        SDL_RenderDrawLine(renderer, x_side_left, y_side_left, x_side_right, y_side_right);

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
