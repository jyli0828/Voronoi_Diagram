// $LAN=C++$
// File: VoronoiDiagram.cpp
// Author: 李俊諺 Jun-Yan Li
// Student ID: M143040074
// Date: 2025/12/5

#include "../include/VoronoiDiagram.h"
#include <cmath>
#include <algorithm>
#include <iterator>
#include <QDebug>
#include <QtMath>

const double EPSILON = 1e-6;
const double FAR = 1e10; // Length of rays to infinity, much larger than any Voronoi vertex
const double ANGLE_EPSILON = 1e-9; // For sines and cosines of angles between unit vectors

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

// --- Initialize static variables ---
QVector<VoronoiStep> VoronoiDiagram::steps;
QVector<QPointF> VoronoiDiagram::P;

void VoronoiDiagram::addStep(const QVector<QLineF>& edges, const QVector<int>& types, const QVector<QPointF>& hull, QString desc, int split) {
    VoronoiStep step;
    step.currentEdges = edges;
    step.edgeTypes = types;
    step.currentHull = hull;
    step.description = desc;
    step.split = split;
    steps.append(step);
}

// The point on the other side of edge e, seen from the region of point s
static int otherSite(const QVector<QPair<int, int>>& sites, int e, int s) {
    return sites[e].first == s ? sites[e].second : sites[e].first;
}

void VoronoiDiagram::linkRegions(VoronoiResult& res, int first, int last) {
    int n = last - first;
    res.firstSite = first;
    res.anyEdge = QVector<int>(n, -1);
    res.next = res.prev = QVector<std::array<int, 2>>(res.edges.size(), {-1, -1});

    QVector<QVector<int>> around(n);
    for (int i = 0; i < res.edges.size(); ++i) {
        around[res.sites[i].first - first].append(i);
        around[res.sites[i].second - first].append(i);
    }
    for (int s = 0; s < n; ++s) {
        QVector<int>& list = around[s];
        if (list.isEmpty()) continue;
        int site = first + s;
        // The edges of a region are in the order of the directions to the other points.
        // Counterclockwise on the screen (+y points down) is decreasing atan2
        auto angle = [&](int e) {
            QPointF d = P[otherSite(res.sites, e, site)] - P[site];
            return std::atan2(d.y(), d.x());
        };
        std::sort(list.begin(), list.end(), [&](int a, int b) { return angle(a) > angle(b); });
        for (int k = 0; k < list.size(); ++k) {
            int e = list[k], side = (res.sites[e].first == site) ? 0 : 1;
            res.next[e][side] = list[(k + 1) % list.size()];
            res.prev[e][side] = list[(k - 1 + list.size()) % list.size()];
        }
        res.anyEdge[s] = list[0];
    }
}

