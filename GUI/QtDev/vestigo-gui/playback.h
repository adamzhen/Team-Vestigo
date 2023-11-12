#ifndef PLAYBACK_H
#define PLAYBACK_H

#include <QObject>
#include <QString>
#include <QList>
#include <QTimer>

// Assuming you have a struct or class to hold position data for each crew member
struct PositionData {
    int crewId;
    double x;
    double y;
    double z;
    double yaw;
};

class Playback : public QObject
{
    Q_OBJECT

public:
    explicit Playback(QObject *parent = nullptr);

    void loadCSV(const QString& csvFilePath);
    void startPlayback();
    void stopPlayback();
    void fastForward();
    void reverse();

signals:
    void positionUpdated(const QList<PositionData>& positions);
    void playbackEnded();

public slots:
    void updatePositions();

private:
    QString csvFilePath;
    int currentTimestamp;
    bool isPlaying;
    QTimer playbackTimer;
    std::vector<PositionData> tag1Data;
    std::vector<PositionData> tag2Data;
    std::vector<PositionData> tag3Data;
    std::vector<PositionData> tag4Data;
};

#endif // PLAYBACK_H


