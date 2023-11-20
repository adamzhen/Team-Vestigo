#ifndef PLAYBACK_H
#define PLAYBACK_H

#include <QObject>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QString>
#include <vector>
#include <QString>
#include <QList>
#include <QTimer>
#include "crewmember.h"

class CrewMember;

class Playback : public QGraphicsView {

    Q_OBJECT

public:
    explicit Playback(QWidget *parent = nullptr);

    QGraphicsTextItem *timeText;

    int rectWidth = 500;
    int rectLength = 500;
    int margin = 20;
    bool isPlaying;
    bool isCurrentlyPlaying() const;
    void loadCSV(const QString& csvFilePath);
    void startPlayback();
    void stopPlayback();
    void fastForward();
    void reverse();
    void drawPlayback();
    void initializeCrewMembers();

signals:
    void positionUpdated(const QString& positionData);
    void playbackEnded();

public slots:
    void updatePositions();
    void setCurrentFrame(int frame);

private:
    QGraphicsScene* playbackScene;
    QString csvFilePath;
    std::vector<std::vector<double>> playbackData;
    int currentFrame;
    QTimer *playbackTimer;
    std::vector<CrewMember*> crewMembers;  // Vector of playback CrewMember objects
    std::vector<std::string> crewColors = {"#E07C79", "#B2A2E0", "#96C9E0", "#E0D98D"};
};

#endif // PLAYBACK_H

