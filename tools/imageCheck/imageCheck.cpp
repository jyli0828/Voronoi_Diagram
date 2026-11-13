// Check the drawn Voronoi diagrams pixel by pixel against an independent brute-force answer.
//
// Every test case of an input file is drawn on the program's CanvasWidget, the same as after pressing Run. The answer
// it is checked against does not use VoronoiDiagram.cpp: the edges (with their end points), the convex hull and the
// points all come from the input file alone.
//
// Usage: imageCheck <input file> [options]
//   --save <dir>        Also save the drawn images as <dir>/<input file name>_<k>.png
//   --images <pattern>  Check existing PNG files instead of drawing, %1 is the case number (e.g. "png/52_%1.png")
//   --mutate            Damage each image first (erase part of an edge, add a fake edge, shift an edge, erase a point)
//                       to see that the check catches it: every case should then be WRONG
//   --where             Print where each problem is
// Exit code 0 when every image is right.
//
// The checks, for an image W x H:
//   - Every Voronoi edge is drawn: points every 0.5 px along it have a red pixel within 1.6 px, or lie under a
//     cyan hull line drawn later (within 1.5 px)
//   - No extra red: every red pixel is within 2 px of a Voronoi edge
//   - The convex hull is drawn in cyan, checked the same way
//   - Every point shows as a dark dot (or is fully covered by lines, but then its middle is not white), and every dark
//     pixel is within 3 px of a point
#include "canvaswidget.h"
#include <QApplication>
#include <QImage>
#include <QPainter>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QRegularExpression>
#include <QSet>
#include <cmath>
#include <cstdio>
#include <algorithm>
#include <limits>

static bool showWhere = false;

static QVector<QVector<QPointF>> readCases(const QString &path) { // Same rules as MainWindow::readInputFormat
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return {};
    QTextStream ts(&f);
    QVector<QVector<QPointF>> cases;
    while (!ts.atEnd()) {
        bool ok = false; int n = -1;
        while (!ts.atEnd()) {
            QString line = ts.readLine().section('#', 0, 0).trimmed();
            if (line.isEmpty()) continue;
            n = line.toInt(&ok);
            if (ok) break;
        }
        if (!ok || n == 0) break;
        QVector<QPointF> pts;
        while (pts.size() < n && !ts.atEnd()) {
            QStringList parts = ts.readLine().section('#', 0, 0).trimmed().split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
            if (parts.size() >= 2) {
                bool okX, okY;
                double x = parts[0].toDouble(&okX), y = parts[1].toDouble(&okY);
                if (okX && okY) pts.append(QPointF(x, y));
            }
        }
        cases.append(pts);
    }
    return cases;
}

// Voronoi edges by brute force: for every pair, the part of the bisector closer to both than to any other point.
// Clipped 10 px past the image like the canvas does, since a line just outside the border still tints the border pixels.
static QVector<QLineF> bruteEdges(const QVector<QPointF> &p, double W, double H) {
    QVector<QLineF> out;
    const double INF = std::numeric_limits<double>::infinity();
    int n = p.size();
    for (int i = 0; i < n; ++i)
        for (int j = i + 1; j < n; ++j) {
            QPointF m = (p[i] + p[j]) / 2, v = p[j] - p[i];
            double len = std::hypot(v.x(), v.y());
            QPointF d(-v.y() / len, v.x() / len); // Points on the bisector are m + t d
            double lo = -INF, hi = INF;
            auto limit = [&](double a, double b) { // Keep t with a t <= b
                if (std::abs(a) < 1e-12) { if (b < 0) hi = lo - 1; }
                else if (a > 0) hi = std::min(hi, b / a);
                else lo = std::max(lo, b / a);
            };
            for (int k = 0; k < n && lo < hi; ++k) { // Closer to p[i] than to p[k]: 2 (pk - pi) . P <= |pk|^2 - |pi|^2
                if (k == i || k == j) continue;
                QPointF w = p[k] - p[i];
                limit(2 * (w.x() * d.x() + w.y() * d.y()),
                      (p[k].x() * p[k].x() + p[k].y() * p[k].y() - p[i].x() * p[i].x() - p[i].y() * p[i].y())
                          - 2 * (w.x() * m.x() + w.y() * m.y()));
            }
            if (!(hi - lo > 1e-6)) continue;
            limit(-d.x(), m.x() + 10); limit(d.x(), W + 10 - m.x()); limit(-d.y(), m.y() + 10); limit(d.y(), H + 10 - m.y());
            if (hi - lo > 1e-6) out.append(QLineF(m + lo * d, m + hi * d));
        }
    return out;
}

