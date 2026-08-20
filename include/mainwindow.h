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

    CanvasWidget *canvas;
    QListWidget *pointListWidget;

    QSpinBox *inputX;
    QSpinBox *inputY;
    QPushButton *btnAddPoint;

    QAction *openAct;
    QAction *readInputAct;
    QAction *readOutputAct;
    QAction *saveAct;
    QAction *stepAct;
    QAction *runAct;
    QAction *clearAct;

    QLabel *coordLabel;
};

#endif // MAINWINDOW_H
