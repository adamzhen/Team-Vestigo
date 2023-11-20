#include "playback.h"
#include <QFile>
#include <QTextStream>
#include <QStringList>
#include <QTimer>
#include "crewmember.h"
#include <iostream>
// Constructor
Playback::Playback(QWidget *parent)
    : QGraphicsView(parent), playbackScene(new QGraphicsScene(this)), currentFrame(0), isPlaying(false) {
    // Set the scene's rectangle
    playbackScene->setSceneRect(0, 0, 2*rectWidth, rectLength);
    playbackScene->setBackgroundBrush(QColor("#000000"));

    playbackTimer = new QTimer(this);
    connect(playbackTimer, &QTimer::timeout, this, &Playback::updatePositions);
}
bool Playback::isCurrentlyPlaying() const {
    return isPlaying;
}
void Playback::setCurrentFrame(int frame) {
    if (frame >= 0 && static_cast<unsigned long long>(frame) < playbackData.size()) {
        currentFrame = frame;
        // Optionally, update the positions of items or refresh the view here.
        updatePositions();
    }
}
void Playback::initializeCrewMembers() {
    // Assuming crewColors and other necessary data are available4
    for (int i = 0; i < 4; ++i) {
        CrewMember* crew = new CrewMember(0, 0, crewColors[i], 500, 500, 6, 6,6,20,45); // Use appropriate constructor parameters
        crewMembers.push_back(crew);
        playbackScene->addItem(crew);
    }
}
void Playback::drawPlayback() {

    // Create the border as a rectangle item
    // The border's width is the pen width, so we reduce the size of the rect item by twice the pen width
    // to ensure that the border is drawn inside the scene.
    qDebug() << "Drawing playback scene";
    int borderWidth = 20;
    QGraphicsRectItem *floorRect1 = playbackScene->addRect(0, 0, rectWidth - 2 * borderWidth, rectLength - 2 * borderWidth);
    QGraphicsRectItem *floorRect2 = playbackScene->addRect(rectWidth+margin, 0, rectWidth - 2 * borderWidth, rectLength - 2 * borderWidth);
    qDebug() << "Added floor rectangles and other items to the scene";
    // Set the pen for the border with the desired color and width
    QPen borderPen(Qt::white);
    borderPen.setWidth(borderWidth);
    floorRect1->setPen(borderPen);
    floorRect2->setPen(borderPen);

    setScene(playbackScene);

    // Assuming you have the positions of anchors in a variable named anchor_positions
    //    for (int i = 0; i < tagData.anchor_positions.rows(); ++i) {
    //        QGraphicsEllipseItem* anchor = new QGraphicsEllipseItem(scaleX*(tagData.anchor_positions(i, 0)) - 5, scaleY*(tagData.anchor_positions(i, 1)) - 5, 10, 10);
    //        anchor->setBrush(Qt::red);
    //        scene->addItem(anchor);
    //    }

    // Create and display crew member objects

    int outerMargin = 5;

    // Create fonts
    QFont legendFont("Source Sans Pro", 10);
    QFont floorFont("Source Sans Pro", 12);
    QFont timeFont("Source Sans Pro", 14);
    floorFont.setBold(true);

    // Create and display legend
    int legendWidth = 80;
    int legendHeight = 84;
    // Crew member circles and text for legend
    int circleRadius = 10;
    int circleMargin = 8;
    int textMargin = 2;
    for (unsigned long long i = 0; i < crewMembers.size(); ++i) {
        QGraphicsEllipseItem* crewCircle = new QGraphicsEllipseItem(outerMargin + circleMargin, rectLength - outerMargin - legendHeight + circleMargin + (circleRadius + circleMargin)*i, circleRadius, circleRadius);
        crewCircle->setBrush(QBrush(QColor(QString::fromStdString(crewColors[i]))));
        playbackScene->addItem(crewCircle);
        QGraphicsTextItem *crewText = playbackScene->addText(QString::fromStdString("TAG " + std::to_string(i+1)));
        crewText->setDefaultTextColor(QColor(QString::fromStdString(crewColors[i])));
        crewText->setFont(legendFont);
        crewText->setPos(outerMargin + circleMargin + circleRadius + textMargin, rectLength - outerMargin - legendHeight + (circleRadius + circleMargin)*i + 2);
    }

    // Label floor 1 and 2
    QGraphicsTextItem *floor1Text = playbackScene->addText("Floor 1");
    floor1Text->setDefaultTextColor(Qt::white);
    floor1Text->setFont(floorFont);
    floor1Text->setPos(outerMargin, outerMargin);
    QGraphicsTextItem *floor2Text = playbackScene->addText("Floor 2");
    floor2Text->setDefaultTextColor(Qt::white);
    floor2Text->setFont(floorFont);
    floor2Text->setPos(rectWidth+margin+outerMargin, +outerMargin);

    // Create text item for current time
    //timeText = scene->addText(QString::fromStdString(currentFrame));
    //timeText->setDefaultTextColor(Qt::white);
    //timeText->setFont(timeFont);
    //timeText->setPos((2*rectWidth+margin)/2 - timeText->boundingRect().width()/2, rectLength + 20);
}

