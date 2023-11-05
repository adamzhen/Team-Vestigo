#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QApplication>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsEllipseItem>
#include <QGraphicsPathItem>
#include <QPainterPath>
#include <QPixmap>
#include <QTimer>
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
using std::string;
using std::vector;
using std::runtime_error;
using namespace Eigen;


/***********************************
*********** MAIN PROGRAM ***********
***********************************/

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();

}
