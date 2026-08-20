// $LAN=C++$
// File: VoronoiDiagram.cpp
// Author: 李俊諺 Jun-Yan Li
// Student ID: M143040074
// Date: 2025/12/5

#include "../include/VoronoiDiagram.h"
#include <cmath>
#include <QMultiMap>
#include <QDebug>

const double EPSILON = 1e-6;
const int SCALE = 2000;

bool PointKey::operator<(const PointKey& other) const {
    if (abs(p.x() - other.p.x()) > EPSILON) return p.x() < other.p.x();
    return p.y() < other.p.y();
}

bool VoronoiDiagram::comparePoints(const QPointF& a, const QPointF& b) {
    if (abs(a.x() - b.x()) > EPSILON) return a.x() < b.x();
    return a.y() < b.y();
}

bool VoronoiDiagram::compareEdges(const QLineF& e1, const QLineF& e2) {
    QPointF a1 = e1.p1(), b1 = e1.p2();
    QPointF a2 = e2.p1(), b2 = e2.p2();

    if (abs(a1.x() - a2.x()) > EPSILON) return a1.x() < a2.x();
    if (abs(a1.y() - a2.y()) > EPSILON) return a1.y() < a2.y();
    if (abs(b1.x() - b2.x()) > EPSILON) return b1.x() < b2.x();
    return b1.y() < b2.y();
}

bool VoronoiDiagram::isBoundaryPoint(const QPointF& p) {
    const double LIMIT = SCALE * 0.8;
    return (abs(p.x()) > LIMIT || abs(p.y()) > LIMIT);
}

QVector<QPointF> VoronoiDiagram::sortAndUnique(const QVector<QPointF>& points) {
    QVector<QPointF> newPoints = points;
    std::sort(newPoints.begin(), newPoints.end(), comparePoints);
    auto last = std::unique(newPoints.begin(), newPoints.end());
    newPoints.erase(last, newPoints.end());
    return newPoints;
}

// --- Initialize static variable ---
QVector<VoronoiStep> VoronoiDiagram::steps;

void VoronoiDiagram::addStep(const QVector<QLineF>& edges, const QVector<int>& types, const QVector<QPointF>& hull, QString desc, int split) {
    VoronoiStep step;
    step.currentEdges = edges;
    step.edgeTypes = types;
    step.currentHull = hull;
    step.description = desc;
    step.split = split;
    steps.append(step);
}

VoronoiResult VoronoiDiagram::voronoi3(const QVector<QPointF>& points) {
    VoronoiResult res;
    QPointF ccenter = calculateCircumcenter(points[0], points[1], points[2]);

    // Collinear
    if (std::abs(crossProduct(points[0], points[1], points[2])) <= EPSILON) {
        res.edges.append(getPerpBisector(points[0], points[1]));
        res.endPoints.append(qMakePair(points[0], points[1]));
        res.edges.append(getPerpBisector(points[1], points[2]));
        res.endPoints.append(qMakePair(points[1], points[2]));
    }
    else {
        // Draw three rays starting from the circumcenter
        // The direction of vec is right, the direction of normal is up
        // 如果法向量方向與中點往第三點方向相同，射線方向要改為反方向
        QPointF mid01 = (points[0] + points[1]) / 2;
        QPointF vec01 = points[1] - points[0];
        QPointF normal01(vec01.y(), -vec01.x());
        if (QPointF::dotProduct(normal01, points[2] - mid01) > 0) normal01 = -normal01;
        res.edges.append(QLineF(ccenter, ccenter + normal01 * SCALE));
        res.endPoints.append(qMakePair(points[0], points[1]));

        QPointF mid12 = (points[1] + points[2]) / 2;
        QPointF vec12 = points[2] - points[1];
        QPointF normal12(vec12.y(), -vec12.x());
        if (QPointF::dotProduct(normal12, points[0] - mid12) > 0) normal12 = -normal12;
        res.edges.append(QLineF(ccenter, ccenter + normal12 * SCALE));
        res.endPoints.append(qMakePair(points[1], points[2]));

        QPointF mid02 = (points[0] + points[2]) / 2;
        QPointF vec02 = points[2] - points[0];
        QPointF normal02(vec02.y(), -vec02.x());
        if (QPointF::dotProduct(normal02, points[1] - mid02) > 0) normal02 = -normal02;
        res.edges.append(QLineF(ccenter, ccenter + normal02 * SCALE));
        res.endPoints.append(qMakePair(points[0], points[2]));
    }

    // Determine whether the order is clockwise or counter-clockwise, or which point is in the middle
    qreal cp = crossProduct(points[0], points[1], points[2]);
    res.convexHull.append(points[0]);
    if (cp < 0) {
        // p2 is to the left of p0->p1, p0 -> p1 -> p2
        res.convexHull.append(points[1]);
        res.convexHull.append(points[2]);
    }
    else if (cp > 0) {
        // p2 is to the right of p0->p1, p0 -> p2 -> p1
        res.convexHull.append(points[2]);
        res.convexHull.append(points[1]);
    }
    else
        res.convexHull.append(points[2]); // Collinear
    return res;
}

