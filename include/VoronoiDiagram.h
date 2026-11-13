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
#include <QRectF>
#include <array>

// Points are referred to by their index in the sorted points
struct VoronoiResult {
    QVector<QLineF> edges; // Voronoi edges
    QVector<QPair<int, int>> sites; // The two points that generated edges[i]
    // Edges of a region in counterclockwise order: next[i][k] (prev[i][k]) is the next (previous) edge
    // around the region of sites[i].first (k = 0) or sites[i].second (k = 1)
    QVector<std::array<int, 2>> next, prev;
    int firstSite = 0; // The points are firstSite, firstSite + 1, ...
    QVector<int> anyEdge; // anyEdge[s - firstSite]: an edge of the region of point s, -1 if it has none
    QVector<int> convexHull; // Sort in counterclockwise order
};

struct VoronoiStep {
    QVector<QLineF> currentEdges; // All edges at the current step (including left, right, and hyperplanes)
    QVector<QPointF> currentHull; // The merged convext hull, the left one, the right one
    QVector<int> edgeTypes; // Indicates the edge type (0: Normal, 1: Left voronoi, 2: Right voronoi, 3: Hyperplane)
    QString description; // For debugging
    int split; // 0: Merged convex hull, not 0: lefthull = currentHull.mid(0, split), righthull = currentHull.mid(split)
};

class VoronoiDiagram {
public:
    // Compare lexicographically
    static bool comparePoints(const QPointF& a, const QPointF& b);
    static bool compareEdges(const QLineF& e1, const QLineF& e2);

    // Clip a line segment to a rectangle, return false if it lies outside (Liang-Barsky)
    static bool clipLine(const QLineF& line, const QRectF& rect, QLineF& clipped);

    static QVector<QPointF> sortAndUnique(const QVector<QPointF>& points);

    static QVector<QLineF> getVoronoiEdges(QVector<QPointF>& points);

    // For step by step
    static QVector<VoronoiStep> steps;
    static void clearSteps() { steps.clear(); }
    static void addStep(const QVector<QLineF>& edges, const QVector<int>& types, const QVector<QPointF>& hull, QString desc = "", int split = 0);

private:
    static QVector<QPointF> P; // The sorted points of the current diagram

    static VoronoiResult voronoi3(int first);

    // End point of a ray from origin in direction dir, far enough away to stand for infinity
    static QPointF rayEnd(const QPointF& origin, const QPointF& dir);

    static QLineF getPerpBisector(const QPointF& p1, const QPointF& p2);
    static QPointF calculateCircumcenter(const QPointF& p1, const QPointF& p2, const QPointF& p3);
    static qreal crossProduct(const QPointF& o, const QPointF& a, const QPointF& b);

    static VoronoiResult divideAndConquer(int first, int last); // Points first, ..., last - 1
    static VoronoiResult merge(const VoronoiResult& left, const VoronoiResult& right);

    static QVector<int> mergeConvexHull(const QVector<int>& left, const QVector<int>& right, const QPair<int, int>& upperTan, const QPair<int, int>& lowerTan);
    static QVector<QPointF> hullPoints(const QVector<int>& hull);

    static QPair<int, int> getUpperTangent(const QVector<QPointF>& leftHull, const QVector<QPointF>& rightHull);
    static QPair<int, int> getLowerTangent(const QVector<QPointF>& leftHull, const QVector<QPointF>& rightHull);

    static bool areAllCollinear(int first, int last);
    static VoronoiResult solveCollinear(int first, int last);

    // Base cases: the edges of each region, in counterclockwise order
    static void linkRegions(VoronoiResult& res, int first, int last);
};

#endif // VORONOIDIAGRAM_H
