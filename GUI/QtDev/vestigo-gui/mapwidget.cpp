#include <iostream>
#include <ctime>
#include "mapwidget.h"
#include "tagdata.h"
#include "Eigen/Dense"

using namespace Eigen;

std::string getCurrentTime(){
    /******** TIME TRACKING ********/
    // get the current timestamp
    auto now = std::chrono::system_clock::now();
    // convert the timestamp to a time_t object
    time_t timestamp = std::chrono::system_clock::to_time_t(now);
    // set the desired timezone
    tm timeinfo;
    localtime_s(&timeinfo, &timestamp);

    //    int year = timeinfo.tm_year + 1900;
    //    int month = timeinfo.tm_mon + 1;
    //    int day = timeinfo.tm_mday;
    int hours = timeinfo.tm_hour;
    int minutes = timeinfo.tm_min;
    int seconds = timeinfo.tm_sec;

    // HH:MM:SS
    // get current time and format in HH:MM:SS
    std::ostringstream timeStream;
    timeStream << std::setfill('0') << std::setw(2) << hours << ":"
               << std::setfill('0') << std::setw(2) << minutes << ":"
               << std::setfill('0') << std::setw(2) << seconds;
    // Convert the string stream to a string
    std::string current_time = timeStream.str();
    return current_time;
}

MapWidget::MapWidget(TagData& tagDataInstance, QGraphicsView *parent)
    : QGraphicsView(parent),
    scene(new QGraphicsScene(this)),
    tagData(tagDataInstance)
{

    // Set the scene's rectangle (where x, y, width, and height are the desired dimensions)
    scene->setSceneRect(0, 0, 2*rectWidth, rectLength);
    scene->setBackgroundBrush(QColor("#000000"));

}

void MapWidget::drawBackground() {

    // Create the border as a rectangle item
    // The border's width is the pen width, so we reduce the size of the rect item by twice the pen width
    // to ensure that the border is drawn inside the scene.
    int borderWidth = 1;
    QGraphicsRectItem *floorRect1 = scene->addRect(0, 0, rectWidth - 2 * borderWidth, rectLength - 2 * borderWidth);
    QGraphicsRectItem *floorRect2 = scene->addRect(rectWidth+margin, 0, rectWidth - 2 * borderWidth, rectLength - 2 * borderWidth);

    // Set the pen for the border with the desired color and width
    QPen borderPen(Qt::white);
    borderPen.setWidth(borderWidth);
    floorRect1->setPen(borderPen);
    floorRect2->setPen(borderPen);

    setScene(scene);

    // Assuming you have the positions of anchors in a variable named anchor_positions
    //    for (int i = 0; i < tagData.anchor_positions.rows(); ++i) {
    //        QGraphicsEllipseItem* anchor = new QGraphicsEllipseItem(scaleX*(tagData.anchor_positions(i, 0)) - 5, scaleY*(tagData.anchor_positions(i, 1)) - 5, 10, 10);
    //        anchor->setBrush(Qt::red);
    //        scene->addItem(anchor);
    //    }

    // Create and display crew member objects
    // crew_positions = tagData.readTagData();
    crew_positions.setZero();
    for (int i = 0; i < 4; ++i) {
        CrewMember* crew = new CrewMember(0, 0, crewColors[i], rectWidth, rectLength, tagData.room_width, tagData.room_length, tagData.room_height, margin);
        crewMembers.push_back(crew);  // Add to the vector
        scene->addItem(crew);
    }

    int outerMargin = 5;

    // Create fonts
    QFont legendFont("Source Sans Pro", 10);
    QFont floorFont("Source Sans Pro", 12);
    QFont timeFont("Source Sans Pro", 14);
    floorFont.setBold(true);

    // Create and display legend
    int legendWidth = 80;
    int legendHeight = 84;
    // Crew member circles and text for legend
    int circleRadius = 10;
    int circleMargin = 8;
    int textMargin = 2;
    for (int i = 0; i < crewMembers.size(); ++i) {
        QGraphicsEllipseItem* crewCircle = new QGraphicsEllipseItem(outerMargin + circleMargin, rectLength - outerMargin - legendHeight + circleMargin + (circleRadius + circleMargin)*i, circleRadius, circleRadius);
        crewCircle->setBrush(QBrush(QColor(QString::fromStdString(crewColors[i]))));
        scene->addItem(crewCircle);
        QGraphicsTextItem *crewText = scene->addText(QString::fromStdString("TAG " + std::to_string(i+1)));
        crewText->setDefaultTextColor(QColor(QString::fromStdString(crewColors[i])));
        crewText->setFont(legendFont);
        crewText->setPos(outerMargin + circleMargin + circleRadius + textMargin, rectLength - outerMargin - legendHeight + (circleRadius + circleMargin)*i + 2);
    }

    // Label floor 1 and 2
    QGraphicsTextItem *floor1Text = scene->addText("Floor 1");
    floor1Text->setDefaultTextColor(Qt::white);
    floor1Text->setFont(floorFont);
    floor1Text->setPos(outerMargin, outerMargin);
    QGraphicsTextItem *floor2Text = scene->addText("Floor 2");
    floor2Text->setDefaultTextColor(Qt::white);
    floor2Text->setFont(floorFont);
    floor2Text->setPos(rectWidth+margin+outerMargin, +outerMargin);

    // Create text item for current time
    timeText = scene->addText(QString::fromStdString(getCurrentTime()));
    timeText->setDefaultTextColor(Qt::white);
    timeText->setFont(timeFont);
    timeText->setPos((2*rectWidth+margin)/2 - timeText->boundingRect().width()/2, rectLength + 20);
}
void MapWidget::clearBackground() {
    scene->clear();
    setScene(scene);
    crewMembers.clear();
}

