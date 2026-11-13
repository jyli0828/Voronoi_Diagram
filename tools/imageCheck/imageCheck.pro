# Image check tool: draws every test case of an input file on the canvas and checks the image against brute force
QT += core gui widgets
CONFIG += c++17 console release
TARGET = imageCheck

ROOT = $$PWD/../..
INCLUDEPATH += $$ROOT/include
SOURCES += imageCheck.cpp $$ROOT/src/VoronoiDiagram.cpp $$ROOT/src/canvaswidget.cpp
HEADERS += $$ROOT/include/canvaswidget.h