void Playback::loadCSV(const QString& csvFilePath) {
    QFile file(csvFilePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        std::cerr << "Error opening file: " << csvFilePath.toStdString() << std::endl;
        return;
    }

    QTextStream in(&file);
    bool isFirstLine = true;
    playbackData.clear();  // Clear existing data

    while (!in.atEnd()) {
        QString line = in.readLine();
        if (isFirstLine) {
            isFirstLine = false; // Skip header line
            continue;
        }

        QStringList values = line.split(',');
        if (values.size() < 3) {  // Assuming each line has at least 3 values (x, y, z)
            std::cerr << "Invalid line format: " << line.toStdString() << std::endl;
            continue;
        }

        std::vector<double> positionData;
        for (const QString& value : values) {
            bool ok;
            double num = value.toDouble(&ok);
            if (!ok) {
                std::cerr << "Invalid number format: " << value.toStdString() << std::endl;
                break; // Skip this line if invalid number format
            }
            positionData.push_back(num);
        }
        if (positionData.size() == static_cast<unsigned long long>(values.size())) {
            playbackData.push_back(positionData);
        }
    }

    if (playbackData.empty()) {
        std::cerr << "No valid data loaded from file: " << csvFilePath.toStdString() << std::endl;
    } else {
        currentFrame = 0; // Reset to the beginning of the data
        std::cout << "Loaded " << playbackData.size() << " data points." << std::endl;
    }
}

void Playback::startPlayback() {
    if (!isPlaying && !playbackData.empty()) {
        isPlaying = true;
        playbackTimer->start(1000);
    }
}

void Playback::stopPlayback() {
    if (isPlaying) {
        isPlaying = false;
        playbackTimer->stop();
        currentFrame = 0;
    }
}


void Playback::fastForward() {
    // Move forward in the position data
    currentFrame += 30;
    if(static_cast<unsigned long long>(currentFrame) >= playbackData.size()){
        stopPlayback();
    }
}

void Playback::reverse() {
    currentFrame -= 30;
    if(currentFrame < 0){
        stopPlayback();
    }
}

void Playback::updatePositions() {
    if(static_cast<unsigned long long>(currentFrame) < playbackData.size()) {
        for (size_t i = 0; i < crewMembers.size(); ++i) {
            int dataIndex = i * 3; // Assuming 3 values per crew member: x, y, z
            crewMembers[i]->updatePosition(playbackData[currentFrame][dataIndex],
                                           playbackData[currentFrame][dataIndex + 1],
                                           playbackData[currentFrame][dataIndex + 2], 0);
        }
        currentFrame++; // Move to the next frame
    } else {
        stopPlayback(); // Stop playback if end of data is reached
    }
}

// Other necessary methods