QLineF VoronoiDiagram::getPerpBisector(const QPointF& p1, const QPointF& p2) {
    QPointF mid = (p1 + p2) / 2;
    QPointF vec = p2 - p1;
    QPointF normal(vec.y(), -vec.x());
    // Allow extending beyond the window
    return QLineF(mid + normal * SCALE, mid - normal * SCALE);
}

QPointF VoronoiDiagram::calculateCircumcenter(const QPointF& p1, const QPointF& p2, const QPointF& p3) {
    /*
    (x, y) is a point on the perpendicular bisector.
        (x - p1.x)^2 + (y - p1.y)^2 = (x - p2.x)^2 + (y - p2.y)^2
    => 2(p2.x - p1.x)x + 2(p2.y - p1.y)y = p2.x^2 + p2.y^2 - p1.x^2 - p1.y^2
    => Ax + By = C
        (x - p2.x)^2 + (y - p2.y)^2 = (x - p3.x)^2 + (y - p3.y)^2
    => 2(p3.x - p2.x)x + 2(p3.y - p2.y)y = p3.x^2 + p3.y^2 - p2.x^2 - p2.y^2
    => Dx + Ey = F
    */
    // Cramer's rule
    double A = 2 * (p2.x() - p1.x());
    double B = 2 * (p2.y() - p1.y());
    double C = p2.x() * p2.x() + p2.y() * p2.y() - p1.x() * p1.x() - p1.y() * p1.y();
    double D = 2 * (p3.x() - p2.x());
    double E = 2 * (p3.y() - p2.y());
    double F = p3.x() * p3.x() + p3.y() * p3.y() - p2.x() * p2.x() - p2.y() * p2.y();
    double delta = A*E - B*D;
    double x = (C*E - B*F) / delta;
    double y = (A*F - C*D) / delta;
    return QPointF(x, y);
}

qreal VoronoiDiagram::crossProduct(const QPointF& o, const QPointF& a, const QPointF& b) {
    return (a.x() - o.x()) * (b.y() - o.y()) - (a.y() - o.y()) * (b.x() - o.x());
}

QVector<QLineF> VoronoiDiagram::getVoronoiEdges(QVector<QPointF>& points) {
    QVector<QPointF> newPoints = sortAndUnique(points);
    int n = newPoints.size();
    VoronoiResult result;
    clearSteps();

    if (areAllCollinear(newPoints)) {
        result = solveCollinear(newPoints);
        addStep(result.edges, QVector<int>(), QVector<QPointF>(), "Collinear Case");
        addStep(result.edges, QVector<int>(), result.convexHull, "Collinear Case");
    }
    else if (n == 3) {
        result = voronoi3(newPoints);
        addStep(result.edges, QVector<int>(), QVector<QPointF>());
        addStep(result.edges, QVector<int>(), result.convexHull);
    }
    else
        result = divideAndConquer(newPoints);
    return result.edges;
}

