// $LAN=C++$
// File: canvaswidget.cpp
// Author: 李俊諺 Jun-Yan Li
// Student ID: M143040074
// Date: 2025/12/5

#include "../include/canvaswidget.h"
#include "../include/VoronoiDiagram.h"
#include <QPainter>
#include <utility>

CanvasWidget::CanvasWidget(QWidget *parent) : QWidget(parent) {
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, Qt::white);
    setPalette(pal);
    currentStepIndex = -1;
    setMouseTracking(true);
}

QVector<QLineF> CanvasWidget::getEdges() {
    return VoronoiDiagram::getVoronoiEdges(points);
}

void CanvasWidget::updateState() {
    currentStepIndex = -1;
    repaint();
    emit pointsModified(points);
}

void CanvasWidget::clearCanvas() {
    points.clear();
    edges.clear();
    VoronoiDiagram::clearSteps();
    updateState();
}

void CanvasWidget::setPoints(const QVector<QPointF>& newPoints) {
    points = newPoints;
    edges.clear();
    updateState();
}

void CanvasWidget::setDiagram(const QVector<QPointF>& newPoints, const QVector<QLineF>& newEdges) {
    points = newPoints;
    edges = newEdges;
    updateState();
}

void CanvasWidget::stepByStep() {
    if (points.size() <= 1) return;

    // The first step
    if (VoronoiDiagram::steps.isEmpty() || currentStepIndex >= VoronoiDiagram::steps.size() - 1) {
        edges = VoronoiDiagram::getVoronoiEdges(points);
        currentStepIndex = -1;
    }

    currentStepIndex++;
    repaint();
}

void CanvasWidget::runCalculation() {
    edges = VoronoiDiagram::getVoronoiEdges(points);
    currentStepIndex = VoronoiDiagram::steps.size() - 1;
    repaint();
}

void CanvasWidget::addPoint(const QPointF& p) {
    points.append(p);
    edges.clear();
    updateState();
}

void CanvasWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton)
        addPoint(event->position());
}

void CanvasWidget::mouseMoveEvent(QMouseEvent *event) {
    emit mouseMoved(event->position());
    QWidget::mouseMoveEvent(event);
}

void CanvasWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Draw Points
    QPen pointPen(Qt::black);
    pointPen.setWidth(4);
    pointPen.setCapStyle(Qt::RoundCap);
    painter.setPen(pointPen);
    painter.setBrush(Qt::black);
    painter.drawPoints(points);

    // Draw Lines
    if (currentStepIndex >= 0 && currentStepIndex < VoronoiDiagram::steps.size()) {
        // Step-by-Step
        const VoronoiStep& step = VoronoiDiagram::steps[currentStepIndex];
        QPen linePen;
        linePen.setWidth(2);

        for (int i = 0; i < step.currentEdges.size(); ++i) {
            QLineF line = step.currentEdges[i];
            int type = (i < step.edgeTypes.size()) ? step.edgeTypes[i] : 0;

            switch (type) {
            case 1: // Left voronoi
                linePen.setColor(Qt::blue);
                break;
            case 2: // Right voronoi
                linePen.setColor(Qt::green);
                break;
            case 3: // Hyperplane
                linePen.setColor(Qt::magenta);
                break;
            default: // Result/Normal
                linePen.setColor(Qt::red);
                break;
            }

            painter.setPen(linePen);
            painter.drawLine(line);
        }

        // Convex hull
        QPen hullPen(Qt::cyan);
        hullPen.setWidth(2);
        painter.setPen(hullPen);
        painter.setBrush(Qt::NoBrush);

        auto drawHull = [&](const QVector<QPointF>& hullPoints) {
            if (hullPoints.size() >= 2)
                painter.drawPolygon(QPolygonF(hullPoints));
        };

        if (step.split == 0)
            drawHull(step.currentHull);
        else {
            drawHull(step.currentHull.mid(0, step.split)); // Left
            drawHull(step.currentHull.mid(step.split));    // Right
        }

        // Description
        painter.setPen(Qt::black);
        painter.drawText(10, 20, step.description);
    }
    else {
        // Run
        painter.setPen(QPen(Qt::red, 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawLines(edges);
    }
}
