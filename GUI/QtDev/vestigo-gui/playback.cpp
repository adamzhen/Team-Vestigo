#include "playback.h"
#include <QFile>
#include <QTextStream>
#include <QStringList>
#include <QTimer>

// Constructor
Playback::Playback(QGraphicsView *parent) : QGraphicsView(parent), scene(new QGraphicsScene(this)) {
    // Set the scene's rectangle (where x, y, width, and height are the desired dimensions)
    scene->setSceneRect(0, 0, 2*rectWidth, rectLength);
    scene->setBackgroundBrush(QColor("#000000"));
}

void Playback::loadCSV(const QString& csvFilePath) {
    QFile file(csvFilePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    QTextStream in(&file);
    bool isFirstLine = true;

    while (!in.atEnd()) {
        QString line = in.readLine();
        if (isFirstLine) {
            isFirstLine = false;
            continue;
        }

        QStringList values = line.split(',');

        // Add the position data to the tagData vector
        std::vector<double> positionData;
        for (int i = 0; i < values.size(); ++i) {
            positionData.push_back(values[i].toDouble());
        }
        tagData.push_back(positionData);

    }
}
void Playback::startPlayback() {
    if (!isPlaying) {
        isPlaying = true;
        playbackTimer.start(1000);
    }
}

void Playback::stopPlayback() {
    if (isPlaying) {
        isPlaying = false;
        playbackTimer.stop();
        currentFrame = 0;
    }
}


void Playback::fastForward() {
    // Move forward in the position data
    currentFrame += 30;
    if(static_cast<unsigned long long>(currentFrame) >= tagData.size()){
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
    // Check if the current timestamp is valid for all vectors
    if (static_cast<unsigned long long>(currentFrame) < tagData.size()){

        // Update the positions of the crew member objects
        for (int i = 0; i < 4; ++i) {
            crewMembers[i]->updatePosition(tagData[currentFrame][1+i*3], tagData[currentFrame][2+i*3], tagData[currentFrame][3+i*3], 0);
        }

        // Update current time
        timeText->setPlainText("");

        // Send location data to be displayed on status bar
        QString positionData = QString("TAG 1: %1, %2, %3, TAG 2: %4, %5, %6, TAG 3: %7, %8, %9, TAG 4: %10, %11, %12 | Time: %13")
                                   .arg(tagData[currentFrame][0], 0, 'f', 2)
                                   .arg(tagData[currentFrame][1], 0, 'f', 2)
                                   .arg(tagData[currentFrame][2], 0, 'f', 2)
                                   .arg(tagData[currentFrame][3], 0, 'f', 2)
                                   .arg(tagData[currentFrame][4], 0, 'f', 2)
                                   .arg(tagData[currentFrame][5], 0, 'f', 2)
                                   .arg(tagData[currentFrame][6], 0, 'f', 2)
                                   .arg(tagData[currentFrame][7], 0, 'f', 2)
                                   .arg(tagData[currentFrame][8], 0, 'f', 2)
                                   .arg(tagData[currentFrame][9], 0, 'f', 2)
                                   .arg(tagData[currentFrame][10], 0, 'f', 2)
                                   .arg(tagData[currentFrame][11], 0, 'f', 2)
                                   .arg(tagData[currentFrame][12], 0, 'f', 2);
        emit positionUpdated(positionData);

        // Update the position of each crew member
        // This assumes you have a way to access your crew member objects.
        // Example:
        // crewMember1->updatePosition(tag1Position.x, tag1Position.y, tag1Position.z, tag1Position.yaw);
        // crewMember2->updatePosition(tag2Position.x, tag2Position.y, tag2Position.z, tag2Position.yaw);
        // ... and so on for tag3Position and tag4Position

    } else {
        // End of data reached
        stopPlayback();
        emit playbackEnded();
    }

    // Increment the timestamp for the next update
    currentFrame++;
}

// Other necessary methods

