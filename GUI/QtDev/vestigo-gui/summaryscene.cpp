#include "summaryscene.h"
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QPainter>
#include <QImage>
#include <QDebug>
#include <cmath>
#include <numeric>
#include <QMainWindow>
#include <QString>
#include <QMap>
#include <QVector>
#include <QPointF>
#include <QPaintEvent>
#include <QPushButton>
#include <QLineEdit>
#include <QVBoxLayout>
#include <iostream>
#include <QLabel>

inline bool operator<(const QPointF& a, const QPointF& b) {
    if (a.x() == b.x()) return a.y() < b.y();
    return a.x() < b.x();
}
SummaryScene::SummaryScene(QWidget *parent){
    // Example usage
    setMinimumSize(400,300);
    QString inputCsv = "12Hr_TestRun.csv";
    QString outputCsv = "percentage_output.csv";
    QString combinedDensityMap = inputCsv + "density_map.png";
    QString referenceImage = "octant_reference.png";
    imageLabel = new QLabel(this);
    imageLabel->setGeometry(10,10,400,400);
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setStyleSheet("QLabel { background-color: white; }");
    calculateAndWriteOctantPercentages(inputCsv, outputCsv);
    createCombinedDensityMapImage(inputCsv, combinedDensityMap);
    createOctantReferenceImage(referenceImage);
}