VoronoiResult VoronoiDiagram::divideAndConquer(const QVector<QPointF>& points) {
    VoronoiResult res;
    int n = points.size();

    if (n == 1)
        res.convexHull.append(points[0]);
    else if (n == 2)
        res = solveCollinear(points);
    else if (n == 3)
        res = voronoi3(points);
    else {
        // Divide
        int m = n / 2;
        QVector<QPointF> leftPoints = points.mid(0, m), rightPoints = points.mid(m);
        VoronoiResult leftRes = divideAndConquer(leftPoints), rightRes = divideAndConquer(rightPoints);

        // Merge
        res = merge(leftRes, rightRes);
    }
    return res;
}

VoronoiResult VoronoiDiagram::merge(const VoronoiResult& left, const VoronoiResult& right) {
    VoronoiResult res;
    QVector<QLineF> stepEdges;
    QVector<QPointF> stepHull;
    QVector<int> stepTypes;

    // Step 1. Left voronoi, type 1 (blue)
    stepEdges.append(left.edges);
    stepTypes.append(QVector<int>(left.edges.size(), 1));
    addStep(stepEdges, stepTypes, QVector<QPointF>(), "Step 1");

    // Step 2. Right voronoi, type 2 (green)
    stepEdges.append(right.edges);
    stepTypes.append(QVector<int>(right.edges.size(), 2));
    addStep(stepEdges, stepTypes, QVector<QPointF>(), "Step 2");

    // Step 3. Left convex hull, cyan
    stepHull = left.convexHull;
    addStep(stepEdges, stepTypes, stepHull, "Step 3");
    // Step 4. Right convex hull, cyan
    stepHull = left.convexHull + right.convexHull;
    addStep(stepEdges, stepTypes, stepHull, "Step 4", left.convexHull.size());

    // Find the upper tangent and the lower tangent
    QPair<int, int> upperTan = getUpperTangent(left.convexHull, right.convexHull);
    QPair<int, int> lowerTan = getLowerTangent(left.convexHull, right.convexHull);

    // Step 5. Merged convex hull, cyan
    stepHull = mergeConvexHull(left.convexHull, right.convexHull, upperTan, lowerTan);
    addStep(stepEdges, stepTypes, stepHull, "Step 5");

    // Step 6. Scan for Hyperplane (scan line), type 3 (magenta)
    QVector<QLineF> hyperPlane;
    QVector<QPair<QPointF, QPointF>> hyperPlanePoints;

    QPointF L = left.convexHull[upperTan.first], R = right.convexHull[upperTan.second];
    QPointF endL = left.convexHull[lowerTan.first], endR = right.convexHull[lowerTan.second];

    // Calculate start point (Extend upwards from the midpoint of the upper tangent)
    QPointF startMid = (L + R) / 2;
    QPointF tanVec = R - L;
    QPointF upNormal(tanVec.y(), -tanVec.x());
    QPointF currentPos = startMid + upNormal * SCALE; // Hyperplane start point

    // Copy edges for clipping
    QVector<QLineF> newLeftEdges = left.edges;
    QVector<QLineF> newRightEdges = right.edges;

    // PointKey maps a point to indices of edges incident to it
    // QMultiMap 一對多，QMap 一對一
    QMultiMap<PointKey, int> leftEdgeMap, rightEdgeMap;
    for (int i = 0; i < left.endPoints.size(); ++i) {
        leftEdgeMap.insert(left.endPoints[i].first, i);
        leftEdgeMap.insert(left.endPoints[i].second, i);
    }
    for (int i = 0; i < right.endPoints.size(); ++i) {
        rightEdgeMap.insert(right.endPoints[i].first, i);
        rightEdgeMap.insert(right.endPoints[i].second, i);
    }

    // --- Hyperplane Scan Loop ---
    int loopSafety = 0;
    const int MAX_LOOP = 500;
    while (true) {
        if (loopSafety++ > MAX_LOOP) {
            qWarning() << "Hyperplane scan loop limit reached!";
            break;
        }

        // 1. Calculate the perpendicular bisector of current L and R
        QLineF bisector = getPerpBisector(L, R);

        // 2. Find the nearest intersection with Voronoi edges
        double bestDistL = 1e18, bestDistR = 1e18;
        int hitIdxL = -1, hitIdxR = -1;
        QPointF hitPointL, hitPointR;

        // Scan Left Edges
        auto lList = leftEdgeMap.values(PointKey(L));
        for (int edgeIdx : std::as_const(lList)) {
            QPointF intersection;
            QLineF::IntersectionType type = bisector.intersects(newLeftEdges[edgeIdx], &intersection);

            // Check if intersection is below relative to scan direction
            double distSq = QPointF::dotProduct(intersection - currentPos, intersection - currentPos);
            QPointF scanDir = intersection - currentPos;
            QPointF biDir = bisector.p2() - bisector.p1(); // Direction of bisector

            // Valid intersection: Intersects segment AND dist > epsilon AND direction matches
            if (type == QLineF::BoundedIntersection && distSq > EPSILON && QPointF::dotProduct(scanDir, biDir) > 0) {
                if (distSq < bestDistL) {
                    bestDistL = distSq;
                    hitIdxL = edgeIdx;
                    hitPointL = intersection;
                }
            }
        }

        // Scan Right Edges
        auto rList = rightEdgeMap.values(PointKey(R));
        for (int edgeIdx : std::as_const(rList)) {
            QPointF intersection;
            QLineF::IntersectionType type = bisector.intersects(newRightEdges[edgeIdx], &intersection);

            double distSq = QPointF::dotProduct(intersection - currentPos, intersection - currentPos);
            QPointF scanDir = intersection - currentPos;
            QPointF biDir = bisector.p2() - bisector.p1();

            if (type == QLineF::BoundedIntersection && distSq > EPSILON && QPointF::dotProduct(scanDir, biDir) > 0) {
                if (distSq < bestDistR) {
                    bestDistR = distSq;
                    hitIdxR = edgeIdx;
                    hitPointR = intersection;
                }
            }
        }

        // 3. Determine Hit Event
        bool hitLeft = false, hitRight = false;
        QLineF edge;

        // Simultaneous Hit
        if (abs(bestDistL - bestDistR) < EPSILON) {
            hitLeft = true;
            hitRight = true;
            edge = QLineF(currentPos, hitPointL);
            currentPos = hitPointL;
        }
        // Left hit
        else if (bestDistL < bestDistR) {
            hitLeft = true;
            edge = QLineF(currentPos, hitPointL);
            currentPos = hitPointL;
        }
        // Right hit
        else {
            hitRight = true;
            edge = QLineF(currentPos, hitPointR);
            currentPos = hitPointR;
        }

        hyperPlane.append(edge);
        hyperPlanePoints.append(qMakePair(L, R));

        // 4. Clip Edges and Update Endpoints
        if (hitIdxL == -1 && hitIdxR == -1) {
            // Set of vertically collinear points (Horizontal perpendicular bisector)
            QPointF biDir = bisector.p2() - bisector.p1();
            double len = QPointF::dotProduct(biDir, biDir);
            QPointF unitDir = biDir / len;

            hyperPlane.append(QLineF(currentPos, currentPos + unitDir * SCALE));
            hyperPlanePoints.append(qMakePair(L, R));
            break;
        }

        if (hitLeft) {
            QLineF &e = newLeftEdges[hitIdxL];
            // Keep the segment closer to L, clip the rest
            if (QPointF::dotProduct(e.p1()-L, e.p1()-L) < QPointF::dotProduct(e.p1()-R, e.p1()-R))
                e.setP2(currentPos);
            else
                e.setP1(currentPos);

            // Traverse to the next point on the left
            QPointF p1 = left.endPoints[hitIdxL].first;
            QPointF p2 = left.endPoints[hitIdxL].second;
            L = (PointKey(p1).p != L) ? p1 : p2;
        }

        if (hitRight) {
            QLineF &e = newRightEdges[hitIdxR];
            // Keep the segment closer to R
            if (QPointF::dotProduct(e.p1()-R, e.p1()-R) < QPointF::dotProduct(e.p1()-L, e.p1()-L))
                e.setP2(currentPos);
            else
                e.setP1(currentPos);

            // Traverse to the next point on the right
            QPointF p1 = right.endPoints[hitIdxR].first;
            QPointF p2 = right.endPoints[hitIdxR].second;
            R = (PointKey(p1).p != R) ? p1 : p2;
        }

        // 5. Check Termination Condition (Reached Lower Tangent)
        bool reachL = (abs(L.x() - endL.x()) < EPSILON && abs(L.y() - endL.y()) < EPSILON);
        bool reachR = (abs(R.x() - endR.x()) < EPSILON && abs(R.y() - endR.y()) < EPSILON);

        if (reachL && reachR) {
            // Add the final infinite segment downwards
            QLineF finalBisector = getPerpBisector(L, R);
            QPointF dir = finalBisector.p2() - finalBisector.p1();
            if (dir.y() < 0) dir = -dir; // Ensure downwards
            hyperPlane.append(QLineF(currentPos, currentPos + dir * SCALE));
            hyperPlanePoints.append(qMakePair(L, R));
            break;
        }
    }

    stepEdges.append(hyperPlane);
    stepTypes.append(QVector<int>(hyperPlane.size(), 3));
    addStep(stepEdges, stepTypes, stepHull, "Step 6");

    // --- Combine Results ---
    // Append clipped edges and the new hyperplane
    res.edges.append(newLeftEdges);
    res.endPoints.append(left.endPoints);

    res.edges.append(newRightEdges);
    res.endPoints.append(right.endPoints);

    res.edges.append(hyperPlane);
    res.endPoints.append(hyperPlanePoints);

    res.convexHull = stepHull;

    // Remove dangling edges
    eliminateEdges(res);

    // Step 7. Final result
    stepEdges = res.edges;
    stepTypes = QVector<int>(res.edges.size(), 0); // 0: Normal/Result, red
    addStep(stepEdges, stepTypes, stepHull, "Step 7");

    return res;
}

