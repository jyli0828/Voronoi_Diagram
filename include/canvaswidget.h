// $LAN=C++$
// File: canvaswidget.h
// Author: 李俊諺 Jun-Yan Li
// Student ID: M143040074
// Date: 2025/12/5

#ifndef CANVASWIDGET_H
#define CANVASWIDGET_H

#include <QWidget>
#include <QVector>
#include <QPointF>
#include <QLineF>
#include <QPaintEvent>
#include <QMouseEvent>

const int WIDTH = 610, HEIGHT = 610;

class CanvasWidget : public QWidget {
    Q_OBJECT // Qt macros for Signals and Slots

public:
    CanvasWidget(QWidget *parent = nullptr);

    // The canvas size
    QSize minimumSizeHint() const override { return QSize(WIDTH, HEIGHT); }

    QVector<QPointF> getPoints() { return points; }
    QVector<QLineF> getEdges();

signals:
    void pointsModified(const QVector<QPointF>& points);
    void mouseMoved(QPointF pos);

public slots:
    void clearCanvas();

    void setPoints(const QVector<QPointF>& newPoints);
    void setDiagram(const QVector<QPointF>& newPoints, const QVector<QLineF>& newEdges);

    void stepByStep();
    void runCalculation();

    void addPoint(const QPointF& p);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    QVector<QPointF> points;
    QVector<QLineF> edges;

    int currentStepIndex;

    void updateState();
};

#endif // CANVASWIDGET_H
