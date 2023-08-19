#include <iostream>
#include <string>
#include <cstring>
#include <cmath>
#include <algorithm>
#define BUF_SIZE 1024
#define WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
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
#include <random>
#include "RootFinder.h"

#undef main

using namespace std;
using std::cout;
using std::cin;
using std::endl;
using namespace Eigen;


// Definitions
const double inch_to_meter = 0.0254;
Matrix<double, 4, 3> Tags_previous 
{
    {1.0, 1.0, 1.0}, 
    {1.0, 1.0, 1.0}, 
    {1.0, 1.0, 1.0}, 
    {1.0, 1.0, 1.0}
};

Matrix<double, 4, 4> previous_IMU_data 
{
    {0.0, 0.0, 0.0, 0.0},
    {0.0, 0.0, 0.0, 0.0}, 
    {0.0, 0.0, 0.0, 0.0},
    {0.0, 0.0, 0.0, 0.0}
};

/*********************************************
*********** ROOM AND ANCHOR CONFIG ***********
*********************************************/

// Function to get dimensions of room from user input
double* getDimensions()
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
PositionMatrix getAnchors()
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
void readDimensions(string filename, double& length, double& width, PositionMatrix& anchor_positions)
{
    ifstream inputFile;
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
void saveDimensions(double length, double width, PositionMatrix anchor_positions, string filename)
{
    ofstream outputFile;
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
Matrix<double, 16, 1> dataProcessing(string str)
{
    vector<double> tempData;

    size_t start = str.find("[") + 1;
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

// Draws Object for Renderer
void drawObject(double positionXmeters, double positionYmeters, double theta, SDL_Renderer* renderer, double screen_scale, SDL_Color colors[], int j)
{
    // Convert the object position from meters to pixels
    int Tag_x_pixel = static_cast<int>(positionXmeters * screen_scale);
    int Tag_y_pixel = static_cast<int>(positionYmeters * screen_scale);

    // Calculate line for orientation, currently assuming that North is in the positive x direction
    int draw_length = 75;
    int x_top = Tag_x_pixel + draw_length * cos(theta);
    int y_top = Tag_y_pixel + draw_length * sin(theta);
    int x_side_left = Tag_x_pixel + draw_length * cos(theta - M_PI / 4);
    int y_side_left = Tag_y_pixel + draw_length * sin(theta - M_PI / 4);
    int x_side_right = Tag_x_pixel + draw_length * cos(theta + M_PI / 4);
    int y_side_right = Tag_y_pixel + draw_length * sin(theta + M_PI / 4);

    // Draw the object
    SDL_Rect object_rect = { Tag_x_pixel - 4, Tag_y_pixel - 4, 8, 8 };
    SDL_SetRenderDrawColor(renderer, colors[j].r, colors[j].g, colors[j].b, colors[j].a);
    SDL_RenderFillRect(renderer, &object_rect);

    // Draw line for orientation
    SDL_RenderDrawLine(renderer, Tag_x_pixel, Tag_y_pixel, x_side_left, y_side_left);
    SDL_RenderDrawLine(renderer, Tag_x_pixel, Tag_y_pixel, x_side_right, y_side_right);
    SDL_RenderDrawLine(renderer, x_side_left, y_side_left, x_side_right, y_side_right);
}

// Total Data Processing for Threads


/***********************************
*********** MAIN PROGRAM ***********
***********************************/

int main()
{
    /***********************************
    *********** ROOM CONFIG ***********
    ***********************************/

    // Anchor Positions in x,y,z
    PositionMatrix anchor_positions;

    // Room Dimensions
     double length;
     double width;

     // Time Tracking
     chrono::high_resolution_clock::time_point time_start;

     cout << "Tracking Startup Menu:" << endl;
     cout << "Do you want to set up a new room? (Y/N)? ";
     char choice_new_room;
     cin >> choice_new_room;

    if (choice_new_room == 'Y' || choice_new_room == 'y') {

        // Ask for room dimensions from user
        double* room_dimensions = getDimensions();
        length = room_dimensions[0];
        width = room_dimensions[1];

        // Ask for the anchor location from user
        anchor_positions = getAnchors();

        cout << "Do you want to save the new room configuration (Y/N)?";
        char choice_save;
        cin >> choice_save;

        if (choice_save == 'Y' || choice_save == 'y')
        {
            cout << "Enter the name of the new room: ";
            string save_filename;
            cin >> save_filename;

            // Check for existing file
            struct stat buffer;
            if (stat(save_filename.c_str(), &buffer) == 0) {

                cout << "File already exists, try again";

                return 0;
            }

            saveDimensions(length, width, anchor_positions, save_filename);

        }
    }
    else if (choice_new_room == 'n' || choice_new_room == 'N') {

        cout << "Enter the name of an existing room: ";
        string read_filename;
        cin >> read_filename;

        // Checks for existing file
        struct stat buffer;
        if (stat(read_filename.c_str(), &buffer) != 0) {

            cout << "File does not exist, try again";

            return 0;
        }

        readDimensions(read_filename, length, width, anchor_positions);

        cout << anchor_positions.size() << endl;
    }
    else {

        cout << "Invalid Input";

        return 0;
    }

    const double screen_scale = 150.0;
    double screen_width = width * screen_scale;
    double screen_height = length * screen_scale;

    cout << "Room dimensions (m): " << length << " x " << width << endl;
    cout << "Screen size (pixels): " << screen_height << " x " << screen_width << endl;
    cout << "Anchor points (m): " << endl;
    for (int i = 0; i < anchor_positions.rows(); ++i) {
        cout << "  Point " << (i + 1) << ": " << anchor_positions.row(i).transpose() << endl;
    }

    // Open the file for writing
    ofstream outFile("Tracked Location", ios::app);

    // Check if the file was opened successfully
    if (!outFile) {
        cerr << "Error opening file." << endl;
        return 1;
    }

    /***********************************
    *********** SDL2 STARTUP ***********
    ***********************************/

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        cout << "Failed to initialize SDL: " << SDL_GetError() << endl;
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

    /*************************************
    *********** SERIAL STARTUP ***********
    *************************************/

    HANDLE hSerial = CreateFile(TEXT("COM4"), GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (hSerial == INVALID_HANDLE_VALUE) {
        // Handle error
        throw runtime_error("Connection Not Init Properly");
    }
    DCB dcbSerialParams = { 0 };
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    GetCommState(hSerial, &dcbSerialParams);
    dcbSerialParams.BaudRate = CBR_256000;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;
    SetCommState(hSerial, &dcbSerialParams);

    if (!PurgeComm(hSerial, PURGE_RXCLEAR && PURGE_TXCLEAR)) {
        // Handle the error, if needed.
        throw runtime_error("Failed to purge the buffers");
    }

    cout << "Serial Start Up Complete" << endl;

    /**************************************
    *********** DATA PROCESSING ***********
    **************************************/

    bool quit = false;
    while (!quit) {

        // get the current timestamp
        auto now = chrono::system_clock::now();
        // convert the timestamp to a time_t object
        time_t timestamp = chrono::system_clock::to_time_t(now);
        // set the desired timezone
        tm timeinfo;
        localtime_s(&timeinfo, &timestamp);

        int hours = timeinfo.tm_hour;
        int minutes = timeinfo.tm_min;
        int seconds = timeinfo.tm_sec;
        int displayIterations = 0;

        // Defines variables for location coordinates
        const int num_tags = 4;
        Matrix<double, 4, 3> Tags_position;

        double roll, pitch, yaw, dt;

        MatrixXd tag_data(num_tags, 16);
        VectorXd distances(12);

        string completeMessage;
        byte buffer[1024];
        size_t MAX_MESSAGE_SIZE = 750;

        //while (true)
        //{
        //    DWORD bytesRead;
        //    char c;

        //    time_start = chrono::high_resolution_clock::now();

        //    if (!ReadFile(hSerial, &c, 1, &bytesRead, NULL) || bytesRead == 0) {
        //        // Handle error
        //        throw runtime_error("Bytes Not Correctly Read, In Loop");
        //    };

        //    auto time_end = chrono::high_resolution_clock::now();

        //    // Compute the difference
        //    auto duration = chrono::duration_cast<std::chrono::microseconds>(time_end - time_start).count();

        //    std::cout << "Program executed in: " << duration << " microseconds. Char: " << c << std::endl;

        //    if (c == '<') {
        //        cout << "!!!!!!!!!!!!!" << endl;
        //    }
        //}

        bool cleared = false;
        while (true)
        {

            DWORD bytesRead;
            char c;

            if (!ReadFile(hSerial, &c, 1, &bytesRead, NULL) || bytesRead == 0) {
                // Handle error
                throw runtime_error("Bytes Not Correctly Read, In Loop");
            };

            if (c == '<') 
            { 
                // Start of a message
                completeMessage.clear();
                cleared = true; 
            }
            else if (c == '>' && cleared) 
            {
                size_t start = 1, end = 0;
                for (int u = 0; u < num_tags; ++u) 
                { 
                    // Iterating through each vector within the 2D matrix
                    end = completeMessage.find("]", start);
                    tag_data.row(u) = dataProcessing(completeMessage.substr(start, end - start + 1)).transpose();
                    start = end + 2;
                }
                // Reset the completeMessage for the next read
                completeMessage.clear();
                break;
            }
            else 
            {
                completeMessage += c;
            }
        
        }


        // Prints out data received
        // cout << tag_data << endl;

        /*** SDL SETUP **/
        // Clear the screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Loop through the tags
        for (int j = 0; j < num_tags; ++j) {
            distances = tag_data.row(j).head(12);
            roll = tag_data(j, 12);
            pitch = tag_data(j, 13);
            yaw = tag_data(j, 14);
            dt = tag_data(j, 15);

            // cout << roll << ", " << pitch << ", " << yaw << ", " << dt << endl;

            // Call the multilateration function
            Vector3d Tags_current;
            LMsolution result;
            try
            {
                 result = RootFinder::LevenbergMarquardt(anchor_positions, distances, Tags_previous.row(j).transpose());
                 Tags_current = result.solution;
            }
            catch (exception& e)
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
                Tags_position.row(j) = Tags_current;
            }
            else if (displayIterations > 0)
            {
                Tags_position.row(j) = Tags_previous.row(j);
                cout << "PREVIOUS USED" << endl;
            }
            else
            {
                Tags_position.row(j) = Tags_current;
            }


            // writes the location data to the console
            cout << "x: " << Tags_position.row(j)[0] << ", y: " << Tags_position.row(j)[1] << ", z: " << Tags_position.row(j)[2] << ", Time: " << hours << ":" << minutes << ":" << seconds << endl;

            // Handle events (such as window close)
            SDL_Event event;
            while (SDL_PollEvent(&event))
            {
                if (event.type == SDL_QUIT)
                {
                    quit = true;
                }
            }

            cout << "Drawing Objects" << endl;
            // randomizer for testing
            static random_device rd;           // Obtain a random number from hardware
            static mt19937 eng(rd());          // Seed the generator
            static uniform_int_distribution<> distr(0, 360); // Define the range

            double randomthing = distr(eng);

            drawObject(Tags_position.row(j)[0], Tags_position.row(j)[1], randomthing, renderer, screen_scale, colors, j);
        }       

        // Presents the rendereer to the screen
        SDL_RenderPresent(renderer);

        displayIterations++;
    }

    // Close the file
    outFile.close();

    // Clean up resources
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