QVector<QPointF> VoronoiDiagram::mergeConvexHull(const QVector<QPointF>& left, const QVector<QPointF>& right, const QPair<int, int>& upperTan, const QPair<int, int>& lowerTan) {
    QVector<QPointF> mergedHull;
    int nL = left.size(), nR = right.size();

    // Left convex hull: Collect points counterclockwise from upperTan.first to lowerTan.first
    int idx = upperTan.first;
    mergedHull.append(left[idx]);
    while (idx != lowerTan.first) {
        idx = (idx + 1) % nL; // Move counterclockwise
        mergedHull.append(left[idx]);
    }

    // Right convex hull: Collect points counterclockwise from lowerTan.second to upperTan.second
    idx = lowerTan.second;
    mergedHull.append(right[idx]);
    while (idx != upperTan.second) {
        idx = (idx + 1) % nR;
        mergedHull.append(right[idx]);
    }
    return mergedHull;
}

void VoronoiDiagram::eliminateEdges(VoronoiResult& res) {
    bool changed = true;

    // Loop until no more edges are removed (deleting one edge might make another dangling)
    while (changed) {
        changed = false;

        // 1. Build Degree Map (Count connections for each point)
        QMap<PointKey, int> degree;
        for (const auto& e : std::as_const(res.edges)) {
            degree[PointKey(e.p1())]++;
            degree[PointKey(e.p2())]++;
        }

        QVector<QLineF> validEdges;
        QVector<QPair<QPointF, QPointF>> validEndPoints;

        // 2. Check and Filter
        for (int i = 0; i < res.edges.size(); ++i) {
            const QLineF& e = res.edges[i];
            bool remove = false;

            // Check P1: If internal (not boundary) and has degree <= 1 -> Dangling
            if (!isBoundaryPoint(e.p1()) && degree[PointKey(e.p1())] <= 1)
                remove = true;
            // Check P2
            else if (!isBoundaryPoint(e.p2()) && degree[PointKey(e.p2())] <= 1)
                remove = true;

            if (remove)
                changed = true; // Mark as changed to trigger another pass
            else {
                validEdges.append(e);
                validEndPoints.append(res.endPoints[i]);
            }
        }

        // Update result
        res.edges = validEdges;
        res.endPoints = validEndPoints;
    }
}

