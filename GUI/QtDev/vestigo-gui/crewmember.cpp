#include "crewmember.h"
#include "qbrush.h"

CrewMember::CrewMember(qreal x, qreal y, double angle)
    : QGraphicsEllipseItem(x, y, 20, 20), fovAngle(angle) {
    setBrush(Qt::blue);
    setRect(-10, -10, 20, 20); // center the ellipse on the position
    setPos(0,0);

    fov = new QGraphicsPathItem(this);
    updateFOV();
}

void CrewMember::updatePosition(double x, double y, double yaw, qreal scalex, qreal scaley) {
    setPos(scalex*x, scaley*y);
    setRotation(-yaw);  // Qt's rotation is counterclockwise, hence the minus sign
    updateFOV();
}

void CrewMember::updateFOV() {
    QPainterPath path;
    path.moveTo(0, 0);
    path.arcTo(-radius*2, -radius*2, radius*4, radius*4, -fovAngle/2, fovAngle);
    path.lineTo(0, 0);
    fov->setPath(path);
    fov->setBrush(QColor(100, 100, 255, 50)); // semi-transparent blue
}
