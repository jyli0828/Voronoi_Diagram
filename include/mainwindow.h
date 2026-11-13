// $LAN=C++$
// File: mainwindow.h
// Author: 李俊諺 Jun-Yan Li
// Student ID: M143040074
// Date: 2025/12/5

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "canvaswidget.h"
#include <QMainWindow>
#include <QListWidget>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>

// Forward declarations
QT_BEGIN_NAMESPACE
class QAction;
class QFile;
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    /*
    1. Input text file with comment
    2. Input text file without comment
    3. Ouput text file
    */
    void readFile();
    void onReadInput();
    void onReadOutput();
    void onNextTestCase();
    void onClear();
    void saveFile();

    void updateMousePos(QPointF pos);

    void onAddPointClicked();

    void updatePointList(const QVector<QPointF>& points);

private:
    void createActions();
    void createMenus();
    void createToolBar();

    void readInputFormat(QFile *file);
    void readOutputFormat(QFile *file);
    void resetTestCases(); // End the test cases of the last input file

    CanvasWidget *canvas;
    QListWidget *pointListWidget;

    QSpinBox *inputX;
    QSpinBox *inputY;
    QPushButton *btnAddPoint;

    QAction *openAct;
    QAction *readInputAct;
    QAction *readOutputAct;
    QAction *nextCaseAct;
    QAction *saveAct;
    QAction *stepAct;
    QAction *runAct;
    QAction *clearAct;

    QLabel *coordLabel;
    QLabel *caseLabel;
    QLabel *stepLabel;

    // Test cases of the last input file, shown one at a time with "Next test case"
    QVector<QVector<QPointF>> testCases;
    int currentCase = -1;
    bool zeroPending = false; // The file ends with 0, show the stop message after the last test case
};

#endif // MAINWINDOW_H
