#include "summaryscene.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    SummaryScene w;
    w.show();
    return a.exec();
}
