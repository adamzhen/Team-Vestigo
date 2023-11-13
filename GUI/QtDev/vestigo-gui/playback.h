#ifndef PLAYBACK_H
#define PLAYBACK_H

#include <QObject>
#include <QString>
#include <QList>
#include <QTimer>
#include "crewmember.h"

class Playback : public QGraphicsView {

    Q_OBJECT

public:
    Playback(QGraphicsView *parent = nullptr);

    QGraphicsTextItem *timeText;

    int rectWidth = 500;
    int rectLength = 500;
    int margin = 20;

    void loadCSV(const QString& csvFilePath);
    void startPlayback();
    void stopPlayback();
    void fastForward();
    void reverse();

signals:
    void positionUpdated(const QString& positionData);
    void playbackEnded();

public slots:
    void updatePositions();

private:
    QGraphicsScene* scene;

    QString csvFilePath;
    int currentFrame;
    bool isPlaying;
    QTimer playbackTimer;

    std::vector<std::vector<double>> tagData;

    std::vector<CrewMember*> crewMembers;  // Vector of playback CrewMember objects
    std::vector<std::string> crewColors = {"#E07C79", "#B2A2E0", "#96C9E0", "#E0D98D"};
};

#endif // PLAYBACK_H

