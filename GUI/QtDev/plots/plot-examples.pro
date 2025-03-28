#
#  QCustomPlot Plot Examples
#

QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

greaterThan(QT_MAJOR_VERSION, 4): CONFIG += c++11
lessThan(QT_MAJOR_VERSION, 5): QMAKE_CXXFLAGS += -std=c++11

TARGET = plot-examples
TEMPLATE = app

SOURCES += main.cpp\
           mainwindow.cpp \
         ../QCustomPlot/qcustomplot.cpp

HEADERS  += mainwindow.h \
         ../QCustomPlot/qcustomplot.h

FORMS    += mainwindow.ui

INCLUDEPATH += "..\Eigen"
INCLUDEPATH += "../QCustomPlot"

LIBS += -lws2_32
