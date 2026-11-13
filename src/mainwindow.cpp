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
#include <QRegularExpression>
#include <QPainter>
#include <cmath>
#include <functional>

// Draws a 64x64 icon in fixed colors, so it stays visible in both light and dark themes
static QIcon paintIcon(const std::function<void(QPainter &)> &draw) {
    QPixmap pixmap(64, 64);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    draw(painter);
    return QIcon(pixmap);
}

// A sheet of paper, shared by the read input / read output icons
static void drawPage(QPainter &painter) {
    painter.setPen(QPen(QColor(120, 120, 120), 3));
    painter.setBrush(Qt::white);
    painter.drawRoundedRect(QRectF(12, 4, 40, 56), 4, 4);
}

// Show a status bar label only while it has text
static void setStatusText(QLabel *label, const QString& text) {
    label->setText(text);
    label->setVisible(!text.isEmpty());
}

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
    inputX->setRange(0, WIDTH); // Points must lie on the canvas
    xLayout->addWidget(inputX);
    inputGroupLayout->addLayout(xLayout);

    // Input Y Coordinate
    QHBoxLayout *yLayout = new QHBoxLayout();
    yLayout->addWidget(new QLabel("Y:"));
    inputY = new QSpinBox(this);
    inputY->setRange(0, HEIGHT);
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
    stepLabel = new QLabel(this); // Which step of the merge is shown, kept off the canvas
    stepLabel->setContentsMargins(0, 0, 8, 0); // Space before the separator
    caseLabel = new QLabel(this); // Which test case of the input file is shown
    caseLabel->setContentsMargins(0, 0, 8, 0);
    statusBar()->addPermanentWidget(stepLabel);
    statusBar()->addPermanentWidget(caseLabel);
    stepLabel->hide(); // Empty labels stay hidden, so the status bar shows no empty separators
    caseLabel->hide();
    statusBar()->addPermanentWidget(coordLabel);

    // Connect mouse move signal from Canvas to update coordinates in StatusBar
    connect(canvas, &CanvasWidget::mouseMoved, this, &MainWindow::updateMousePos);
    connect(canvas, &CanvasWidget::stepChanged, this, [this](const QString& description) { setStatusText(stepLabel, description); });

    // 6. Initialize Actions and Toolbars
    createActions();
    createMenus();
    createToolBar();

    setWindowTitle(tr("Voronoi Diagram"));
}

MainWindow::~MainWindow() {}

