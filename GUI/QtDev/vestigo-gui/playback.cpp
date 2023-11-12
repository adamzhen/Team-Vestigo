#include "playback.h"
#include <QFile>
#include <QTextStream>
#include <QStringList>
#include <QTimer>

// Constructor
Playback::Playback(QObject *parent) : QObject(parent) {

    connect(&playbackTimer, &QTimer::timeout, this, &Playback::updatePositions); //May need to go to mainwindow??
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

        tag1Data.push_back({1, values[1].toDouble(), values[2].toDouble(), values[3].toDouble(), values[4].toDouble()});
        tag2Data.push_back({2, values[5].toDouble(), values[6].toDouble(), values[7].toDouble(), values[8].toDouble()});
        tag3Data.push_back({3, values[9].toDouble(), values[10].toDouble(), values[11].toDouble(), values[12].toDouble()});
        tag4Data.push_back({4, values[13].toDouble(), values[14].toDouble(), values[15].toDouble(), values[16].toDouble()});
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
        currentTimestamp = 0;
    }
}


void Playback::fastForward() {
    // Move forward in the position data
    currentTimestamp += 30;
    if(static_cast<unsigned long long>(currentTimestamp) >= tag1Data.size()){
        stopPlayback();
    }
}

void Playback::reverse() {
    currentTimestamp -= 30;
    if(currentTimestamp < 0){
        stopPlayback();
    }
}

void Playback::updatePositions() {
    // Check if the current timestamp is valid for all vectors
    if (static_cast<unsigned long long>(currentTimestamp) < tag1Data.size()){

        // Access the position data for each tag
        PositionData tag1Position = tag1Data[currentTimestamp];
        PositionData tag2Position = tag2Data[currentTimestamp];
        PositionData tag3Position = tag3Data[currentTimestamp];
        PositionData tag4Position = tag4Data[currentTimestamp];

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
    currentTimestamp++;
}

// Other necessary methods
