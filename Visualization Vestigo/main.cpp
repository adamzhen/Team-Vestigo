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

using std::cout;
using std::cin;
using std::endl;

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
    // Ask for dimensions of the room in inches
    std::cout << "Enter the dimensions of the room in inches (X Y): ";
    double room_width_inches, room_height_inches;
    std::cin >> room_width_inches >> room_height_inches;

    // Convert dimensions from inches to meters and set screen size
    const double inch_to_meter = 0.0254;
    double room_width_meters = room_width_inches * inch_to_meter;
    double room_height_meters = room_height_inches * inch_to_meter;
    const double screen_scale = 150.0;
    double screen_width = room_width_meters * screen_scale;
    double screen_height = room_height_meters * screen_scale;

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

    // Print out results
    std::cout << "Room dimensions (m): " << room_width_meters << " x " << room_height_meters << std::endl;
    std::cout << "Screen size (pixels): " << screen_width << " x " << screen_height << std::endl;
    std::cout << "Anchor points (m): " << std::endl;
    std::cout << "  Point 1: " << point_1.transpose() << std::endl;
    std::cout << "  Point 2: " << point_2.transpose() << std::endl;
    std::cout << "  Point 3: " << point_3.transpose() << std::endl;
    std::cout << "  Point 4: " << point_4.transpose() << std::endl;

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
    SDL_Window* window = SDL_CreateWindow("Object Movement", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, screen_width, screen_height, SDL_WINDOW_SHOWN);

    // Create a renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    // Initial position of the object
    double x_location = 3.0;
    double y_locatioin = 3.0;

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

    // Clean up resources
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}