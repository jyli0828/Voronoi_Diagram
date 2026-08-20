// $LAN=C++$
// File: VoronoiDiagram.h
// Author: 李俊諺 Jun-Yan Li
// Student ID: M143040074
// Date: 2025/12/5

#ifndef VORONOIDIAGRAM_H
#define VORONOIDIAGRAM_H

#include <QVector>
#include <QPointF>
#include <QLineF>

struct VoronoiResult {
    QVector<QLineF> edges; // Voronoi edges
    QVector<QPair<QPointF, QPointF>> endPoints; // Record which two points (p1, p2) generated edges[i]
    QVector<QPointF> convexHull; // Sort in counterclockwise order
};

struct VoronoiStep {
    QVector<QLineF> currentEdges; // All edges at the current step (including left, right, and hyperplanes)
    QVector<QPointF> currentHull; // The merged convext hull, the left one, the right one
    QVector<int> edgeTypes; // Indicates the edge type (0: Normal, 1: Left voronoi, 2: Right voronoi, 3: Hyperplane)
    QString description; // For debugging
    int split; // 0: Merged convex hull, not 0: lefthull = currentHull.mid(0, split), righthull = currentHull.mid(split)
};

// 使 QMap 能以 QPointF 作為 Key (容許些微浮點數誤差)
class PointKey {
public:
    QPointF p;
    PointKey(QPointF _p) : p(_p) {}
    bool operator<(const PointKey& other) const;
};

class VoronoiDiagram {
public:
    // Compare lexicographically
    static bool comparePoints(const QPointF& a, const QPointF& b);
    static bool compareEdges(const QLineF& e1, const QLineF& e2);

    static bool isBoundaryPoint(const QPointF& p);

    static QVector<QPointF> sortAndUnique(const QVector<QPointF>& points);

    static QVector<QLineF> getVoronoiEdges(QVector<QPointF>& points);

    // For step by step
    static QVector<VoronoiStep> steps;
    static void clearSteps() { steps.clear(); }
    static void addStep(const QVector<QLineF>& edges, const QVector<int>& types, const QVector<QPointF>& hull, QString desc = "", int split = 0);

private:
    static VoronoiResult voronoi3(const QVector<QPointF>& points);

    static QLineF getPerpBisector(const QPointF& p1, const QPointF& p2);
    static QPointF calculateCircumcenter(const QPointF& p1, const QPointF& p2, const QPointF& p3);
    static qreal crossProduct(const QPointF& o, const QPointF& a, const QPointF& b);

    static VoronoiResult divideAndConquer(const QVector<QPointF>& points);
    static VoronoiResult merge(const VoronoiResult& left, const VoronoiResult& right);

    static QVector<QPointF> mergeConvexHull(const QVector<QPointF>& left, const QVector<QPointF>& right, const QPair<int, int>& upperTan, const QPair<int, int>& lowerTan);
    static void eliminateEdges(VoronoiResult& res);

    static QPair<int, int> getUpperTangent(const QVector<QPointF>& leftHull, const QVector<QPointF>& rightHull);
    static QPair<int, int> getLowerTangent(const QVector<QPointF>& leftHull, const QVector<QPointF>& rightHull);

    static bool areAllCollinear(const QVector<QPointF>& points);
    static VoronoiResult solveCollinear(const QVector<QPointF>& points);
};

#endif // VORONOIDIAGRAM_H
