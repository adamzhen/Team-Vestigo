#ifndef TAGDATA_H
#define TAGDATA_H

#include <vector>
#include <Eigen/Dense>
#include <iostream>
#include <fstream>
#include <cmath>
#include <string>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#include <fstream>
#include <vector>
#include <iomanip>
#include <thread>
#include "RootFinder.h"

#define BUF_SIZE 1024
#define WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#pragma comment(lib, "ws2_32.lib")
using namespace Eigen;

class TagData {
public:

    PositionMatrix anchor_positions; // Anchor Positions in x,y,z

    const int num_tags;
    const int num_data_pts;
    // static constants for use in Eigen matrices
    const static int static_num_tags = 4;
    const static int static_num_tag_data_pts = 4;
    const static int static_num_data_pts = 7;

    double room_length;
    double room_width;
    double room_height; // height of 1 floor

    TagData(int tags, int data_pts);

    // Function to get dimensions of room from user input
    double* getDimensions();

    // Function to get anchor locations within room from user input
    PositionMatrix getAnchors();

    // Function to read dimensions from a text file
    void readDimensions(std::string filename, double& length, double& width, PositionMatrix& anchor_positions);

    // Function to save dimensions to a text file
    void saveDimensions(double length, double width, PositionMatrix anchor_positions, std::string filename);

    Matrix<double, static_num_data_pts, 1> dataProcessing(std::string str);

    Matrix<double, static_num_tags, static_num_tag_data_pts> readTagData();

private:
    const double screen_scale = 150.0;

    int displayIterations = 0;
    Matrix<double, static_num_tags, 3> Tags_previous
        {
            {1.0, 1.0, 1.0},
            {1.0, 1.0, 1.0},
            {1.0, 1.0, 1.0},
            {1.0, 1.0, 1.0}
        };


    // Serial variables
    HANDLE hSerial;
    DCB dcbSerialParams = { 0 };

    // Output file
    std::ofstream outFile; // Declare the ofstream object
    std::string outFileName;
};

#endif // TAGDATA_H