static QVector<QPointF> convexHull(QVector<QPointF> p) { // Monotone chain, points already distinct
    std::sort(p.begin(), p.end(), [](const QPointF &a, const QPointF &b) { return a.x() < b.x() || (a.x() == b.x() && a.y() < b.y()); });
    if (p.size() < 3) return p;
    auto cross = [](QPointF o, QPointF a, QPointF b) { return (a.x() - o.x()) * (b.y() - o.y()) - (a.y() - o.y()) * (b.x() - o.x()); };
    QVector<QPointF> h(2 * p.size());
    int k = 0;
    for (int i = 0; i < p.size(); ++i) { while (k >= 2 && cross(h[k - 2], h[k - 1], p[i]) <= 0) --k; h[k++] = p[i]; }
    for (int i = p.size() - 2, t = k + 1; i >= 0; --i) { while (k >= t && cross(h[k - 2], h[k - 1], p[i]) <= 0) --k; h[k++] = p[i]; }
    h.resize(k - 1);
    return h;
}

static double segDist(QPointF q, const QLineF &s) {
    QPointF a = s.p1(), b = s.p2(), ab = b - a;
    double L = ab.x() * ab.x() + ab.y() * ab.y();
    double t = L > 0 ? std::clamp(((q.x() - a.x()) * ab.x() + (q.y() - a.y()) * ab.y()) / L, 0.0, 1.0) : 0;
    QPointF c = a + t * ab;
    return std::hypot(q.x() - c.x(), q.y() - c.y());
}

// Segments grouped by 10 px cells, for nearest-segment queries within pad
struct Grid {
    int cw, ch; QVector<QVector<int>> cells; const QVector<QLineF> *segs;
    Grid(const QVector<QLineF> &s, int W, int H, double pad) : cw(W / 10 + 1), ch(H / 10 + 1), cells(cw * ch), segs(&s) {
        for (int i = 0; i < s.size(); ++i) {
            QPointF a = s[i].p1(), b = s[i].p2();
            int steps = std::max(1, (int)std::ceil(s[i].length() / 5));
            QSet<int> seen;
            for (int k = 0; k <= steps; ++k) {
                QPointF c = a + (b - a) * (double(k) / steps);
                for (int gx = int((c.x() - pad - 5) / 10); gx <= int((c.x() + pad + 5) / 10); ++gx)
                    for (int gy = int((c.y() - pad - 5) / 10); gy <= int((c.y() + pad + 5) / 10); ++gy)
                        if (gx >= 0 && gy >= 0 && gx < cw && gy < ch && !seen.contains(gy * cw + gx)) {
                            seen.insert(gy * cw + gx);
                            cells[gy * cw + gx].append(i);
                        }
            }
        }
    }
    double nearest(QPointF q) const {
        int gx = std::clamp(int(q.x() / 10), 0, cw - 1), gy = std::clamp(int(q.y() / 10), 0, ch - 1);
        double best = 1e9;
        for (int i : cells[gy * cw + gx]) best = std::min(best, segDist(q, (*segs)[i]));
        return best;
    }
};

enum Kind { White, Red, Cyan, Black, Other };
static Kind kind(QRgb c) {
    int r = qRed(c), g = qGreen(c), b = qBlue(c);
    if (r > 235 && g > 235 && b > 235) return White;
    if (r > g + 60 && r > b + 60) return Red;
    if (g > r + 60 && b > r + 60) return Cyan;
    if (r < 110 && g < 110 && b < 110) return Black;
    return Other;
}

struct Result { int missing = 0; double longestGap = 0; int extra = 0; };

