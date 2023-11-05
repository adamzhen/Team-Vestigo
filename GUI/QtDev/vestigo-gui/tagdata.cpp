#include "tagdata.h"
#include <cmath>
#include <Eigen/Dense>
#include <iostream>
#include <string>
#include <cstring>
#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#include <fstream>
#include <vector>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <thread>
#include <random>
#include <sys/stat.h>
#include "RootFinder.h"

#define BUF_SIZE 1024
#define WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#pragma comment(lib, "ws2_32.lib")

using std::cout;
using std::cin;
using std::endl;
using std::vector;
using std::string;
using std::runtime_error;

const double inch_to_meter = 0.0254;

TagData::TagData(int tags, int data_pts) : num_tags(tags), num_data_pts(data_pts) {

    /******** TIME TRACKING ********/
    // get the current timestamp
    auto now = std::chrono::system_clock::now();
    // convert the timestamp to a time_t object
    time_t timestamp = std::chrono::system_clock::to_time_t(now);
    // set the desired timezone
    tm timeinfo;
    localtime_s(&timeinfo, &timestamp);

    int year = timeinfo.tm_year + 1900;
    int month = timeinfo.tm_mon + 1;
    int day = timeinfo.tm_mday;
    int hours = timeinfo.tm_hour;
    int minutes = timeinfo.tm_min;
    int seconds = timeinfo.tm_sec;

    // Open the file for writing
    outFileName = std::to_string(month) + "-" + std::to_string(day) + "-" + std::to_string(year) + ".csv";
    outFile.open(outFileName, std::ios::out | std::ios::app);
    // Check if the file was opened successfully
    if (!outFile) {
        std::cerr << "Error opening file." << endl;
    }
    // write header
    outFile << "Time, ";
    for (int i = 0; i < num_tags; ++i) {
        outFile << "Tag " << i + 1 << " X, " << "Tag " << i + 1 << " Y, " << "Tag " << i + 1 << " Z, ";
    }
    outFile << endl;

    /***********************************
    *********** ROOM CONFIG ***********
    ***********************************/

    cout << "Tracking Startup..." << endl;
    string read_filename = "dev";

    // Checks for existing file
    struct stat buffer;
    if (stat(read_filename.c_str(), &buffer) != 0) {
        cout << "File does not exist, try again";
    }

    readDimensions(read_filename, room_length, room_width, anchor_positions);

    cout << anchor_positions.size() << endl;

    cout << "Room dimensions (m): " << room_length << " x " << room_width << endl;
    cout << "Anchor points (m): " << endl;
    for (int i = 0; i < anchor_positions.rows(); ++i) {
        cout << "  Point " << (i + 1) << ": " << anchor_positions.row(i) << endl;
    }

    /*************************************
    *********** SERIAL STARTUP ***********
    *************************************/

    hSerial = CreateFile(TEXT("COM3"), GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (hSerial == INVALID_HANDLE_VALUE) {
        // Handle error
        throw runtime_error("Connection Not Init Properly");
    }
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    GetCommState(hSerial, &dcbSerialParams);
    dcbSerialParams.BaudRate = CBR_9600;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;
    SetCommState(hSerial, &dcbSerialParams);

    if (!PurgeComm(hSerial, PURGE_RXCLEAR | PURGE_TXCLEAR)) {
        // Handle the error, if needed.
        throw runtime_error("Failed to purge the buffers");
    }

    DWORD bytesRead;
    char c;

    if (!ReadFile(hSerial, &c, 1, &bytesRead, NULL) || bytesRead == 0) {
        // Handle error
        cout << "Bytes Read: " << bytesRead << endl;
        throw runtime_error("Bytes Not Correctly Read, In Main");
    };

    cout << "Serial Start Up Complete" << endl;

}

// Function to get dimensions of room from user input
double* TagData::getDimensions()
{
    double length;
    double width;

    cout << "Room Setup Menu" << endl;
    cout << "Input the length of the room in inches and press enter: ";
    cin >> length;


    cout << "Input the width of the room in inches and press enter: ";
    cin >> width;

    cout << endl;

    double* result = new double[2];
    result[0] = (length * inch_to_meter);
    result[1] = (width * inch_to_meter);

    return result;
}

// Function to get anchor locations within room from user input
PositionMatrix TagData::getAnchors()
{
    // Define the number of anchor points
    const int numAnchors = 12;

    // Ask for dimensions of the anchor points in inches
    cout << "Input the x, y, and z position of " << numAnchors << " anchor points in inches with spaces in between each and press enter: " << endl;

    // Initialize the result matrix with correct size
    PositionMatrix result(numAnchors, 3);

    double x, y, z;

    // Loop through each anchor point and ask for its dimensions
    for (int i = 0; i < numAnchors; ++i)
    {
        cout << "Anchor Point " << i + 1 << ": ";
        cin >> x >> y >> z;

        // Convert each anchor point from inches to meters and insert it into the result
        result.row(i) << x * inch_to_meter, y* inch_to_meter, z* inch_to_meter;
    }

    return result;
}


// Function to read dimensions from a text file
void TagData::readDimensions(string filename, double& length, double& width, PositionMatrix& anchor_positions)
{
    std::ifstream inputFile;
    inputFile.open(filename);

    const int numberOfAnchors = 12;

    if (inputFile.is_open())
    {
        string line;
        getline(inputFile, line);
        sscanf_s(line.c_str(), "Room Dimensions: %lf, %lf", &length, &width);

        // Resize the anchor_positions matrix to the correct size
        anchor_positions.resize(numberOfAnchors, 3);

        for (int i = 0; i < numberOfAnchors; i++)
        {
            Vector3d point;
            getline(inputFile, line);
            sscanf_s(line.c_str(), "Anchor %*d: %lf, %lf, %lf", &point[0], &point[1], &point[2]);

            // Set the i-th row of anchor_positions to point
            anchor_positions.row(i) = point;
        }

        cout << "Dimensions read from " << filename << endl;
    }
    else
    {
        cout << "Error: Unable to open file" << endl;
    }

    inputFile.close();
}

// Function to save dimensions to a text file
void TagData::saveDimensions(double length, double width, PositionMatrix anchor_positions, string filename)
{
    std::ofstream outputFile;
    outputFile.open(filename);

    if (outputFile.is_open())
    {
        outputFile << "Room Dimensions: " << length << ", " << width << endl;

        // Loop through the anchor positions and write each one to the file
        for (int i = 0; i < anchor_positions.rows(); ++i)
        {
            Vector3d point = anchor_positions.row(i);
            outputFile << "Anchor " << i + 1 << ": " << point[0] << ", " << point[1] << ", " << point[2] << endl;
        }

        cout << "Dimensions saved to " << filename << endl;
    }
    else
    {
        cout << "Error: Unable to open file" << endl;
    }

    outputFile.close();
}


/*****************************************
*********** GENERAL FUNCTIONS  ***********
*****************************************/

// Incoming Data Processing
Matrix<double, TagData::static_num_data_pts, 1> TagData::dataProcessing(string str)
{

    vector<double> tempData;

    size_t start = str.find("[") + 2;
    size_t end = str.find(",");

    while (str.find("]", start) != string::npos) {
        tempData.push_back(stof(str.substr(start, end - start)));
        start = end + 1;
        end = str.find(",", start);
        if (end == string::npos) {
            end = str.find("]", start);
        }
    }

    // Initialize Eigen vector with standard vector
    VectorXd data = Map<VectorXd, Unaligned>(tempData.data(), tempData.size());

    return data;
}


Matrix<double, TagData::static_num_tags, TagData::static_num_tag_data_pts> TagData::readTagData() {

    /******** TIME TRACKING ********/
    // get the current timestamp
    auto now = std::chrono::system_clock::now();
    // convert the timestamp to a time_t object
    time_t timestamp = std::chrono::system_clock::to_time_t(now);
    // set the desired timezone
    tm timeinfo;
    localtime_s(&timeinfo, &timestamp);

    int year = timeinfo.tm_year + 1900;
    int month = timeinfo.tm_mon + 1;
    int day = timeinfo.tm_mday;
    int hours = timeinfo.tm_hour;
    int minutes = timeinfo.tm_min;
    int seconds = timeinfo.tm_sec;

    /**************************************
   *********** DATA PROCESSING ***********
   **************************************/

    // Defines variables for all tag data (3 position, 1 orientation)
    Matrix<double, TagData::static_num_tags, TagData::static_num_tag_data_pts> TAG_DATA;

    double yaw;

    MatrixXd raw_data(num_tags, num_data_pts);
    VectorXd distances(12);

    bool reading = false; // represents if we are currently reading a message
    bool donereading = false; // represents if we have finished reading a message

    //    if (!PurgeComm(hSerial, PURGE_RXCLEAR | PURGE_TXCLEAR)) {
    //        // Handle the error, if needed.
    //        throw runtime_error("Failed to purge the buffers");
    //    }
    constexpr size_t BUFFER_SIZE = 1024;  // Choose an appropriate buffer size

    char buffer[BUFFER_SIZE];
    std::string completeMessage;

    while (!donereading) {
        DWORD bytesRead;
        if (!ReadFile(hSerial, buffer, BUFFER_SIZE, &bytesRead, NULL) || bytesRead == 0) {
            // Handle error
            std::cerr << "Bytes Read: " << bytesRead << std::endl;
            throw std::runtime_error("Bytes Not Correctly Read, In Loop");
        }

        for (DWORD i = 0; i < bytesRead; ++i) {
            char c = buffer[i];

            if (c == '<') {
                // Start of a message
                completeMessage.clear();
                completeMessage += c;  // Include the '<' character in the message
                reading = true;
            } else if (c == '>' && !completeMessage.empty()) {
                // End of a message
                size_t start = 1, end = 0;
                for (int u = 0; u < num_tags; ++u) {
                    end = completeMessage.find("]", start);
                    raw_data.row(u) = dataProcessing(completeMessage.substr(start, end - start + 1)).transpose();
                    start = end + 2;
                }

                // Reset the completeMessage for the next read
                completeMessage.clear();
                if (reading){
                    donereading = true;
                    break;
                }
            } else if (!completeMessage.empty()) {
                // Append to the message if we are in the middle of reading
                completeMessage += c;
            }
        }
    }

    // Prints out data received
    // cout << raw_data << endl;

    // get current time and format in HH:MM:SS
    std::ostringstream timeStream;
    timeStream << std::setfill('0') << std::setw(2) << hours << ":"
               << std::setfill('0') << std::setw(2) << minutes << ":"
               << std::setfill('0') << std::setw(2) << seconds;
    // Convert the string stream to a string
    std::string timeString = timeStream.str();

    // Write timestamp in HH:MM:SS format
    outFile << timeString << ", ";

    // Loop through the tags
    for (int j = 0; j < num_tags; ++j) {

        distances = raw_data.row(j).head(12);
        yaw = raw_data(j, 12);
        TAG_DATA(j, 3) = yaw;

        // Call the multilateration function
        Vector3d Tags_current;
        LMsolution result;
        try
        {
            result = RootFinder::LevenbergMarquardt(anchor_positions, distances, Tags_previous.row(j).transpose());
            Tags_current = result.solution;
        }
        catch (_exception& e)
        {
            Tags_current = Tags_previous.row(j).transpose();
            throw runtime_error("Exception in LM");
        }

        if (result.exit_type == ExitType::AboveMaxIterations)
        {
            throw runtime_error("Max Iterations in LM");
        }

        if (result.exit_type == ExitType::BelowDynamicTolerance)
        {
            cout << "DYNAMIC TOLERANCE EXIT WARNING. Iterations: " << result.iterations << endl;
        }

        distances.setZero();

        if (abs(Tags_current.norm() - Tags_previous.row(j).norm()) < 0.25)
        {
            TAG_DATA.block<1,3>(j, 0) = Tags_current;
        }
        else if (displayIterations > 0)
        {
            TAG_DATA.block<1,3>(j, 0) = Tags_previous.row(j);
            cout << "PREVIOUS USED" << endl;
        }
        else
        {
            TAG_DATA.block<1,3>(j, 0) = Tags_current;
        }

        // writes the location data to the console
        cout << "x: " << TAG_DATA.row(j)[0] << ", y: " << TAG_DATA.row(j)[1] << ", z: " << TAG_DATA.row(j)[2] << ", yaw: " << TAG_DATA.row(j)[3] << ", Time: " << timeString << endl;

        // writes data to output csv file
        outFile << TAG_DATA.row(j)[0] << ", " << TAG_DATA.row(j)[1] << ", " << TAG_DATA.row(j)[2];
        if (j == num_tags - 1)
        {
            outFile << endl;
        } else {
            outFile << ", ";
        }

    }
    cout << endl;

    return TAG_DATA;

}

