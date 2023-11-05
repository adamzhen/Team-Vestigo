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
    CrewMember(qreal x, qreal y, double angle = 45);

    void updatePosition(double x, double y, double yaw, qreal scalex, qreal scaley);
    void updateFOV();

private:
    QGraphicsPathItem* fov;
    double fovAngle; // Angular width of the FOV in degrees
    double radius = 20.0;
};

#endif // CREWMEMBER_H
