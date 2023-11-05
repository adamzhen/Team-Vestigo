#include <iostream>
#include "mapwidget.h"
#include "tagdata.h"

MapWidget::MapWidget(TagData& tagDataInstance, QGraphicsView *parent)
    : QGraphicsView(parent),
    scene(new QGraphicsScene(this)),
    tagData(tagDataInstance)
{

    setScene(scene);

    // Current dimensions of the window or view
    QRectF viewRect = this->viewport()->rect();
    qreal scaleX = viewRect.width() / tagData.room_width;
    qreal scaleY = viewRect.height() / tagData.room_length;

    // Assuming you have the positions of anchors in a variable named anchor_positions
    for (int i = 0; i < tagData.anchor_positions.rows(); ++i) {
        QGraphicsEllipseItem* anchor = new QGraphicsEllipseItem(scaleX*(tagData.anchor_positions(i, 0)) - 5, scaleY*(tagData.anchor_positions(i, 1)) - 5, 10, 10);
        anchor->setBrush(Qt::red);
        scene->addItem(anchor);
    }

    // Create and display crew member objects
    crew_positions = tagData.readTagData();
    for (int i = 0; i < crew_positions.rows(); ++i) {
        CrewMember* crew = new CrewMember(crew_positions(i, 0), crew_positions(i, 1));
        crewMembers.push_back(crew);  // Add to the vector
        scene->addItem(crew);
    }
}


void MapWidget::updateCrewPositions() {

    // Here, fetch or update the crew_positions data
    crew_positions = tagData.readTagData();

    // Current dimensions of the window or view
    QRectF viewRect = this->viewport()->rect();
    qreal scaleX = viewRect.width() / tagData.room_width;
    qreal scaleY = viewRect.height() / tagData.room_length;

    // Update the positions of the crew member objects
    for (int i = 0; i < crew_positions.rows(); ++i) {
        crewMembers[i]->updatePosition(crew_positions(i, 0), crew_positions(i, 1), crew_positions(i, 3), scaleX, scaleY);
    }

    QString locationData = QString("X: %1, Y: %2")
                               .arg(crew_positions(0, 0), 0, 'f', 2)
                               .arg(crew_positions(0, 1), 0, 'f', 2);
    emit locationUpdated(locationData);
}


MapWidget::~MapWidget() {
    for (auto crewMember : crewMembers) {
        delete crewMember;
    }
}