SummaryScene::~SummaryScene() {

}
void SummaryScene::loadFile(const QString& filename) {
    currentFilename = filename;

    // Process the file and update the visuals
    calculateAndWriteOctantPercentages(currentFilename, currentFilename + "_output.csv");
    createCombinedDensityMapImage(currentFilename, "density_map.png");
    combinedImage.save(currentFilename + "density_map.png");
    QPixmap pixmap(currentFilename + "density_map.png");
    imageLabel->setPixmap(pixmap.scaled(imageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}
QPointF SummaryScene::roundPointToGrid(const QPointF& point, double gridSize) {
    double x = round(point.x() / gridSize) * gridSize;
    double y = round(point.y() / gridSize) * gridSize;
    return QPointF(x, y);
}
int SummaryScene::determineOctant(const QPointF &point) {
    double angle = std::atan2(point.y(), point.x());
    if (angle < 0) angle += 2 * M_PI; // Adjust angle to be in the range [0, 2*pi]
    int octant = static_cast<int>(8 * angle / (2 * M_PI)) + 1; // Calculate octant (1-8)
    return octant;
}
void SummaryScene::calculateAndWriteOctantPercentages(const QString &inputFilename, const QString &outputFilename) {
    QFile inputFile(inputFilename);
    if (!inputFile.open(QIODevice::ReadOnly)) {
        qDebug() << "Failed to open input file";
        return;
    }

    QFile outputFile(outputFilename);
    if (!outputFile.open(QIODevice::WriteOnly)) {
        qDebug() << "Failed to open output file";
        return;
    }

    QTextStream in(&inputFile);
    QTextStream out(&outputFile);

    // Headers for the output CSV
    out << "Crew Member,Octant,Percentage\n";

    // Initialize variables to track time spent in each octant for each crew member
    const int numCrew = 4; // Assuming 4 crew members
    const int numOctants = 16;
    QVector<QMap<int, int>> timeInOctantPerCrew(numCrew);
    QMap<QPair<int, int>, QMap<int, int>> sharedOctantCounts; // Shared octant counts

    while (!in.atEnd()) {
        QString line = in.readLine();
        QStringList fields = line.split(",");

        if (fields.length() < 12) { // Assuming 3 fields per crew member and at least 4 crew members
            continue; // Skip lines with not enough data
        }

        // Process each crew member's data
        QVector<int> octantsForCrew(4, 0); // Vector to store octants for each crew member

        for (int i = 0; i < 4; ++i) {
            QPointF point(fields.at((i * 3) + 1).toDouble(), fields.at((i * 3) + 2).toDouble());
            double z = fields.at((i * 3) + 3).toDouble(); // Z-coordinate to determine the floor
            int baseOctant = determineOctant(point);
            int octant = z <= 0.5 ? baseOctant : baseOctant + 8; // Octants 9-16 for the second floor
            octantsForCrew[i] = octant;
            timeInOctantPerCrew[i][octant]++; // Increment time for this crew member in this octant
        }

        // Compare octants and increment counts for shared octants
        for (int i = 0; i < 4; ++i) {
            for (int j = i + 1; j < 4; ++j) { // Compare each pair only once
                if (octantsForCrew[i] == octantsForCrew[j]) {
                    QPair<int, int> pair(i + 1, j + 1); // Crew member numbers
                    int octant = octantsForCrew[i];
                    sharedOctantCounts[pair][octant]++; // Increment count for this pair and octant
                }
            }
        }
    }
    QVector<QVector<double>> percentages(numCrew, QVector<double>(numOctants, 0.0));
    for (int crewIndex = 0; crewIndex < numCrew; ++crewIndex) {
        int totalTime = std::accumulate(timeInOctantPerCrew[crewIndex].begin(), timeInOctantPerCrew[crewIndex].end(), 0);
        for (auto octant : timeInOctantPerCrew[crewIndex].keys()) {
            percentages[crewIndex][octant - 1] = (totalTime > 0) ? (100.0 * timeInOctantPerCrew[crewIndex][octant] / totalTime) : 0.0;
        }
    }

    // Write headers for the new CSV format
    out << "Crew Member";
    for (int octant = 1; octant <= numOctants; ++octant) {
        out << ",Octant " << octant;
    }
    out << "\n";

    // Write the data in the new format
    for (int crewIndex = 0; crewIndex < numCrew; ++crewIndex) {
        out << "Person " << crewIndex + 1;
        for (int octant = 0; octant < numOctants; ++octant) {
            out << "," << QString::number(percentages[crewIndex][octant], 'f', 2);
        }
        out << "\n";
    }

    // Write shared octant information
    out << "\nShared Octant Time\n";
    out << "Crew Pair";

    // Prepare headers for shared octants for all crew pairs
    for (int i = 0; i < 4; ++i) {
        for (int j = i + 1; j < 4; ++j) {
            out << QString(",%1 & %2").arg(i + 1).arg(j + 1);
        }
    }
    out << "\n";

    // Write data for each octant
    for (int octant = 1; octant <= 16; ++octant) {
        out << QString("Octant %1").arg(octant);

        // Write shared time percentages for this octant for all crew pairs
        for (int i = 0; i < 4; ++i) {
            for (int j = i + 1; j < 4; ++j) {
                QPair<int, int> pair(i + 1, j + 1);
                const QMap<int, int>& counts = sharedOctantCounts.value(pair);
                int count = counts.value(octant, 0);
                double percentage = (count > 0) ? (100.0 * count) / std::accumulate(counts.begin(), counts.end(), 0) : 0.0;
                out << QString(",%1").arg(percentage, 0, 'f', 2);
            }
        }
        out << "\n";
    }

    inputFile.close();
    outputFile.close();
}
void SummaryScene::update() {
    qDebug() << "Custom update called";
    repaint(); // Force a repaint
}

void SummaryScene::paintEvent(QPaintEvent *event) {
    QPainter painter(this);

    qDebug() << "Paint event called with rect:" << this->rect();

    if (!combinedImage.isNull()) {
        qDebug() << "Drawing image";
        painter.drawImage(this->rect(), combinedImage);
    } else {
        qDebug() << "Drawing fallback content";
        // Drawing a simple rectangle as fallback
        painter.setPen(Qt::blue);
        painter.drawRect(10, 10, 100, 100);
    }
}

void SummaryScene::drawDensityMap(QPainter &painter, const QSize &windowSize) {
    double dataSpan = 10.0; // Your data spans from -5 to 5.

    // Calculate scale factors to map the data range to the window size
    double scaleX = windowSize.width() / dataSpan;
    double scaleY = windowSize.height() / dataSpan;

    // Draw each data point scaled to the window size
    painter.setPen(Qt::red);
    for (const auto& point : dataPoints) {
        // Shift the data points so that the center of the data (0,0) maps to the center of the window
        // Then scale the coordinates to fit the window size
        double x = (point.x() + 5) * scaleX; // Shift by adding 5 to move from [-5,5] to [0,10] then scale
        double y = (point.y() + 5) * scaleY; // Shift by adding 5 to move from [-5,5] to [0,10] then scale

        // Flip the y-coordinate to account for the Qt coordinate system
        y = windowSize.height() - y;

        painter.drawPoint(x, y);
    }
}
void SummaryScene::createCombinedDensityMapImage(const QString &inputFilename, const QString &combinedImageFilename) {
    QFile file(inputFilename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Unable to open the file for reading";
        return;
    }

    QTextStream in(&file);
    QString line = in.readLine(); // Skip the header line

    const double maxBuildingDimension = 10.0; // Data spans from -5 to 5, hence 10 units
    const int singleImageSize = 500; // Size of the image for a single floor
    const int titleSpace = 50; // Space for the title
    const int combinedImageWidth = singleImageSize * 2; // Double the width to place images side by side
    combinedImage = QImage(combinedImageWidth, singleImageSize + titleSpace, QImage::Format_ARGB32);
    combinedImage.fill(Qt::black);

    QPainter painter(&combinedImage);
    painter.setRenderHint(QPainter::Antialiasing); // Optional, for smoother edges

    // Set up the font for the titles
    QFont font = painter.font();
    font.setPointSize(16); // Larger font size for the titles
    painter.setFont(font);
    QPen textPen(Qt::white);
    painter.setPen(textPen);

    // Draw titles
    painter.drawText(QRect(0, 0, singleImageSize, titleSpace), Qt::AlignCenter, "Floor 1");
    painter.drawText(QRect(singleImageSize, 0, singleImageSize, titleSpace), Qt::AlignCenter, "Floor 2");

    // Set up the pen for the outline of the floor circles
    QPen floorOutlinePen(Qt::white);
    floorOutlinePen.setWidth(2); // Set the width of the outline for the floors
    painter.setPen(floorOutlinePen);

    // Draw the large circles for the floor outlines
    painter.drawEllipse(QRect(0, titleSpace, singleImageSize, singleImageSize)); // Floor 1
    painter.drawEllipse(QRect(singleImageSize, titleSpace, singleImageSize, singleImageSize)); // Floor 2

    // Set the pen for the density points
    painter.setPen(Qt::NoPen);

    // Set up the brush for the density points (blue fill)
    QColor fillColor(Qt::green);
    painter.setBrush(fillColor);

    QMap<QPointF, int> firstFloorFrequencyMap;
    QMap<QPointF, int> secondFloorFrequencyMap;

    while (!in.atEnd()) {
        line = in.readLine();
        QStringList fields = line.split(",");
        for (int i = 1; i <= 4; ++i) {
            double x = (fields.at((i - 1) * 3 + 1).toDouble() + 5) / maxBuildingDimension * singleImageSize;
            double y = (fields.at((i - 1) * 3 + 2).toDouble() + 5) / maxBuildingDimension * singleImageSize;
            double z = fields.at((i - 1) * 3 + 3).toDouble(); // Z-coordinate to determine the floor

            // Adjust y-coordinate for title space
            y += titleSpace;

            // Round the point to the nearest grid point of size 0.1
            QPointF roundedPoint = roundPointToGrid(QPointF(x, y), 1.0);

            // Use the rounded point for mapping
            QPointF point = z <= 0.5 ? roundedPoint : QPointF(roundedPoint.x() + singleImageSize, roundedPoint.y()); // Place second floor on the right side

            QMap<QPointF, int> &frequencyMap = z <= 0.5 ? firstFloorFrequencyMap : secondFloorFrequencyMap;
            frequencyMap[point] += 1;
        }
    }

    // Draw points for the first and second floors
    for (int floor = 0; floor < 2; ++floor) {
        const QMap<QPointF, int> &frequencyMap = floor == 0 ? firstFloorFrequencyMap : secondFloorFrequencyMap;
        for (auto it = frequencyMap.constBegin(); it != frequencyMap.constEnd(); ++it) {
            fillColor.setAlpha(qMin(it.value() * 10, 255)); // Adjust the alpha based on frequency
            painter.setBrush(fillColor); // Set the brush for the fill color

            // Draw the filled ellipse (density point)
            painter.drawEllipse(it.key(), 2, 2);
        }
    }

    file.close();
    combinedImage.save(combinedImageFilename, "PNG");
}

void SummaryScene::createOctantReferenceImage(const QString &imageFilename) {
    const int imageSize = 600; // Increase the image size for more space
    const int circleRadius = imageSize / 5; // Radius for each circle
    QImage image(imageSize, imageSize, QImage::Format_ARGB32);
    image.fill(Qt::white);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::black);
    painter.setBrush(Qt::NoBrush);

    // Set up the font for the title and labels
    QFont font = painter.font();
    font.setPointSize(12); // Point size for the title
    painter.setFont(font);

    // Draw the title
    QRectF titleRect(0, 10, imageSize, 40); // Position the title at the top
    painter.drawText(titleRect, Qt::AlignCenter, "Octant Overview");

    // Adjust the font size for the labels
    font.setPointSize(10);
    painter.setFont(font);

    // Define the centers of the two circles
    QPointF center1(imageSize / 4, imageSize / 2);
    QPointF center2(3 * imageSize / 4, imageSize / 2);

    // Draw the circles for each floor
    painter.drawEllipse(center1, circleRadius, circleRadius);
    painter.drawEllipse(center2, circleRadius, circleRadius);

    // Function to draw octants for a given center
    auto drawOctants = [&](const QPointF& center, int floor) {
        for (int i = 0; i < 8; ++i) {
            double angle = (2 * M_PI / 8) * i;
            QPointF edge(center.x() + circleRadius * std::cos(angle), center.y() + circleRadius * std::sin(angle));
            painter.drawLine(center, edge);
        }

        for (int i = 0; i < 8; ++i) {
            double angle = (2 * M_PI / 8) * i + (2 * M_PI / 16); // Offset by half an octant to center text
            QPointF textPos(center.x() + (circleRadius * 0.7) * std::cos(angle), center.y() + (circleRadius * 0.7) * std::sin(angle));
            QString number = QString::number(floor * 8 + i + 1);
            QRectF textRect(textPos.x() - 15, textPos.y() - 15, 30, 30); // Adjust text positioning
            painter.drawText(textRect, Qt::AlignCenter, number);
        }
    };

    // Draw and label octants for each floor
    drawOctants(center1, 0); // First floor, octants 1-8
    drawOctants(center2, 1); // Second floor, octants 9-16

    // Add floor labels
    painter.setPen(Qt::blue);
    QRectF floor1LabelRect(center1.x() - circleRadius, center1.y() - circleRadius - 30, circleRadius * 2, 20);
    QRectF floor2LabelRect(center2.x() - circleRadius, center2.y() - circleRadius - 30, circleRadius * 2, 20);
    painter.drawText(floor1LabelRect, Qt::AlignCenter, "Floor 1");
    painter.drawText(floor2LabelRect, Qt::AlignCenter, "Floor 2");

    painter.end();
    image.save(imageFilename, "PNG");
}



