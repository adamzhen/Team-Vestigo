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



Eigen::Vector3d trilateration(double distance_1, double distance_2, double distance_3, Eigen::Vector3d point_1, Eigen::Vector3d point_2, Eigen::Vector3d point_3)
{
    Eigen::Vector3d p1 = point_1;
    Eigen::Vector3d p2 = point_2;
    Eigen::Vector3d p3 = point_3;

    double d1 = distance_1;
    double d2 = distance_2;
    double d3 = distance_3;

    Eigen::MatrixXd A(3, 3);
    Eigen::VectorXd B(3);

    A << 2 * (p2(0) - p1(0)), 2 * (p2(1) - p1(1)), 2 * (p2(2) - p1(2)),
        2 * (p3(0) - p1(0)), 2 * (p3(1) - p1(1)), 2 * (p3(2) - p1(2)),
        2 * (p3(0) - p2(0)), 2 * (p3(1) - p2(1)), 2 * (p3(2) - p2(2));

    B << d1 * d1 - d2 * d2 - p1(0) * p1(0) + p2(0) * p2(0) - p1(1) * p1(1) + p2(1) * p2(1) - p1(2) * p1(2) + p2(2) * p2(2),
        d1* d1 - d3 * d3 - p1(0) * p1(0) + p3(0) * p3(0) - p1(1) * p1(1) + p3(1) * p3(1) - p1(2) * p1(2) + p3(2) * p3(2),
        d2* d2 - d3 * d3 - p2(0) * p2(0) + p3(0) * p3(0) - p2(1) * p2(1) + p3(1) * p3(1) - p2(2) * p2(2) + p3(2) * p3(2);

    Eigen::Vector3d x = A.colPivHouseholderQr().solve(B);

    return x;
}

int main()
{
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        std::cout << "WSAStartup failed with error: " << iResult << std::endl;
        return 1;
    }

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
        std::cout << "socket failed with error: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(1234);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cout << "bind failed with error: " << WSAGetLastError() << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    char buffer[1024];
    int recvLen;
    sockaddr_in clientAddr;
    int clientAddrLen = sizeof(clientAddr);

    while (true) {
        recvLen = recvfrom(sock, buffer, sizeof(buffer), 0, (sockaddr*)&clientAddr, &clientAddrLen);
        if (recvLen > 0) {
            std::string str(buffer, recvLen);

            Eigen::Vector3d point_1(0, 0, 0);
            Eigen::Vector3d point_2(1.71, 0, 0);
            Eigen::Vector3d point_3(-0.75, 2.74, 0);
            Eigen::Vector3d point_4(0.935, 2.79, 0);


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

            Eigen::Vector3d point_1_prime = trilateration(distance_1, distance_2, distance_3, point_1, point_2, point_3);
            Eigen::Vector3d point_2_prime = trilateration(distance_1, distance_2, distance_4, point_1, point_2, point_4);
            Eigen::Vector3d point_3_prime = trilateration(distance_4, distance_2, distance_3, point_4, point_2, point_3);
            Eigen::Vector3d point_4_prime = trilateration(distance_1, distance_4, distance_3, point_1, point_4, point_3);

            double x_location = (point_1_prime(0) + point_2_prime(0) + point_3_prime(0) + point_4_prime(0)) / 4;
            double y_location = (point_1_prime(1) + point_2_prime(1) + point_3_prime(1) + point_4_prime(1)) / 4;
            double z_location = (point_1_prime(2) + point_2_prime(2) + point_3_prime(2) + point_4_prime(2)) / 4;


            std::cout << "x: " << x_location << ", y: " << y_location << ", z: " << z_location << std::endl;
        }
    }
    return 0;
}