// Every truth segment must be drawn in the given color (or lie under a line drawn later), and every pixel of that
// color must lie on a truth segment
static Result compare(const QImage &img, const QVector<QLineF> &truth, Kind color, const QVector<Kind> &hiders) {
    const double tolCover = 1.6, tolHidden = 1.5, tolExtra = 2.0;
    const char *name = color == Red ? "red" : "cyan";
    Result r;
    int W = img.width(), H = img.height();
    for (const QLineF &s : truth) {
        double len = s.length(), gapStart = -1;
        int steps = std::max(1, (int)std::ceil(len / 0.5));
        for (int k = 0; k <= steps; ++k) {
            QPointF q = s.pointAt(double(k) / steps);
            if (q.x() < 0.5 || q.y() < 0.5 || q.x() > W - 0.5 || q.y() > H - 0.5) continue; // Not inside the image
            bool seen = false, hidden = false;
            for (int x = int(q.x() - 2.5); x <= int(q.x() + 2.5); ++x)
                for (int y = int(q.y() - 2.5); y <= int(q.y() + 2.5); ++y) {
                    if (x < 0 || y < 0 || x >= W || y >= H) continue;
                    double d = std::hypot(x + 0.5 - q.x(), y + 0.5 - q.y());
                    Kind kd = kind(img.pixel(x, y));
                    if (d <= tolCover && kd == color) seen = true;
                    if (d <= tolHidden && hiders.contains(kd)) hidden = true; // The mix under a later line is neither color
                }
            double here = len * k / steps;
            if (seen || hidden) { gapStart = -1; continue; }
            ++r.missing;
            if (showWhere) std::printf("    %s not drawn at (%.2f, %.2f), on (%.2f, %.2f)-(%.2f, %.2f)\n", name, q.x(), q.y(), s.x1(), s.y1(), s.x2(), s.y2());
            if (gapStart < 0) gapStart = here;
            r.longestGap = std::max(r.longestGap, here - gapStart + 0.5);
        }
    }
    Grid g(truth, W, H, tolExtra);
    for (int x = 0; x < W; ++x)
        for (int y = 0; y < H; ++y)
            if (kind(img.pixel(x, y)) == color && g.nearest(QPointF(x + 0.5, y + 0.5)) > tolExtra) {
                ++r.extra;
                if (showWhere) std::printf("    extra %s pixel at (%d, %d)\n", name, x, y);
            }
    return r;
}

// Damage the image in one of four ways, chosen by the case number
static QString mutate(QImage &img, int c, const QVector<QLineF> &edges, const QVector<QLineF> &hullSegs,
                      const QVector<QPointF> &pts, const QVector<QPointF> &hull) {
    QPainter pt(&img);
    pt.setRenderHint(QPainter::Antialiasing);
    QLineF longest;
    for (const QLineF &e : edges) if (e.length() > longest.length()) longest = e;
    int W = img.width(), H = img.height();
    if (c % 4 == 0 && !edges.isEmpty()) { // Cut 6 px out of the middle of the longest edge
        pt.setPen(QPen(Qt::white, 4, Qt::SolidLine, Qt::FlatCap));
        pt.drawLine(QLineF(longest.pointAt(0.5), longest.pointAt(0.5 + 6 / longest.length())));
        return "erase 6 px of an edge";
    }
    if (c % 4 == 1) { // A fake 30 px red edge in the emptiest spot
        QPointF best; double far = -1;
        for (int x = 20; x < W - 40; x += 7)
            for (int y = 30; y < H - 20; y += 7) {
                QPointF q(x, y); double d = 1e9;
                for (const QLineF &e : edges) d = std::min(d, segDist(q, e));
                for (const QLineF &e : hullSegs) d = std::min(d, segDist(q, e));
                for (const QPointF &p : pts) d = std::min(d, std::hypot(q.x() - p.x(), q.y() - p.y()));
                if (d > far) { far = d; best = q; }
            }
        pt.setPen(QPen(Qt::red, 2));
        pt.drawLine(QLineF(best, best + QPointF(30, 0)));
        return "add a 30 px red line";
    }
    if (c % 4 == 2 && !edges.isEmpty()) { // Move the longest edge 3 px sideways
        pt.setPen(QPen(Qt::white, 4, Qt::SolidLine, Qt::FlatCap));
        pt.drawLine(longest);
        QPointF nrm = longest.normalVector().unitVector().p2() - longest.p1();
        pt.setPen(QPen(Qt::red, 2));
        pt.drawLine(longest.translated(3 * nrm));
        return "shift an edge 3 px";
    }
    if (pts.isEmpty()) return "";
    QPointF p = pts[0]; // Erase a point, one not on the hull when there is one
    for (const QPointF &q : pts) if (!hull.contains(q)) { p = q; break; }
    pt.setPen(Qt::NoPen);
    pt.setBrush(Qt::white);
    pt.drawEllipse(p, 3, 3);
    return "erase a point";
}

