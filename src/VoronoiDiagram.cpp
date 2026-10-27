// $LAN=C++$
// File: VoronoiDiagram.cpp
// Author: 李俊諺 Jun-Yan Li
// Student ID: M143040074
// Date: 2025/12/5

#include "../include/VoronoiDiagram.h"
#include <cmath>
#include <QMultiMap>
#include <QDebug>
#include <QtMath>

const double EPSILON = 1e-6;
const double FAR = 1e10; // Length of rays to infinity, much larger than any Voronoi vertex

bool PointKey::operator<(const PointKey& other) const {
    if (abs(p.x() - other.p.x()) > EPSILON) return p.x() < other.p.x();
    if (abs(p.y() - other.p.y()) > EPSILON) return p.y() < other.p.y();
    if (abs(r - other.r) > EPSILON * (1 + std::max(r, other.r))) return r < other.r;
    return false;
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
    // Ray end points are at least FAR away from the origin, Voronoi vertices are much closer
    const double LIMIT = FAR * 0.5;
    return (abs(p.x()) > LIMIT || abs(p.y()) > LIMIT);
}

QPointF VoronoiDiagram::rayEnd(const QPointF& origin, const QPointF& dir) {
    double len = std::sqrt(QPointF::dotProduct(dir, dir));
    double reach = FAR + std::abs(origin.x()) + std::abs(origin.y());
    return origin + dir / len * reach;
}