QPair<int, int> VoronoiDiagram::getUpperTangent(const QVector<QPointF>& leftHull, const QVector<QPointF>& rightHull) {
    int nL = leftHull.size(), nR = rightHull.size();

    // Find rightmost of left hull
    int l = 0;
    for (int i = 1; i < nL; ++i)
        if (leftHull[i].x() > leftHull[l].x()) l = i;

    // Find leftmost of right hull
    int r = 0;
    for (int i = 1; i < nR; ++i)
        if (rightHull[i].x() < rightHull[r].x()) r = i;

    // Adjust points using the Walking Algorithm
    bool done = false;
    while (!done) {
        done = true;

        // Adjust left point: If cross product > EPSILON (positive), it means the line cuts into the hull
        // We need to move counterclockwise (l + 1) to find the tangent
        while (crossProduct(rightHull[r], leftHull[l], leftHull[(l + 1) % nL]) > EPSILON) {
            l = (l + 1) % nL;
            done = false;
        }

        // Adjust right point: If cross product < -EPSILON (negative), it means the line cuts into the hull
        // We need to move clockwise (r - 1) to find the tangent
        while (crossProduct(leftHull[l], rightHull[r], rightHull[(r - 1 + nR) % nR]) < -EPSILON) {
            r = (r - 1 + nR) % nR;
            done = false;
        }
    }

    return qMakePair(l, r);
}