VoronoiResult VoronoiDiagram::voronoi3(int first) {
    VoronoiResult res;
    const QVector<QPointF> points = P.mid(first, 3);
    QPointF ccenter = calculateCircumcenter(points[0], points[1], points[2]);

    // Collinear
    if (std::abs(crossProduct(points[0], points[1], points[2])) <= EPSILON) {
        res.edges.append(getPerpBisector(points[0], points[1]));
        res.sites.append(qMakePair(first, first + 1));
        res.edges.append(getPerpBisector(points[1], points[2]));
        res.sites.append(qMakePair(first + 1, first + 2));
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
        res.sites.append(qMakePair(first, first + 1));

        QPointF mid12 = (points[1] + points[2]) / 2;
        QPointF vec12 = points[2] - points[1];
        QPointF normal12(vec12.y(), -vec12.x());
        if (QPointF::dotProduct(normal12, points[0] - mid12) > 0) normal12 = -normal12;
        res.edges.append(QLineF(ccenter, rayEnd(ccenter, normal12)));
        res.sites.append(qMakePair(first + 1, first + 2));

        QPointF mid02 = (points[0] + points[2]) / 2;
        QPointF vec02 = points[2] - points[0];
        QPointF normal02(vec02.y(), -vec02.x());
        if (QPointF::dotProduct(normal02, points[1] - mid02) > 0) normal02 = -normal02;
        res.edges.append(QLineF(ccenter, rayEnd(ccenter, normal02)));
        res.sites.append(qMakePair(first, first + 2));
    }

    // Determine whether the order is clockwise or counter-clockwise, or which point is in the middle
    qreal cp = crossProduct(points[0], points[1], points[2]);
    res.convexHull.append(first);
    if (cp < 0) {
        // p2 is to the left of p0->p1, p0 -> p1 -> p2
        res.convexHull.append(first + 1);
        res.convexHull.append(first + 2);
    }
    else if (cp > 0) {
        // p2 is to the right of p0->p1, p0 -> p2 -> p1
        res.convexHull.append(first + 2);
        res.convexHull.append(first + 1);
    }
    else
        res.convexHull.append(first + 2); // Collinear

    linkRegions(res, first, first + 3);
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
    P = sortAndUnique(points);
    int n = P.size();
    VoronoiResult result;
    clearSteps();

    if (areAllCollinear(0, n)) {
        result = solveCollinear(0, n);
        addStep(result.edges, QVector<int>(), QVector<QPointF>(), "Collinear Case");
        addStep(result.edges, QVector<int>(), hullPoints(result.convexHull), "Collinear Case");
    }
    else if (n == 3) {
        result = voronoi3(0);
        addStep(result.edges, QVector<int>(), QVector<QPointF>());
        addStep(result.edges, QVector<int>(), hullPoints(result.convexHull));
    }
    else
        result = divideAndConquer(0, n);
    return result.edges;
}

VoronoiResult VoronoiDiagram::divideAndConquer(int first, int last) {
    VoronoiResult res;
    int n = last - first;

    if (n == 1) {
        res.convexHull.append(first);
        linkRegions(res, first, last);
    }
    else if (n == 2 || areAllCollinear(first, last)) // Collinear sub-problems are solved directly
        res = solveCollinear(first, last);
    else if (n == 3)
        res = voronoi3(first);
    else {
        // Divide
        int m = first + n / 2;
        VoronoiResult leftRes = divideAndConquer(first, m), rightRes = divideAndConquer(m, last);

        // Merge
        res = merge(leftRes, rightRes);
    }
    return res;
}