bool VoronoiDiagram::clipLine(const QLineF& line, const QRectF& rect, QLineF& clipped) {
    double x0 = line.x1(), y0 = line.y1(), dx = line.dx(), dy = line.dy();
    double t0 = 0, t1 = 1;
    // Each boundary: p * t <= q
    const double p[4] = {-dx, dx, -dy, dy};
    const double q[4] = {x0 - rect.left(), rect.right() - x0, y0 - rect.top(), rect.bottom() - y0};
    int side0 = -1, side1 = -1; // Which boundary the clipped end points lie on
    for (int i = 0; i < 4; ++i) {
        if (p[i] == 0) {
            if (q[i] < 0) return false; // Parallel and outside
            continue;
        }
        double t = q[i] / p[i];
        if (p[i] < 0 && t > t0) { t0 = t; side0 = i; }
        else if (p[i] > 0 && t < t1) { t1 = t; side1 = i; }
    }
    if (t0 > t1) return false;

    // Put clipped end points exactly on the boundary
    auto pointAt = [&](double t, int side) {
        QPointF pt(x0 + t * dx, y0 + t * dy);
        if (side == 0) pt.setX(rect.left());
        else if (side == 1) pt.setX(rect.right());
        else if (side == 2) pt.setY(rect.top());
        else if (side == 3) pt.setY(rect.bottom());
        return pt;
    };
    clipped = QLineF(pointAt(t0, side0), pointAt(t1, side1));
    return true;
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
        res.edges.append(QLineF(ccenter, rayEnd(ccenter, normal01)));
        res.endPoints.append(qMakePair(points[0], points[1]));

        QPointF mid12 = (points[1] + points[2]) / 2;
        QPointF vec12 = points[2] - points[1];
        QPointF normal12(vec12.y(), -vec12.x());
        if (QPointF::dotProduct(normal12, points[0] - mid12) > 0) normal12 = -normal12;
        res.edges.append(QLineF(ccenter, rayEnd(ccenter, normal12)));
        res.endPoints.append(qMakePair(points[1], points[2]));

        QPointF mid02 = (points[0] + points[2]) / 2;
        QPointF vec02 = points[2] - points[0];
        QPointF normal02(vec02.y(), -vec02.x());
        if (QPointF::dotProduct(normal02, points[1] - mid02) > 0) normal02 = -normal02;
        res.edges.append(QLineF(ccenter, rayEnd(ccenter, normal02)));
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
    return QLineF(rayEnd(mid, normal), rayEnd(mid, -normal));
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
    else if (n == 2 || areAllCollinear(points)) // Collinear sub-problems are solved directly
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

    // The hyperplane starts from infinity above the upper tangent
    bool atStart = true;
    QPointF currentPos;

    // Copy edges for clipping
    QVector<QLineF> newLeftEdges = left.edges;
    QVector<QLineF> newRightEdges = right.edges;
    QVector<bool> leftRemoved(left.edges.size(), false), rightRemoved(right.edges.size(), false);

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

    // An intersection of the hyperplane with an edge of the region of L (or R)
    struct Hit {
        bool isLeft;
        int edgeIdx;
        QPointF other;   // The other point that generated the edge
        QPointF point;   // Intersection
        double t;        // Position along the hyperplane direction
        bool atEndPoint; // Intersection is an existing Voronoi vertex
    };

    // --- Hyperplane Scan Loop ---
    int loopSafety = 0;
    const int MAX_LOOP = left.edges.size() + right.edges.size() + 10;
    while (true) {
        if (loopSafety++ > MAX_LOOP) {
            qWarning() << "Hyperplane scan loop limit reached!";
            break;
        }

        // 1. The perpendicular bisector of current L and R, dir points downwards along the hyperplane
        QPointF mid = (L + R) / 2;
        QPointF vec = R - L;
        QPointF dir(-vec.y(), vec.x());
        dir /= std::sqrt(QPointF::dotProduct(dir, dir));
        double currentT = atStart ? -qInf() : QPointF::dotProduct(currentPos - mid, dir);

        // 2. Find intersections below the current position with edges of the regions of L and R
        QVector<Hit> hits;
        auto scanEdges = [&](bool isLeft, const QPointF& site) {
            const QMultiMap<PointKey, int>& edgeMap = isLeft ? leftEdgeMap : rightEdgeMap;
            const QVector<QPair<QPointF, QPointF>>& endPoints = isLeft ? left.endPoints : right.endPoints;
            const QVector<QLineF>& edges = isLeft ? newLeftEdges : newRightEdges;
            const QVector<bool>& removed = isLeft ? leftRemoved : rightRemoved;

            const QList<int> edgeList = edgeMap.values(PointKey(site));
            for (int edgeIdx : edgeList) {
                if (removed[edgeIdx]) continue;
                QPointF other = (endPoints[edgeIdx].first == site) ? endPoints[edgeIdx].second : endPoints[edgeIdx].first;

                // The edge lies on the bisector of site and other, so it meets the bisector of L and R
                // at the circumcenter of L, R and other (parallel if they are collinear)
                if (std::abs(crossProduct(L, R, other)) <= EPSILON) continue;
                QPointF p = calculateCircumcenter(L, R, other);
                double t = QPointF::dotProduct(p - mid, dir);
                if (t <= currentT + EPSILON) continue;

                // The intersection must lie on the (clipped) edge
                const QLineF& e = edges[edgeIdx];
                QPointF eDir = e.p2() - e.p1();
                eDir /= std::sqrt(QPointF::dotProduct(eDir, eDir));
                if (QPointF::dotProduct(p - e.p1(), eDir) < -EPSILON || QPointF::dotProduct(e.p2() - p, eDir) < -EPSILON) continue;

                // Snap to an existing vertex so that the same vertex keeps the same coordinates
                bool atEndPoint = true;
                if (QLineF(p, e.p1()).length() < EPSILON) p = e.p1();
                else if (QLineF(p, e.p2()).length() < EPSILON) p = e.p2();
                else atEndPoint = false;

                hits.append({isLeft, edgeIdx, other, p, t, atEndPoint});
            }
        };
        scanEdges(true, L);
        scanEdges(false, R);

        // No more intersections: the rest of the hyperplane goes to infinity
        if (hits.isEmpty()) {
            QPointF start = atStart ? rayEnd(mid, -dir) : currentPos;
            hyperPlane.append(QLineF(start, rayEnd(atStart ? mid : currentPos, dir)));
            hyperPlanePoints.append(qMakePair(L, R));
            break;
        }

        // 3. Determine Hit Event: all intersections at the nearest point are handled together
        //    (more than one when 4 or more points lie on the same circle)
        double bestT = hits[0].t;
        for (const Hit& h : std::as_const(hits))
            bestT = std::min(bestT, h.t);

        QVector<Hit> nearHits;
        for (const Hit& h : std::as_const(hits))
            if (h.t <= bestT + EPSILON) nearHits.append(h);

        QPointF v = nearHits[0].point;
        for (const Hit& h : std::as_const(nearHits))
            if (h.atEndPoint) { v = h.point; break; }

        hyperPlane.append(QLineF(atStart ? rayEnd(v, -dir) : currentPos, v));
        hyperPlanePoints.append(qMakePair(L, R));

        // 4. Find the next L and R
        // All generating points lie on a circle centered at v. Measured from the incoming hyperplane
        // and turning towards L, the angles go: L (smallest) ... next L, next R ... R (largest)
        QPointF back = -dir;
        double side = (crossProduct(QPointF(), back, L - v) > 0) ? 1 : -1;
        auto angleOf = [&](const QPointF& w) {
            double a = std::atan2(side * crossProduct(QPointF(), back, w), QPointF::dotProduct(back, w));
            return (a < 0) ? a + 2 * M_PI : a;
        };

        QPointF nextL = L, nextR = R;
        double maxAngleL = angleOf(L - v), minAngleR = angleOf(R - v);
        for (const Hit& h : std::as_const(nearHits)) {
            double a = angleOf(h.other - v);
            if (h.isLeft && a > maxAngleL) { maxAngleL = a; nextL = h.other; }
            if (!h.isLeft && a < minAngleR) { minAngleR = a; nextR = h.other; }
        }
        QPointF nextVec = nextR - nextL;
        double outAngle = angleOf(QPointF(-nextVec.y(), nextVec.x())); // Outgoing hyperplane

        // 5. Clip Edges: keep the parts on their own side of the hyperplane
        //    (left side: between the incoming and the outgoing hyperplane, right side: the rest)
        for (const Hit& h : std::as_const(nearHits)) {
            QLineF &e = h.isLeft ? newLeftEdges[h.edgeIdx] : newRightEdges[h.edgeIdx];
            auto keepPart = [&](const QPointF& endPoint) {
                if (QLineF(v, endPoint).length() < EPSILON) return false;
                return (angleOf(endPoint - v) < outAngle) == h.isLeft;
            };
            bool keep1 = keepPart(e.p1()), keep2 = keepPart(e.p2());

            if (keep1 && !keep2)
                e.setP2(v);
            else if (!keep1 && keep2)
                e.setP1(v);
            else if (!keep1 && !keep2)
                (h.isLeft ? leftRemoved : rightRemoved)[h.edgeIdx] = true; // Entirely on the wrong side
        }

        L = nextL;
        R = nextR;
        currentPos = v;
        atStart = false;
    }

    stepEdges.append(hyperPlane);
    stepTypes.append(QVector<int>(hyperPlane.size(), 3));
    addStep(stepEdges, stepTypes, stepHull, "Step 6");

    // --- Combine Results ---
    // Append clipped edges and the new hyperplane
    for (int i = 0; i < newLeftEdges.size(); ++i) {
        if (leftRemoved[i]) continue;
        res.edges.append(newLeftEdges[i]);
        res.endPoints.append(left.endPoints[i]);
    }

    for (int i = 0; i < newRightEdges.size(); ++i) {
        if (rightRemoved[i]) continue;
        res.edges.append(newRightEdges[i]);
        res.endPoints.append(right.endPoints[i]);
    }

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

        // 1. Build Degree Map (Count connections for each vertex)
        // A vertex is identified by its position and its distance to the generating points, so that
        // a vertex of the left diagram and one of the right diagram at the same position are not mixed up
        auto vertexKey = [&](const QPointF& p, int edgeIdx) {
            return PointKey(p, QLineF(p, res.endPoints[edgeIdx].first).length());
        };
        QMap<PointKey, int> degree;
        for (int i = 0; i < res.edges.size(); ++i) {
            degree[vertexKey(res.edges[i].p1(), i)]++;
            degree[vertexKey(res.edges[i].p2(), i)]++;
        }

        QVector<QLineF> validEdges;
        QVector<QPair<QPointF, QPointF>> validEndPoints;

        // 2. Check and Filter
        for (int i = 0; i < res.edges.size(); ++i) {
            const QLineF& e = res.edges[i];
            bool remove = false;

            // Check P1: If internal (not boundary) and has degree <= 1 -> Dangling
            if (!isBoundaryPoint(e.p1()) && degree[vertexKey(e.p1(), i)] <= 1)
                remove = true;
            // Check P2
            else if (!isBoundaryPoint(e.p2()) && degree[vertexKey(e.p2(), i)] <= 1)
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

    // Find rightmost of left hull (lexicographically, so ties in x pick the point next to the right hull)
    int l = 0;
    for (int i = 1; i < nL; ++i)
        if (comparePoints(leftHull[l], leftHull[i])) l = i;

    // Find leftmost of right hull
    int r = 0;
    for (int i = 1; i < nR; ++i)
        if (comparePoints(rightHull[i], rightHull[r])) r = i;

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
        if (comparePoints(leftHull[l], leftHull[i])) l = i;

    int r = 0;
    for (int i = 1; i < nR; ++i)
        if (comparePoints(rightHull[i], rightHull[r])) r = i;

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
