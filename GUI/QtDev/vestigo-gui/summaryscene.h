#ifndef SUMMARYSCENE_H
#define SUMMARYSCENE_H

#include <QMainWindow>
#include <QString>
#include <QMap>
#include <QVector>
#include <QPointF>
#include <QPushButton>
#include <QLineEdit>
#include <QString>
#include <QLabel>

QT_BEGIN_NAMESPACE
namespace Ui { class SummaryScene; }
QT_END_NAMESPACE

class SummaryScene : public QMainWindow
{
    Q_OBJECT

public:
    SummaryScene(QWidget *parent = nullptr);
    ~SummaryScene();

    void processCSV(const QString &inputFilename);
    void calculateAndWriteOctantPercentages(const QString &inputFilename, const QString &outputFilename);
    void createCombinedDensityMapImage(const QString &inputFilename, const QString &imageFilename);
    void createOctantReferenceImage(const QString &imageFilename);
    int determineOctant(const QPointF &point);
    void update();
public slots:
    void loadFile(const QString& filename);
protected:
    void paintEvent(QPaintEvent *event) override;
private:
    QString currentFilename;
    QImage combinedImage;
    QLabel* imageLabel;
    void drawDensityMap(QPainter &painter, const QSize &windowSize);
    QVector<QPointF> dataPoints;
    QPointF roundPointToGrid(const QPointF& point, double gridSize = 0.1);
    Ui::SummaryScene *ui;
};

#endif // SUMMARYSCENE_








