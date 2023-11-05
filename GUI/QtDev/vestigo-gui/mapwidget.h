#ifndef MAPWIDGET_H
#define MAPWIDGET_H

#include <QGraphicsView>
#include <QGraphicsScene>
#include "crewmember.h"
#include "tagdata.h"
#include <Eigen/Dense>
#include <vector>  // Include the header for std::vector

class MapWidget : public QGraphicsView {
    Q_OBJECT

public:
    MapWidget(TagData& tagDataInstance, QGraphicsView *parent = nullptr); // Make the parent argument optional

    ~MapWidget();

    void updateCrewPositions();

private:
    QGraphicsScene* scene;

    TagData& tagData;
    Eigen::Matrix<double, Eigen::Dynamic, 4> crew_positions;  // Assuming you use Eigen; Adjust accordingly

    std::vector<CrewMember*> crewMembers;  // Vector of 4 CrewMember objects
signals:
    void locationUpdated(const QString& locationData);

    // ... [Other member variables and functions]
};

#endif // MAPWIDGET_H
