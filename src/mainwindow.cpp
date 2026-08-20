// $LAN=C++$
// File: mainwindow.cpp
// Author: 李俊諺 Jun-Yan Li
// Student ID: M143040074
// Date: 2025/12/5

#include "../include/mainwindow.h"
#include "../include/VoronoiDiagram.h"
#include <QMenuBar>
#include <QToolBar>
#include <QStyle>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QStatusBar>
#include <QCoreApplication>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {

    // 1. Setup Central Widget
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    // Main Layout (Horizontal)
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);

    // 2. Left Side: Canvas
    canvas = new CanvasWidget(this);
    mainLayout->addWidget(canvas, 1);

    // 3. Right Side: Container for List and Input Area
    QWidget *rightPanel = new QWidget(this);
    rightPanel->setMaximumWidth(200);

    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);

    // 3-1. Point List
    pointListWidget = new QListWidget(this);
    rightLayout->addWidget(pointListWidget, 1);

    // 3-2. Input Area (Group Box)
    QGroupBox *inputGroup = new QGroupBox(tr("Add Point"), this);
    QVBoxLayout *inputGroupLayout = new QVBoxLayout(inputGroup);

    // Input X Coordinate
    QHBoxLayout *xLayout = new QHBoxLayout();
    xLayout->addWidget(new QLabel("X:"));
    inputX = new QSpinBox(this);
    inputX->setRange(-2000, 2000);
    xLayout->addWidget(inputX);
    inputGroupLayout->addLayout(xLayout);

    // Input Y Coordinate
    QHBoxLayout *yLayout = new QHBoxLayout();
    yLayout->addWidget(new QLabel("Y:"));
    inputY = new QSpinBox(this);
    inputY->setRange(-2000, 2000);
    yLayout->addWidget(inputY);
    inputGroupLayout->addLayout(yLayout);

    // Add Button
    btnAddPoint = new QPushButton(tr("Add"), this);
    inputGroupLayout->addWidget(btnAddPoint);

    // Add the group box to the right panel layout
    rightLayout->addWidget(inputGroup, 0);

    // Add the right panel to the main layout
    mainLayout->addWidget(rightPanel, 0);

    // 4. Signal Connections
    // Connect canvas modification signal to update the point list UI
    connect(canvas, &CanvasWidget::pointsModified, this, &MainWindow::updatePointList);

    // Connect "Add" button click signal
    connect(btnAddPoint, &QPushButton::clicked, this, &MainWindow::onAddPointClicked);

    // 5. Status Bar Setup
    coordLabel = new QLabel("Pos: (0, 0)", this);
    coordLabel->setMinimumWidth(150); // Prevent text jitter by setting min width
    statusBar()->addPermanentWidget(coordLabel);

    // Connect mouse move signal from Canvas to update coordinates in StatusBar
    connect(canvas, &CanvasWidget::mouseMoved, this, &MainWindow::updateMousePos);

    // 6. Initialize Actions and Toolbars
    createActions();
    createMenus();
    createToolBar();

    setWindowTitle(tr("Voronoi Diagram"));
}

MainWindow::~MainWindow() {}

void MainWindow::updateMousePos(QPointF pos) {
    QString text = QString("Pos: (%1, %2)").arg((int)pos.x()).arg((int)pos.y());
    coordLabel->setText(text);
}

void MainWindow::onAddPointClicked() {
    double x = inputX->value(), y = inputY->value();
    canvas->addPoint(QPointF(x, y));
}

void MainWindow::updatePointList(const QVector<QPointF>& points) {
    pointListWidget->clear();
    for (int i = 0; i < points.size(); i++) {
        QString str = QString("(%1, %2)").arg(points[i].x()).arg(points[i].y());
        pointListWidget->addItem(str);
    }
}

void MainWindow::createActions() {
    // Open
    openAct = new QAction(tr("&Open"), this);
    openAct->setIcon(style()->standardIcon(QStyle::SP_DialogOpenButton));
    openAct->setStatusTip(tr("Open"));
    connect(openAct, &QAction::triggered, this, &MainWindow::readFile);

    // Read Input Text
    readInputAct = new QAction(tr("Read input text"), this);
    readInputAct->setStatusTip(tr("Read input text"));
    connect(readInputAct, &QAction::triggered, this, &MainWindow::onReadInput);

    // Read Output Text
    readOutputAct = new QAction(tr("Read output text"), this);
    readOutputAct->setStatusTip(tr("Read output text"));
    connect(readOutputAct, &QAction::triggered, this, &MainWindow::onReadOutput);

    // Save
    saveAct = new QAction(tr("&Save"), this);
    saveAct->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
    saveAct->setStatusTip(tr("Save"));
    connect(saveAct, &QAction::triggered, this, &MainWindow::saveFile);

    // Step by step
    stepAct = new QAction(tr("&Step"), this);
    stepAct->setIcon(style()->standardIcon(QStyle::SP_MediaSkipForward));
    stepAct->setStatusTip(tr("Step by step"));
    connect(stepAct, &QAction::triggered, canvas, &CanvasWidget::stepByStep);

    // Run
    runAct = new QAction(tr("&Run"), this);
    runAct->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    runAct->setStatusTip(tr("Run"));
    connect(runAct, &QAction::triggered, canvas, &CanvasWidget::runCalculation);

    // Clear
    clearAct = new QAction(tr("&Clear"), this);
    clearAct->setIcon(style()->standardIcon(QStyle::SP_DialogResetButton));
    clearAct->setStatusTip(tr("Clear"));
    connect(clearAct, &QAction::triggered, canvas, &CanvasWidget::clearCanvas);
}