void MapWidget::updateCrewPositions(const Eigen::MatrixXd& raw_data) {
    //std::cout << "updateCrewPositions()" << std::endl;

    // Here, fetch or update the crew_positions data
    crew_positions = tagData.processTagData(raw_data);

    // Current dimensions of the window or view
    // QRectF viewRect = this->viewport()->rect();

    // Update the positions of the crew member objects
    for (int i = 0; i < crew_positions.rows(); ++i) {
        crewMembers[i]->updatePosition(crew_positions(i, 0), crew_positions(i, 1),  crew_positions(i, 2), crew_positions(i, 3));
    }

    // Update current time
    timeText->setPlainText(QString::fromStdString(getCurrentTime()));

    count++;
    // Send location data to be displayed on status bar
    QString locationData = QString("TAG 1: %1, %2, %3, TAG 2: %4, %5, %6, TAG 3: %7, %8, %9, TAG 4: %10, %11, %12 | Count: %13")
                               .arg(crew_positions(0, 0), 0, 'f', 2)
                               .arg(crew_positions(0, 1), 0, 'f', 2)
                               .arg(crew_positions(0, 2), 0, 'f', 2)
                               .arg(crew_positions(1, 0), 0, 'f', 2)
                               .arg(crew_positions(1, 1), 0, 'f', 2)
                               .arg(crew_positions(1, 2), 0, 'f', 2)
                               .arg(crew_positions(2, 0), 0, 'f', 2)
                               .arg(crew_positions(2, 1), 0, 'f', 2)
                               .arg(crew_positions(2, 2), 0, 'f', 2)
                               .arg(crew_positions(3, 0), 0, 'f', 2)
                               .arg(crew_positions(3, 1), 0, 'f', 2)
                               .arg(crew_positions(3, 2), 0, 'f', 2)
                               .arg(count);
    emit locationUpdated(locationData);
}


MapWidget::~MapWidget() {
    for (auto crewMember : crewMembers) {
        delete crewMember;
    }
}