QPair<int, int> VoronoiDiagram::getLowerTangent(const QVector<QPointF>& leftHull, const QVector<QPointF>& rightHull) {
    int nL = leftHull.size(), nR = rightHull.size();

    int l = 0;
    for (int i = 1; i < nL; ++i)
        if (leftHull[i].x() > leftHull[l].x()) l = i;

    int r = 0;
    for (int i = 1; i < nR; ++i)
        if (rightHull[i].x() < rightHull[r].x()) r = i;

    bool done = false;
    while (!done) {
        done = true;

        while (crossProduct(rightHull[r], leftHull[l], leftHull[(l - 1 + nL) % nL]) < -EPSILON) {
            l = (l - 1 + nL) % nL;
            done = false;
        }

        while (crossProduct(leftHull[l], rightHull[r], rightHull[(r + 1) % nR]) > EPSILON) {
            r = (r + 1) % nR;
            done = false;
        }
    }

    return qMakePair(l, r);
}

bool VoronoiDiagram::areAllCollinear(const QVector<QPointF>& points) {
    if (points.size() < 3) return true;

    const QPointF& p1 = points[0];
    const QPointF& p2 = points[1];

    // Check if every subsequent point lies on the line defined by p1 and p2 (cross product is 0)
    for (int i = 2; i < points.size(); ++i)
        if (std::abs(crossProduct(p1, p2, points[i])) > EPSILON)
            return false;
    return true;
}

VoronoiResult VoronoiDiagram::solveCollinear(const QVector<QPointF>& points) {
    VoronoiResult res;

    for (int i = 0; i < points.size() - 1; ++i) {
        // Directly calculate the perpendicular bisector of adjacent points
        res.edges.append(getPerpBisector(points[i], points[i+1]));
        res.endPoints.append(qMakePair(points[i], points[i+1]));
    }

    // Adding just the first and last points
    if (!points.isEmpty()) {
        res.convexHull.append(points.first());
        res.convexHull.append(points.last());
    }
    return res;
}