void MainWindow::updateMousePos(QPointF pos) {
    // Rounded the same way as a clicked point
    QString text = QString("Pos: (%1, %2)").arg(qRound(pos.x())).arg(qRound(pos.y()));
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
    readInputAct->setIcon(paintIcon([](QPainter &p) {
        drawPage(p);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(30, 136, 229));
        p.drawEllipse(QPointF(24, 20), 5, 5);
        p.drawEllipse(QPointF(40, 28), 5, 5);
        p.drawEllipse(QPointF(27, 45), 5, 5);
    }));
    readInputAct->setStatusTip(tr("Read input text"));
    connect(readInputAct, &QAction::triggered, this, &MainWindow::onReadInput);

    // Read Output Text
    readOutputAct = new QAction(tr("Read output text"), this);
    readOutputAct->setIcon(paintIcon([](QPainter &p) {
        drawPage(p);
        p.setPen(QPen(QColor(229, 57, 53), 4, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(QPointF(32, 34), QPointF(18, 16));
        p.drawLine(QPointF(32, 34), QPointF(46, 18));
        p.drawLine(QPointF(32, 34), QPointF(32, 52));
    }));
    readOutputAct->setStatusTip(tr("Read output text"));
    connect(readOutputAct, &QAction::triggered, this, &MainWindow::onReadOutput);

    // Next test case of the input file
    nextCaseAct = new QAction(tr("Next test case"), this);
    nextCaseAct->setIcon(paintIcon([](QPainter &p) {
        p.setBrush(QColor(251, 140, 0)); // Orange, so it is not mistaken for the blue Step icon
        p.drawPolygon(QPolygonF({QPointF(6, 12), QPointF(30, 32), QPointF(6, 52)}));
        p.drawPolygon(QPolygonF({QPointF(32, 12), QPointF(56, 32), QPointF(32, 52)}));
    }));
    nextCaseAct->setStatusTip(tr("Next test case"));
    nextCaseAct->setEnabled(false); // Enabled after reading an input file with more test cases
    connect(nextCaseAct, &QAction::triggered, this, &MainWindow::onNextTestCase);

    // Save
    saveAct = new QAction(tr("&Save"), this);
    saveAct->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
    saveAct->setStatusTip(tr("Save"));
    connect(saveAct, &QAction::triggered, this, &MainWindow::saveFile);

    // Step by step
    stepAct = new QAction(tr("&Step"), this);
    stepAct->setIcon(paintIcon([](QPainter &p) {
        p.setBrush(QColor(30, 136, 229));
        p.drawPolygon(QPolygonF({QPointF(8, 10), QPointF(40, 32), QPointF(8, 54)}));
        p.drawRoundedRect(QRectF(44, 10, 10, 44), 2, 2);
    }));
    stepAct->setStatusTip(tr("Step by step"));
    connect(stepAct, &QAction::triggered, canvas, &CanvasWidget::stepByStep);

    // Run
    runAct = new QAction(tr("&Run"), this);
    runAct->setIcon(paintIcon([](QPainter &p) {
        p.setBrush(QColor(67, 160, 71));
        p.drawPolygon(QPolygonF({QPointF(14, 8), QPointF(56, 32), QPointF(14, 56)}));
    }));
    runAct->setStatusTip(tr("Run"));
    connect(runAct, &QAction::triggered, canvas, &CanvasWidget::runCalculation);

    // Clear
    clearAct = new QAction(tr("&Clear"), this);
    clearAct->setIcon(style()->standardIcon(QStyle::SP_DialogResetButton));
    clearAct->setStatusTip(tr("Clear"));
    connect(clearAct, &QAction::triggered, this, &MainWindow::onClear);
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
    toolBar->addAction(nextCaseAct);
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
        QMessageBox::warning(this, tr("Error"), tr("Cannot open file for reading"));
        return;
    }

    // The first non-empty line of an output file starts with "P"
    QTextStream checkIn(&checkFile);
    QString firstLine;
    while (firstLine.isEmpty() && !checkIn.atEnd())
        firstLine = checkIn.readLine().trimmed();
    checkFile.close();

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Error"), tr("Cannot open file for reading"));
        return;
    }
    if (firstLine.startsWith('P')) readOutputFormat(&file);
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

    // Read all test cases first, then show them one at a time
    resetTestCases();

    while (!textStream.atEnd()) {
        // 1. Loop until we find the next number "n"
        ok = false;
        while (!textStream.atEnd()) {
            // Ignore comments (also at the end of a line) and empty lines
            QString line = textStream.readLine().section('#', 0, 0).trimmed();
            if (line.isEmpty()) continue;

            // Non-comment line, try to convert it to an integer
            n = line.toInt(&ok);
            if (ok) break;
        }

        // No more test cases (file ends without "0")
        if (!ok) break;

        // 2. Number of points is zero, file test stopped (shown after the last test case)
        if (n == 0) {
            zeroPending = true;
            break;
        }

        // 3. Read n points
        QVector<QPointF> points;

        while (points.size() < n && !textStream.atEnd()) {
            // Coordinates may be separated by spaces or tabs
            QString dataLine = textStream.readLine().section('#', 0, 0).trimmed();
            QStringList parts = dataLine.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);

            if (parts.size() >= 2) {
                bool okX, okY;
                double x = parts[0].toDouble(&okX), y = parts[1].toDouble(&okY);
                if (okX && okY)
                    points.append(QPointF(x, y));
            }
        }
        testCases.append(points);
    }

    // 4. Show the first test case
    onNextTestCase();
}