void MainWindow::createMenus() {
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(readInputAct);
    fileMenu->addAction(readOutputAct);
    fileMenu->addSeparator();
    fileMenu->addAction(saveAct);

    QMenu *runMenu = menuBar()->addMenu(tr("&Execute"));
    runMenu->addAction(stepAct);
    runMenu->addAction(runAct);
    runMenu->addSeparator();
    runMenu->addAction(clearAct);
}

void MainWindow::createToolBar() {
    QToolBar *toolBar = addToolBar(tr("Main Toolbar"));
    toolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);

    toolBar->addAction(openAct);
    toolBar->addAction(saveAct);
    toolBar->addSeparator();

    toolBar->addAction(stepAct);
    toolBar->addAction(runAct);
    toolBar->addAction(clearAct);
}

void MainWindow::readFile() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Read File"), "", tr("Text File (*.txt)"));
    if (fileName.isEmpty()) return;

    QFile checkFile(fileName);
    if (!checkFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Cannot open output file for reading:" << fileName;
        return;
    }

    QTextStream checkIn(&checkFile);
    QString firstLine = checkIn.readLine().trimmed();
    checkFile.close();

    QFile file(fileName);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text) && firstLine[0] == 'P') readOutputFormat(&file);
    else readInputFormat(&file);
}

void MainWindow::onReadInput() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Read Input File"), "", tr("Text File (*.txt)"));
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Error"), tr("Cannot open file for reading"));
        return;
    }

    readInputFormat(&file);
    file.close();
}

void MainWindow::onReadOutput() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Read Output File"), "", tr("Text File (*.txt)"));
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Error"), tr("Cannot open file for reading"));
        return;
    }

    readOutputFormat(&file);
    file.close();
}

void MainWindow::readInputFormat(QFile *file) {
    QTextStream textStream(file);
    int n = -1;
    bool ok = false;

    while (!textStream.atEnd()) {
        // 1. Loop until we find the next number "n"
        while (!textStream.atEnd()) {
            QString line = textStream.readLine().trimmed();

            // Ignore comments and empty lines
            if (line.startsWith("#") || line.isEmpty()) continue;

            // Non-comment line, try to convert it to an integer
            n = line.toInt(&ok);
            if (ok) break;
        }

        // 2. Number of points is zero, file test stopped
        if (n == 0) {
            // Display
            QMessageBox::information(this, tr("Test Finished"), tr("讀入點數為零，檔案測試停止"));
            return;
        }

        // 3. Read n points
        QVector<QPointF> points;
        QStringList pointStrings;

        while (points.size() < n && !textStream.atEnd()) {
            QString dataLine = textStream.readLine().trimmed();
            QStringList parts = dataLine.split(" ", Qt::SkipEmptyParts);

            if (parts.size() >= 2) {
                bool okX, okY;
                double x = parts[0].toDouble(&okX), y = parts[1].toDouble(&okY);
                if (okX && okY) {
                    points.append(QPointF(x, y));
                    pointStrings.append(QString("(%1, %2)").arg(x).arg(y));
                }
            }
        }

        // 4. Update the canvas and pause
        canvas->setPoints(points);
        canvas->runCalculation();
        QCoreApplication::processEvents();

        QString verificationMsg = QString(tr("Read test case: %1 points\n%2\n"
                                             "Press OK to continue the next test case."))
                                            .arg(n)
                                            .arg(pointStrings.join(", "));
        QMessageBox msgBox(this);
        msgBox.setWindowTitle(tr("Test Case Read"));
        msgBox.setText(verificationMsg);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec(); // This line blocks execution until the user presses "OK"
    }
}

void MainWindow::readOutputFormat(QFile *file) {
    QVector<QPointF> points;
    QVector<QLineF> edges;
    QTextStream textStream(file);

    while (!textStream.atEnd()) {
        QString line = textStream.readLine().trimmed();
        QStringList parts = line.split(" ", Qt::SkipEmptyParts);
        if (parts.isEmpty()) continue;
        else if (parts[0] == "P")
            points.append(QPointF(parts[1].toDouble(), parts[2].toDouble()));
        else if (parts[0] == "E") {
            QPointF p1(parts[1].toDouble(), parts[2].toDouble());
            QPointF p2(parts[3].toDouble(), parts[4].toDouble());
            edges.append(QLineF(p1, p2));
        }
    }

    canvas->setDiagram(points, edges); // Pass both points and edges to the canvas
    for (const QPointF& p : std::as_const(points))
        qDebug() << "P:" << p.x() << p.y();
    for (const QLineF& e : std::as_const(edges)) {
        QPointF p1 = e.p1(), p2 = e.p2();
        qDebug() << "E:" << p1.x() << p1.y() << p2.x() << p2.y();
    }
}

void MainWindow::saveFile() {
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"), "", tr("Text File (*.txt)"));
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Cannot open file for writing:" << fileName;
        return;
    }

    QVector<QPointF> points = canvas->getPoints();
    QVector<QLineF> edges = canvas->getEdges();
    QTextStream out(&file);

    // Sort points by lexical order
    std::sort(points.begin(), points.end(), VoronoiDiagram::comparePoints);
    for (const QPointF& p : std::as_const(points))
        out << "P " << p.x() << " " << p.y() << "\n";

    // Sort segments by lexical order
    for (QLineF& e : edges) {
        QPointF p1 = e.p1(), p2 = e.p2();
        if (!VoronoiDiagram::comparePoints(p1, p2))
            e.setPoints(p2, p1);
    }
    std::sort(edges.begin(), edges.end(), VoronoiDiagram::compareEdges);
    for (const QLineF& e : std::as_const(edges)) {
        QPointF p1 = e.p1(), p2 = e.p2();
        out << "E " << p1.x() << " " << p1.y() << " " << p2.x() << " " << p2.y() << "\n";
    }
}