int main(int argc, char **argv) {
#ifdef Q_OS_WIN
    qputenv("QT_QPA_PLATFORM", "windows:darkmode=0"); // Same colors as the report images in any Windows theme
#endif
    QApplication app(argc, argv);
    QStringList args = app.arguments().mid(1);
    QString input, saveDir, pattern;
    bool damage = false;
    for (int i = 0; i < args.size(); ++i) {
        if (args[i] == "--save" && i + 1 < args.size()) saveDir = args[++i];
        else if (args[i] == "--images" && i + 1 < args.size()) pattern = args[++i];
        else if (args[i] == "--mutate") damage = true;
        else if (args[i] == "--where") showWhere = true;
        else if (input.isEmpty() && !args[i].startsWith("--")) input = args[i];
        else { input.clear(); break; }
    }
    if (input.isEmpty()) {
        std::printf("Usage: imageCheck <input file> [--save <dir>] [--images <pattern with %%1>] [--mutate] [--where]\n");
        return 2;
    }
    if (!QFileInfo::exists(input)) { std::printf("Cannot open %s\n", qPrintable(input)); return 2; }
    auto cases = readCases(input);

    int bad = 0;
    for (int c = 0; c < cases.size(); ++c) {
        QVector<QPointF> pts; // Repeated points count once
        for (const QPointF &p : cases[c]) if (!pts.contains(p)) pts.append(p);

        QImage img;
        if (pattern.isEmpty()) { // Draw it the same way as pressing Run
            CanvasWidget w;
            w.setPoints(cases[c]);
            w.runCalculation();
            img = w.grab().toImage();
            if (!saveDir.isEmpty())
                img.save(saveDir + "/" + QFileInfo(input).completeBaseName() + QString("_%1.png").arg(c + 1));
        } else {
            img = QImage(pattern.arg(c + 1));
            if (img.isNull()) { std::printf("case %2d: cannot open %s\n", c + 1, qPrintable(pattern.arg(c + 1))); ++bad; continue; }
        }
        img = img.convertToFormat(QImage::Format_RGB32);
        int W = img.width(), H = img.height();

        QVector<QLineF> edges = pts.size() >= 2 ? bruteEdges(pts, W, H) : QVector<QLineF>();
        QVector<QPointF> hull = pts.size() >= 2 ? convexHull(pts) : QVector<QPointF>();
        QVector<QLineF> hullSegs;
        for (int i = 0; hull.size() >= 2 && i < hull.size(); ++i) hullSegs.append(QLineF(hull[i], hull[(i + 1) % hull.size()]));
        QString note = damage ? mutate(img, c, edges, hullSegs, pts, hull) : QString();

        Result re = compare(img, edges, Red, {Cyan});
        Result rh = compare(img, hullSegs, Cyan, {});

        int missingPts = 0, strayDark = 0;
        for (const QPointF &p : pts) {
            int dark = 0, white = 0;
            for (int x = int(p.x()) - 3; x <= int(p.x()) + 3; ++x)
                for (int y = int(p.y()) - 3; y <= int(p.y()) + 3; ++y) {
                    if (x < 0 || y < 0 || x >= W || y >= H) continue;
                    double d = std::hypot(x + 0.5 - p.x(), y + 0.5 - p.y());
                    Kind kd = kind(img.pixel(x, y));
                    if (d <= 2.2 && kd == Black) ++dark;
                    if (d <= 1.2 && kd == White) ++white;
                }
            if (dark == 0 && white > 0) {
                ++missingPts;
                if (showWhere) std::printf("    point (%g, %g) not drawn\n", p.x(), p.y());
            }
        }
        for (int x = 0; x < W; ++x)
            for (int y = 0; y < H; ++y) {
                if (kind(img.pixel(x, y)) != Black) continue;
                bool near = false;
                for (const QPointF &p : pts) if (std::hypot(x + 0.5 - p.x(), y + 0.5 - p.y()) <= 3.0) { near = true; break; }
                if (!near) {
                    ++strayDark;
                    if (showWhere) std::printf("    stray dark pixel at (%d, %d)\n", x, y);
                }
            }

        bool ok = re.missing == 0 && re.extra == 0 && rh.missing == 0 && rh.extra == 0 && missingPts == 0 && strayDark == 0;
        if (!ok) ++bad;
        std::printf("case %2d: n=%4lld %dx%d | edges %4lld missing %d (longest gap %.1f px) extra %d"
                    " | hull %3lld missing %d extra %d | points missing %d, stray dark %d  %s%s\n",
                    c + 1, (long long)pts.size(), W, H, (long long)edges.size(), re.missing, re.longestGap, re.extra,
                    (long long)hullSegs.size(), rh.missing, rh.extra, missingPts, strayDark, ok ? "OK" : "WRONG",
                    note.isEmpty() ? "" : qPrintable("  <- " + note));
        fflush(stdout);
    }
    std::printf("%s: %d of %lld images wrong\n", qPrintable(input), bad, (long long)cases.size());
    return bad != 0;
}
