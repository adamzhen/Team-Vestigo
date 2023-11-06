#ifndef CREWMEMBER_H
#define CREWMEMBER_H

#include <QGraphicsEllipseItem>
#include <QGraphicsPathItem>
#include <QPainterPath>
#include <QColor>
#include <QApplication>
#include <QApplication>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPixmap>
#include <QTimer>

class CrewMember : public QGraphicsEllipseItem {
public:
    CrewMember(qreal x, qreal y, std::string color, int rect_width, int rect_length, double room_width, double room_length, double room_height, int margin, double angle = 45);

    int rectWidth;
    int rectLength;
    double roomWidth;
    double roomLength;
    double roomHeight;
    double scaleX;
    double scaleY;
    int margin;

    void updatePosition(double x, double y, double z, double yaw);
    void updateFOV();

private:
    QGraphicsPathItem* fov;
    double fovAngle; // Angular width of the FOV in degrees
    double radius = 20.0;
};

#endif // CREWMEMBER_H
