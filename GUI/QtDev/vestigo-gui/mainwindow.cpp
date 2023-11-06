#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <iostream>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#  include <QDesktopWidget>
#endif
#include <QScreen>
#include <QMessageBox>
#include <QMetaEnum>
#include <QTimer>
#include "mapwidget.h"
#include "tagdata.h"

TagData tagData(4, 7); // Declare and construct TagData before using it

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    mapWidget(nullptr),  // Initialize to nullptr or with a new instance
    timer(nullptr)       // Initialize to nullptr
{
    ui->setupUi(this);
    setGeometry(0, 0, 1300, 700);
    //showMaximized();

    mapWidget = new MapWidget(tagData); // Use 'new' to allocate on the heap
    setCentralWidget(mapWidget);
    // mapWidget->show();

    // Create a QTimer
    timer = new QTimer(this); // Allocate QTimer on the heap

    // Connect the timer's timeout signal to the mapWidget's updateCrewPositions slot
    connect(timer, &QTimer::timeout, mapWidget, &MapWidget::updateCrewPositions);
    // MainWindow constructor or setup function
    connect(mapWidget, &MapWidget::locationUpdated, this, &MainWindow::updateStatusBar);

    // Set the timer interval to DELAY milliseconds
    timer->start(1); // Replace 1 with the desired DELAY in milliseconds
}

void MainWindow::updateStatusBar(const QString& locationData) {
    statusBar()->setStyleSheet("QStatusBar { color: #e8e8e8; }");
    statusBar()->showMessage(locationData);
}

MainWindow::~MainWindow()
{
    delete ui; // QTimer and MapWidget will be deleted automatically since they are children of MainWindow
}

