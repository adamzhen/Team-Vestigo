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

TagData tagData(4, 13); // Declare and construct TagData before using it

void MainWindow::setupTabs() {
    // Assuming we are inside the MainWindow constructor or setup function

    // Live visualization page setup
    mapWidget = new MapWidget(tagData);
    mapWidget->setStyleSheet("QWidget { background-color: #000000; }");

    // Add a start button to the live visualization page
    QPushButton *startButton = new QPushButton("Start Visualization", mapWidget);
    startButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #336699;" // Normal background color
        "    color: white;"              // Text color
        "    border: 2px solid #000000;"    // Border color and width
        "    border-radius: 5px;"       // Rounded corners with radius of 10px
        "    padding: 5px;"              // Padding around the text
        "}"
        "QPushButton:hover {"
        "    background-color: #5588cc;" // Background color when hovered
        "}"
        "QPushButton:pressed {"
        "    background-color: #224466;" // Background color when pressed
        "}"
        );


    // Create a QTimer
    vis_timer = new QTimer(this); // Allocate QTimer on the heap

    // Connect the timer's timeout signal to the mapWidget's updateCrewPositions slot
    connect(vis_timer, &QTimer::timeout, mapWidget, &MapWidget::updateCrewPositions);

    // Connect the start button to start the data reading and visualization
    connect(startButton, &QPushButton::clicked, this, [this, startButton]() {
        isPlaying = !isPlaying;
        if (isPlaying){
            // Start the timer with the desired DELAY in milliseconds
            vis_timer->start(800); // Example: 50 ms delay
            mapWidget->drawBackground();
            startButton->setText("Stop Visualization");
        } else {
            vis_timer->stop();
            mapWidget->clearBackground();
            startButton->setText("Start Visualization");
        }

    });

    // MainWindow constructor or setup function continues
    connect(mapWidget, &MapWidget::locationUpdated, this, &MainWindow::updateStatusBar);

    // Playback page setup
    playbackPage = new QWidget(this);
    playbackPage->setStyleSheet("QWidget { background-color: #000000; }");

    QPushButton *playPauseButton = new QPushButton("Play/Pause", playbackPage);
    startButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #336699;" // Normal background color
        "    color: white;"              // Text color
        "    border: 2px solid #000000;"    // Border color and width
        "    border-radius: 5px;"       // Rounded corners with radius of 10px
        "    padding: 5px;"              // Padding around the text
        "}"
        "QPushButton:hover {"
        "    background-color: #5588cc;" // Background color when hovered
        "}"
        "QPushButton:pressed {"
        "    background-color: #224466;" // Background color when pressed
        "}"
        );
    QPushButton *fastForwardButton = new QPushButton(">>", playbackPage);
    startButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #336699;" // Normal background color
        "    color: white;"              // Text color
        "    border: 2px solid #000000;"    // Border color and width
        "    border-radius: 5px;"       // Rounded corners with radius of 10px
        "    padding: 5px;"              // Padding around the text
        "}"
        "QPushButton:hover {"
        "    background-color: #5588cc;" // Background color when hovered
        "}"
        "QPushButton:pressed {"
        "    background-color: #224466;" // Background color when pressed
        "}"
        );
    QPushButton *rewindButton = new QPushButton("<<", playbackPage);
    startButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #336699;" // Normal background color
        "    color: white;"              // Text color
        "    border: 2px solid #000000;"    // Border color and width
        "    border-radius: 5px;"       // Rounded corners with radius of 10px
        "    padding: 5px;"              // Padding around the text
        "}"
        "QPushButton:hover {"
        "    background-color: #5588cc;" // Background color when hovered
        "}"
        "QPushButton:pressed {"
        "    background-color: #224466;" // Background color when pressed
        "}"
        );
    connect(playPauseButton, &QPushButton::clicked, this, [this, playPauseButton]() {
        isPlaying = !isPlaying;
        if (isPlaying){
            // Start the timer with the desired DELAY in milliseconds
            mapWidget->drawBackground();
            playPauseButton->setText("Pause");
        } else {
            playPauseButton->setText("Play");
        }
    });
    connect(fastForwardButton, &QPushButton::clicked, this, [this, fastForwardButton]() {

    });
    connect(rewindButton, &QPushButton::clicked, this, [this, rewindButton]() {
    });

    // Summary page setup
    summaryPage = new QWidget(this);
    summaryPage->setStyleSheet("QWidget { background-color: #000000; }");

    // Create a text input box
    QLineEdit *fileInput = new QLineEdit(summaryPage);
    fileInput->setGeometry(20, 20, 200, 30); // Position and size the input box
    fileInput->setStyleSheet("QLineEdit { color: white; background-color: black; }");
    // Create a Load File button
    QPushButton *loadFileButton = new QPushButton("Load File", summaryPage);
    loadFileButton->setGeometry(230, 20, 120, 30); // Position and size the button next to the input box
    loadFileButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #336699;" // Normal background color
        "    color: white;"              // Text color
        "    border: 2px solid #000000;" // Border color and width
        "    border-radius: 5px;"       // Rounded corners with radius of 10px
        "    padding: 5px;"             // Padding around the text
        "}"
        "QPushButton:hover {"
        "    background-color: #5588cc;" // Background color when hovered
        "}"
        "QPushButton:pressed {"
        "    background-color: #224466;" // Background color when pressed
        "}"
        );

    // Connect the Load File button to the appropriate slot or lambda function
    SummaryScene *summaryScene = new SummaryScene(this);
    connect(loadFileButton, &QPushButton::clicked, [summaryScene, fileInput, this]() {
        QString filename = fileInput->text();
        summaryScene->loadFile(filename);
        QLabel *imageLabel = new QLabel(summaryPage);
        int imageWidth = 600; // Example width
        int imageHeight = 600; // Example height
        int xPosition = (summaryPage->width() - imageWidth) / 2;
        int yPosition = (summaryPage->height() - imageHeight) / 2;

        imageLabel->setGeometry(xPosition, yPosition, imageWidth, imageHeight);

        // Optional: Style the QLabel
        imageLabel->setStyleSheet("QLabel { border: 2px solid #ffffff; }"); // White border for the image label

        // Load the image
        QPixmap imagePixmap(filename + "density_map.png"); // Adjust the file path as needed
        if (!imagePixmap.isNull()) {
            imageLabel->setPixmap(imagePixmap.scaled(imageWidth, imageHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            imageLabel->show(); // Show the label with the image
        }
    });


    // Assuming SummaryScene is a QWidget that you want to display
    // Assuming you want to fill the entire summary tab with the SummaryScene widget
    //QVBoxLayout *summaryLayout = new QVBoxLayout; // Create a layout for the summary page
    //summaryLayout->addWidget(summaryScene); // Add the summary scene to the layout
    //summaryPage->setLayout(summaryLayout); // Set the layout to the summary page widget


    // Tab widget setup
    tabWidget = new QTabWidget(this);
    tabWidget->addTab(mapWidget, "Live Visualization");
    tabWidget->addTab(playbackPage, "Playback");
    tabWidget->addTab(summaryPage, "Summary");

    setCentralWidget(tabWidget);

}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    mapWidget(nullptr),  // Initialize to nullptr or with a new instance
    vis_timer(nullptr)       // Initialize to nullptr
{
    ui->setupUi(this);
    setGeometry(0, 0, 1300, 700);
    //showMaximized();

    // Setup the tabs
    setupTabs();



}

void MainWindow::updateStatusBar(const QString& locationData) {
    statusBar()->setStyleSheet("QStatusBar { color: #e8e8e8; }");
    statusBar()->showMessage(locationData);
}

MainWindow::~MainWindow()
{
    delete ui; // QTimer and MapWidget will be deleted automatically since they are children of MainWindow
}