void MainWindow::onNextTestCase() {
    if (currentCase + 1 < testCases.size()) {
        ++currentCase;
        canvas->setPoints(testCases[currentCase]);
        canvas->runCalculation();
        setStatusText(caseLabel, tr("Test case %1 / %2").arg(currentCase + 1).arg(testCases.size()));
    }
    else if (zeroPending) {
        zeroPending = false;
        QMessageBox::information(this, tr("Test Finished"), tr("讀入點數為零，檔案測試停止"));
    }

    // Stays enabled after the last test case if the stop message is still to be shown
    nextCaseAct->setEnabled(currentCase + 1 < testCases.size() || zeroPending);
}

void MainWindow::resetTestCases() {
    testCases.clear();
    currentCase = -1;
    zeroPending = false;
    setStatusText(caseLabel, QString());
    nextCaseAct->setEnabled(false);
}

void MainWindow::onClear() {
    resetTestCases();
    canvas->clearCanvas();
}

void MainWindow::readOutputFormat(QFile *file) {
    QVector<QPointF> points;
    QVector<QLineF> edges;
    QTextStream textStream(file);
    resetTestCases(); // The canvas no longer shows a test case of the input file

    while (!textStream.atEnd()) {
        QString line = textStream.readLine().trimmed();
        QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (parts.isEmpty()) continue;
        else if (parts[0] == "P" && parts.size() >= 3)
            points.append(QPointF(parts[1].toDouble(), parts[2].toDouble()));
        else if (parts[0] == "E" && parts.size() >= 5) {
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
        QMessageBox::warning(this, tr("Error"), tr("Cannot open file for writing"));
        return;
    }

    // Sort points by lexical order, a repeated point is written once
    QVector<QPointF> points = VoronoiDiagram::sortAndUnique(canvas->getPoints());
    QVector<QLineF> edges = canvas->getEdges();
    QTextStream out(&file);

    for (const QPointF& p : std::as_const(points))
        out << "P " << p.x() << " " << p.y() << "\n";

    // Clip segments to the canvas border, coordinates are truncated to integers (as in the spec example).
    // A clipped end point is computed from a far away ray end and is off by about 1e-6, while border
    // crossings of integer points are multiples of 1/1220, so it gets a larger tolerance
    const QRectF outputRect(0, 0, WIDTH, HEIGHT);
    auto toInt = [](double v, double tolerance) { return (int)std::floor(v + tolerance); };
    QVector<QLineF> outEdges;
    for (const QLineF& e : std::as_const(edges)) {
        QLineF clipped;
        if (!VoronoiDiagram::clipLine(e, outputRect, clipped)) continue;
        double tol1 = (clipped.p1() == e.p1()) ? 1e-6 : 1e-4;
        double tol2 = (clipped.p2() == e.p2()) ? 1e-6 : 1e-4;
        QPointF p1(toInt(clipped.x1(), tol1), toInt(clipped.y1(), tol1)), p2(toInt(clipped.x2(), tol2), toInt(clipped.y2(), tol2));
        if (p1 == p2) continue; // Only touches the canvas

        // Sort segments by lexical order
        if (!VoronoiDiagram::comparePoints(p1, p2))
            std::swap(p1, p2);
        outEdges.append(QLineF(p1, p2));
    }
    std::sort(outEdges.begin(), outEdges.end(), VoronoiDiagram::compareEdges);
    for (const QLineF& e : std::as_const(outEdges)) {
        QPointF p1 = e.p1(), p2 = e.p2();
        out << "E " << (int)p1.x() << " " << (int)p1.y() << " " << (int)p2.x() << " " << (int)p2.y() << "\n";
    }
}
