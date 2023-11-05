#
#  QCustomPlot Plot Examples
#

QT       += core gui
QT += widgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

greaterThan(QT_MAJOR_VERSION, 4): CONFIG += c++11
lessThan(QT_MAJOR_VERSION, 5): QMAKE_CXXFLAGS += -std=c++11

TARGET = plot-examples
TEMPLATE = app

SOURCES += main.cpp\
    crewmember.cpp \
           mainwindow.cpp \
         ../RootFinder.cpp \
    mapwidget.cpp \
    tagdata.cpp

HEADERS  += mainwindow.h \
         ../RootFinder.h \
         crewmember.h \
         mapwidget.h \
         tagdata.h


FORMS    += mainwindow.ui

INCLUDEPATH += "../Eigen"

LIBS += -lws2_32
