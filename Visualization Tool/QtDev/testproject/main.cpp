#include "mainwindow.h"
#include <QApplication>
#include <fstream>
#include <iostream>
#include <string>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    std::cout << "yo" << std::endl;
    // Read data from the CSV file
    std::ifstream inputFile("simdata.csv");
    if (!inputFile.is_open()) {
        std::cerr << "Failed to open the CSV file." << std::endl;
        return 1; // Return an error code or handle the error as needed.
    }

    std::string line;
    while (std::getline(inputFile, line)) {
        // Process and parse 'line' to extract CSV data.
        // For example, you can split the line into tokens using ',' as the delimiter.
        // Process and use the data as needed.
        std::cout << line << std::endl;
    }

    inputFile.close();

    return a.exec();
}
