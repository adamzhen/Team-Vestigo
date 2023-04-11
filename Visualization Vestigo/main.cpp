#include <iostream>
#include <Eigen/Dense>
#include <string>
#include <cstring>
#include <ws2tcpip.h>

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
#include <sys/stat.h>

using std::cout;
using std::cin;
using std::endl;


// Definitions
const double inch_to_meter = 0.0254;

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
    result[1] = (width* inch_to_meter);

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
Eigen::Vector3d trilateration(double distance_1, double distance_2, double distance_3, Eigen::Vector3d point_1, Eigen::Vector3d point_2, Eigen::Vector3d point_3)
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

// main code
int main()
{   
    // Declarations
    Eigen::Vector3d point_1;
    Eigen::Vector3d point_2;
    Eigen::Vector3d point_3;
    Eigen::Vector3d point_4;

    double length;
    double width;

    std::cout << "Do you want to set up a new room? (Y/N)? ";
    char choice_new_room;
    std::cin >> choice_new_room;

    if (choice_new_room == 'Y' || choice_new_room == 'y') {

        // Ask for room dimensions from user
        std::vector<double, double> room_dimensions = getDimensions();
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

        std::vector<Eigen::Vector3d> all_dimensions = readDimensions(read_filename);


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
    std::cout << "Room dimensions (m): " << width << " x " << length << std::endl;
    std::cout << "Screen size (pixels): " << screen_width << " x " << screen_height << std::endl;
    std::cout << "Anchor points (m): " << std::endl;
    std::cout << "  Point 1: " << point_1.transpose() << std::endl;
    std::cout << "  Point 2: " << point_2.transpose() << std::endl;
    std::cout << "  Point 3: " << point_3.transpose() << std::endl;
    std::cout << "  Point 4: " << point_4.transpose() << std::endl;

    // Open the file for writing
    std::ofstream outFile("Tracked Location", std::ios::app);

    // Check if the file was opened successfully
    if (!outFile) {
        std::cerr << "Error opening file." << std::endl;
        return 1;
    }

    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        std::cout << "WSAStartup failed with error: " << iResult << std::endl;
        return 1;
    }

    // check if socket connection is good
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
        std::cout << "socket failed with error: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    // defines port and ip address
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(1234);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    // checks if port binded properly
    if (bind(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cout << "bind failed with error: " << WSAGetLastError() << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    // defines buffer size and client address
    char buffer[1024];
    int recvLen;
    sockaddr_in clientAddr;
    int clientAddrLen = sizeof(clientAddr);

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
    int object_width = 5;
    int object_height = 5;
    SDL_Color object_color = { 255, 0 , 0, 255 };

    // data collection and display loop
    bool quit = false;
    while (!quit) {
        recvLen = recvfrom(sock, buffer, sizeof(buffer), 0, (sockaddr*)&clientAddr, &clientAddrLen);
        if (recvLen > 0) {
            std::string str(buffer, recvLen);

            // converts incoming string into 4 distance doubles
            size_t start = str.find("[") + 1;
            size_t end = str.find(",");
            double distance_1 = std::stod(str.substr(start, end - start));

            start = end + 1;
            end = str.find(",", start);
            double distance_2 = std::stod(str.substr(start, end - start));

            start = end + 1;
            end = str.find(",", start);
            double distance_3 = std::stod(str.substr(start, end - start));

            start = end + 1;
            end = str.find("]", start);
            double distance_4 = std::stod(str.substr(start, end - start));

            // calculates the prime points to determine the location given overestimates
            Eigen::Vector3d point_1_prime = trilateration(distance_1, distance_2, distance_3, point_1, point_2, point_3);
            Eigen::Vector3d point_2_prime = trilateration(distance_1, distance_2, distance_4, point_1, point_2, point_4);
            Eigen::Vector3d point_3_prime = trilateration(distance_4, distance_2, distance_3, point_4, point_2, point_3);
            Eigen::Vector3d point_4_prime = trilateration(distance_1, distance_4, distance_3, point_1, point_4, point_3);

            // finds location of tag in (x,y,z)
            double x_location = (point_1_prime(0) + point_2_prime(0) + point_3_prime(0) + point_4_prime(0)) / 4;
            double y_location = (point_1_prime(1) + point_2_prime(1) + point_3_prime(1) + point_4_prime(1)) / 4;
            double z_location = (point_1_prime(2) + point_2_prime(2) + point_3_prime(2) + point_4_prime(2)) / 4;

            // writes the location data to the console
            std::cout << "x: " << x_location << ", y: " << y_location << ", z: " << z_location << std::endl;

            // Handle events (such as window close)
            SDL_Event event;
            while (SDL_PollEvent(&event))
            {
                if (event.type == SDL_QUIT)
                {
                    quit = true;
                }
            }

            // Write location data to file
            outFile << "X Position: " << x_location << ", Y Position: " << y_location << std::endl;

            // Clear the screen
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);

            // Convert the object position from meters to pixels
            int x_location_pixel = static_cast<int>(x_location * 150);
            int y_location_pixel = static_cast<int>(y_location * 150);

            // Draw the object
            SDL_Rect object_rect = { x_location_pixel, y_location_pixel, object_width, object_height };
            SDL_SetRenderDrawColor(renderer, object_color.r, object_color.g, object_color.b, object_color.a);
            SDL_RenderFillRect(renderer, &object_rect);

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