VoronoiResult VoronoiDiagram::merge(const VoronoiResult& left, const VoronoiResult& right) {
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
    const QVector<QPointF> leftHull = hullPoints(left.convexHull), rightHull = hullPoints(right.convexHull);
    stepHull = leftHull;
    addStep(stepEdges, stepTypes, stepHull, "Step 3");
    // Step 4. Right convex hull, cyan
    stepHull = leftHull + rightHull;
    addStep(stepEdges, stepTypes, stepHull, "Step 4", leftHull.size());

    // Find the upper tangent and the lower tangent
    QPair<int, int> upperTan = getUpperTangent(leftHull, rightHull);
    QPair<int, int> lowerTan = getLowerTangent(leftHull, rightHull);

    // Step 5. Merged convex hull, cyan
    const QVector<int> mergedHull = mergeConvexHull(left.convexHull, right.convexHull, upperTan, lowerTan);
    stepHull = hullPoints(mergedHull);
    addStep(stepEdges, stepTypes, stepHull, "Step 5");

    // Step 6. Scan for Hyperplane (scan line), type 3 (magenta)

    // Edges of both diagrams, the right edges after the left ones. edges are clipped in place
    const int nLeft = left.edges.size(), nEdges = nLeft + right.edges.size();
    QVector<QLineF> edges = left.edges + right.edges;
    const QVector<QPair<int, int>> sites = left.sites + right.sites;
    QVector<std::array<int, 2>> next = left.next, prev = left.prev;
    for (int i = 0; i < right.edges.size(); ++i) {
        next.append({right.next[i][0] + nLeft, right.next[i][1] + nLeft});
        prev.append({right.prev[i][0] + nLeft, right.prev[i][1] + nLeft});
    }
    const int firstSite = left.firstSite, nSites = left.anyEdge.size() + right.anyEdge.size();
    QVector<int> anyEdge = left.anyEdge;
    for (int e : right.anyEdge) anyEdge.append(e < 0 ? -1 : e + nLeft);

    // removed: the edge lies on the wrong side, hit: the hyperplane met the edge,
    // passed: walking around a region, the edge came before the point where the hyperplane leaves it
    QVector<bool> removed(nEdges, false), hit(nEdges, false), passed(nEdges, false);

    // Walking around the region of point s: clockwise (turn = 1) or counterclockwise (turn = -1)
    auto step = [&](int e, int s, int turn) {
        int side = (sites[e].first == s) ? 0 : 1;
        return (turn > 0) ? prev[e][side] : next[e][side];
    };
    // Unit vector from point s to the point on the other side of edge e, perpendicular to the edge
    auto normal = [&](int e, int s) {
        QPointF d = P[otherSite(sites, e, s)] - P[s];
        return d / std::sqrt(QPointF::dotProduct(d, d));
    };
    // Between consecutive edges u and w (turning from u to w) the region goes to infinity,
    // when the angle from the normal of u to that of w is at least 180 degrees
    auto isGap = [&](int u, int w, int s, int turn) {
        if (u == w) return true;
        QPointF a = normal(u, s), b = normal(w, s);
        double c = turn * crossProduct(QPointF(), a, b); // Positive when turning the way of the walk
        return c < -ANGLE_EPSILON || (std::abs(c) <= ANGLE_EPSILON && QPointF::dotProduct(a, b) < 0);
    };
    // A ray in direction dir goes to infinity through the gap between u and w: it moves away from both
    // edges. When they are parallel (a strip), the gap on this side is the one turned to from u
    auto throughGap = [&](int u, int w, int s, int turn, const QPointF& dir) {
        QPointF a = normal(u, s);
        if (QPointF::dotProduct(a, dir) > ANGLE_EPSILON) return false;
        if (u == w) return true;
        QPointF b = normal(w, s);
        if (QPointF::dotProduct(b, dir) > ANGLE_EPSILON) return false;
        return std::abs(crossProduct(QPointF(), a, b)) > ANGLE_EPSILON || turn * crossProduct(QPointF(), a, dir) > 0;
    };

    // An intersection of the hyperplane with an edge of the region of L (or R)
    struct Hit {
        bool isLeft;
        int edgeIdx;
        int other;       // The other point that generated the edge
        QPointF point;   // Intersection
        double t;        // Position along the hyperplane direction
        bool atEndPoint; // Intersection is an existing Voronoi vertex
    };

    int L = left.convexHull[upperTan.first], R = right.convexHull[upperTan.second];
    QPointF mid, dir;
    double currentT = 0;

    // Whether the bisector of L and R, below the current position, meets edge e of the region of point s
    auto meets = [&](int e, int s, bool isLeft, Hit& out) {
        int other = otherSite(sites, e, s);
        // The edge lies on the bisector of s and other, so it meets the bisector of L and R
        // at the circumcenter of L, R and other (parallel if they are collinear)
        if (std::abs(crossProduct(P[L], P[R], P[other])) <= EPSILON) return false;
        QPointF p = calculateCircumcenter(P[L], P[R], P[other]);
        double t = QPointF::dotProduct(p - mid, dir);
        if (t <= currentT + EPSILON) return false;

        // The intersection must lie on the (clipped) edge
        const QLineF& line = edges[e];
        QPointF eDir = line.p2() - line.p1();
        eDir /= std::sqrt(QPointF::dotProduct(eDir, eDir));
        if (QPointF::dotProduct(p - line.p1(), eDir) < -EPSILON || QPointF::dotProduct(line.p2() - p, eDir) < -EPSILON) return false;

        // Snap to an existing vertex so that the same vertex keeps the same coordinates
        bool atEndPoint = true;
        if (QLineF(p, line.p1()).length() < EPSILON) p = line.p1();
        else if (QLineF(p, line.p2()).length() < EPSILON) p = line.p2();
        else atEndPoint = false;

        out = {isLeft, e, other, p, t, atEndPoint};
        return true;
    };

    // Find where the hyperplane leaves the region of point s, walking around it from edge ptr
    // (atGap: from the gap before ptr, where the hyperplane last went to infinity or came from).
    // Going down, the hyperplane leaves each region at points further along its boundary (clockwise for
    // a left region, counterclockwise for a right one), so ptr only moves forward. The edges walked
    // past lie on the side the hyperplane cuts off, and each one is walked past once.
    // Returns 1 when it meets edges, 0 when it goes to infinity, -1 if it is not found (unexpected)
    auto walk = [&](int s, bool isLeft, int& ptr, bool& atGap, QVector<Hit>& hits) {
        if (ptr < 0) return 0; // The region has no edges
        const int turn = isLeft ? 1 : -1;
        if (atGap && throughGap(step(ptr, s, -turn), ptr, s, turn, dir)) return 0; // Through the same gap
        QVector<int> walked;
        for (int e = ptr;;) {
            Hit h;
            if (!removed[e] && meets(e, s, isLeft, h)) {
                hits.append(h);
                ptr = e;
                atGap = false;
                // The next edges may meet it at the same vertex
                for (int f = e, g = step(f, s, turn); g != e && !isGap(f, g, s, turn) && !removed[g] && meets(g, s, isLeft, h); f = g, g = step(f, s, turn))
                    hits.append(h);
                for (int w : std::as_const(walked)) passed[w] = true;
                return 1;
            }
            int w = step(e, s, turn);
            walked.append(e);
            if (isGap(e, w, s, turn) && throughGap(e, w, s, turn, dir)) {
                for (int x : std::as_const(walked)) passed[x] = true;
                ptr = w;
                atGap = true;
                return 0;
            }
            e = w;
            if (e == ptr) return -1;
        }
    };

    // The hyperplane comes from infinity: the walk around the region of s starts after the gap it comes through
    auto entry = [&](int s, bool isLeft, const QPointF& up, bool& atGap) {
        atGap = false;
        const int turn = isLeft ? 1 : -1, start = anyEdge[s - firstSite];
        if (start < 0) return -1;
        int e = start;
        do {
            int w = step(e, s, turn);
            if (isGap(e, w, s, turn) && throughGap(e, w, s, turn, up)) {
                atGap = true;
                return w;
            }
            e = w;
        } while (e != start);
        return start; // Not found (unexpected): walk the whole region
    };

    int ptrL, ptrR;
    bool gapL, gapR;
    {
        QPointF vec = P[R] - P[L];
        QPointF up(vec.y(), -vec.x());
        ptrL = entry(L, true, up, gapL);
        ptrR = entry(R, false, up, gapR);
    }

    QVector<QLineF> hyperPlane;
    QVector<QPair<int, int>> hyperPlaneSites;
    bool atStart = true; // The hyperplane starts from infinity above the upper tangent
    QPointF currentPos;
    bool unexpected = false;

    // --- Hyperplane Scan Loop ---
    int loopSafety = 0;
    const int MAX_LOOP = nEdges + 10;
    while (true) {
        if (loopSafety++ > MAX_LOOP) {
            qWarning() << "Hyperplane scan loop limit reached!";
            break;
        }

        // 1. The perpendicular bisector of current L and R, dir points downwards along the hyperplane
        mid = (P[L] + P[R]) / 2;
        QPointF vec = P[R] - P[L];
        dir = QPointF(-vec.y(), vec.x());
        dir /= std::sqrt(QPointF::dotProduct(dir, dir));
        currentT = atStart ? -qInf() : QPointF::dotProduct(currentPos - mid, dir);

        // 2. Find where it leaves the regions of L and R
        QVector<Hit> hits;
        if (walk(L, true, ptrL, gapL, hits) < 0) unexpected = true;
        if (walk(R, false, ptrR, gapR, hits) < 0) unexpected = true;

        // No more intersections: the rest of the hyperplane goes to infinity
        if (hits.isEmpty()) {
            QPointF start = atStart ? rayEnd(mid, -dir) : currentPos;
            hyperPlane.append(QLineF(start, rayEnd(atStart ? mid : currentPos, dir)));
            hyperPlaneSites.append(qMakePair(L, R));
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
        hyperPlaneSites.append(qMakePair(L, R));

        // 4. Find the next L and R
        // All generating points lie on a circle centered at v. Measured from the incoming hyperplane
        // and turning towards L, the angles go: L (smallest) ... next L, next R ... R (largest)
        QPointF back = -dir;
        double side = (crossProduct(QPointF(), back, P[L] - v) > 0) ? 1 : -1;
        auto angleOf = [&](const QPointF& w) {
            double a = std::atan2(side * crossProduct(QPointF(), back, w), QPointF::dotProduct(back, w));
            return (a < 0) ? a + 2 * M_PI : a;
        };

        int nextL = L, nextR = R;
        double maxAngleL = angleOf(P[L] - v), minAngleR = angleOf(P[R] - v);
        for (const Hit& h : std::as_const(nearHits)) {
            double a = angleOf(P[h.other] - v);
            if (h.isLeft && a > maxAngleL) { maxAngleL = a; nextL = h.other; }
            if (!h.isLeft && a < minAngleR) { minAngleR = a; nextR = h.other; }
        }
        QPointF nextVec = P[nextR] - P[nextL];
        double outAngle = angleOf(QPointF(-nextVec.y(), nextVec.x())); // Outgoing hyperplane

        // 5. Clip Edges: keep the parts on their own side of the hyperplane
        //    (left side: between the incoming and the outgoing hyperplane, right side: the rest)
        for (const Hit& h : std::as_const(nearHits)) {
            QLineF &e = edges[h.edgeIdx];
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
                removed[h.edgeIdx] = true; // Entirely on the wrong side
            hit[h.edgeIdx] = true;
        }

        // 6. The walk around the region of a new L (or R) starts at the edge the hyperplane crossed into it
        for (const Hit& h : std::as_const(nearHits)) {
            if (h.isLeft && nextL != L && h.other == nextL) { ptrL = h.edgeIdx; gapL = false; }
            if (!h.isLeft && nextR != R && h.other == nextR) { ptrR = h.edgeIdx; gapR = false; }
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
    // Remove the edges on the wrong side: those clipped away entirely and those walked past.
    // Append the remaining edges and the new hyperplane
    VoronoiResult res;
    QVector<int> newIndex(nEdges, -1);
    QVector<bool> changed(nSites, false); // The region lost edges
    for (int i = 0; i < nEdges; ++i) {
        if (removed[i] || (passed[i] && !hit[i])) {
            changed[sites[i].first - firstSite] = changed[sites[i].second - firstSite] = true;
            continue;
        }
        newIndex[i] = res.edges.size();
        res.edges.append(edges[i]);
        res.sites.append(sites[i]);
    }
    const int firstHyperPlane = res.edges.size();
    res.edges.append(hyperPlane);
    res.sites.append(hyperPlaneSites);
    res.convexHull = mergedHull;
    res.firstSite = firstSite;
    res.anyEdge = QVector<int>(nSites, -1);
    res.next = res.prev = QVector<std::array<int, 2>>(res.edges.size(), {-1, -1});

    // Hyperplane edges of each region, in the order along the hyperplane
    QVector<QVector<int>> added(nSites);
    for (int k = 0; k < hyperPlane.size(); ++k) {
        added[hyperPlaneSites[k].first - firstSite].append(firstHyperPlane + k);
        added[hyperPlaneSites[k].second - firstSite].append(firstHyperPlane + k);
    }

    // Regions the hyperplane did not touch keep their edges and order
    for (int i = 0; i < nEdges; ++i) {
        if (newIndex[i] < 0) continue;
        for (int k = 0; k < 2; ++k) {
            int s = ((k == 0) ? sites[i].first : sites[i].second) - firstSite;
            if (changed[s] || !added[s].isEmpty()) continue;
            res.next[newIndex[i]][k] = newIndex[next[i][k]];
            res.prev[newIndex[i]][k] = newIndex[prev[i][k]];
        }
    }

    // The other regions: the remaining edges in their old order and the hyperplane edges are both sorted
    // counterclockwise by the direction to the other point, so merge them. Along the hyperplane, a right
    // region is passed counterclockwise and a left region clockwise
    auto direction = [&](int e, int s) {
        QPointF d = P[otherSite(res.sites, e, s)] - P[s];
        return std::atan2(d.y(), d.x()); // Counterclockwise on the screen (+y points down) is decreasing
    };
    // Rotate a list sorted counterclockwise (cyclically) to start at the largest angle; false if it is not sorted
    auto startAtLargest = [&](QVector<int>& list, int s) {
        if (list.size() < 2) return true;
        int top = 0;
        for (int i = 1; i < list.size(); ++i)
            if (direction(list[i], s) > direction(list[top], s)) top = i;
        std::rotate(list.begin(), list.begin() + top, list.end());
        for (int i = 1; i < list.size(); ++i)
            if (direction(list[i], s) > direction(list[i - 1], s)) return false;
        return true;
    };
    for (int s = 0; s < nSites; ++s) {
        const int site = firstSite + s;
        if (!changed[s] && added[s].isEmpty()) {
            if (anyEdge[s] >= 0) res.anyEdge[s] = newIndex[anyEdge[s]];
            continue;
        }
        QVector<int> kept;
        if (anyEdge[s] >= 0) {
            int e = anyEdge[s], count = 0;
            do {
                if (newIndex[e] >= 0) kept.append(newIndex[e]);
                e = next[e][(sites[e].first == site) ? 0 : 1];
            } while (e != anyEdge[s] && ++count <= nEdges);
        }
        QVector<int>& more = added[s];
        if (site < right.firstSite) std::reverse(more.begin(), more.end());
        QVector<int> order;
        auto later = [&](int a, int b) { return direction(a, site) > direction(b, site); };
        if (startAtLargest(kept, site) && startAtLargest(more, site))
            std::merge(kept.begin(), kept.end(), more.begin(), more.end(), std::back_inserter(order), later);
        else {
            unexpected = true;
            order = kept + more;
            std::sort(order.begin(), order.end(), later);
        }
        int m = order.size();
        for (int k = 0; k < m; ++k) {
            int e = order[k], side = (res.sites[e].first == site) ? 0 : 1;
            res.next[e][side] = order[(k + 1) % m];
            res.prev[e][side] = order[(k - 1 + m) % m];
        }
        if (m > 0) res.anyEdge[s] = order[0];
    }
    if (unexpected) qWarning() << "Merge: unexpected hyperplane";

    // Step 7. Final result
    stepEdges = res.edges;
    stepTypes = QVector<int>(res.edges.size(), 0); // 0: Normal/Result, red
    addStep(stepEdges, stepTypes, stepHull, "Step 7");

    return res;
}

QVector<int> VoronoiDiagram::mergeConvexHull(const QVector<int>& left, const QVector<int>& right, const QPair<int, int>& upperTan, const QPair<int, int>& lowerTan) {
    QVector<int> mergedHull;
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

QVector<QPointF> VoronoiDiagram::hullPoints(const QVector<int>& hull) {
    QVector<QPointF> points;
    for (int i : hull) points.append(P[i]);
    return points;
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

bool VoronoiDiagram::areAllCollinear(int first, int last) {
    if (last - first < 3) return true;

    const QPointF& p1 = P[first];
    const QPointF& p2 = P[first + 1];

    // Check if every subsequent point lies on the line defined by p1 and p2 (cross product is 0)
    for (int i = first + 2; i < last; ++i)
        if (std::abs(crossProduct(p1, p2, P[i])) > EPSILON)
            return false;
    return true;
}

VoronoiResult VoronoiDiagram::solveCollinear(int first, int last) {
    VoronoiResult res;

    for (int i = first; i < last - 1; ++i) {
        // Directly calculate the perpendicular bisector of adjacent points
        res.edges.append(getPerpBisector(P[i], P[i + 1]));
        res.sites.append(qMakePair(i, i + 1));
    }

    // Adding just the first and last points
    if (last > first) {
        res.convexHull.append(first);
        res.convexHull.append(last - 1);
    }
    linkRegions(res, first, last);
    return res;
}
