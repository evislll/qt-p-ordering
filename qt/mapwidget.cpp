#include "mapwidget.h"

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPushButton>
#include <QToolTip>
#include <QtMath>
#include <algorithm>

// NOTE: coordinates are approximate demo values around Don Mills / Sheppard (North York).
QList<Restaurant> defaultRestaurants()
{
    return {
        { "Pizza Pizza — Peanut Plaza",       "Don Mills Rd & Sheppard Ave E",
          "Open until 11:00 PM", ":/logos/images/logos.png",   43.7765, -79.3425, 4.6, 2.99 },
        { "Pizza Hut — 3555 Don Mills Rd",    "3555 Don Mills Rd",
          "Open until 12:00 AM", ":/logos/images/phut.png",    43.7872, -79.3425, 4.3, 3.49 },
        { "Pizza Hot Wings — 3883 Don Mills Rd", "3883 Don Mills Rd",
          "Open until 1:00 AM",  ":/logos/images/phwings.png", 43.8040, -79.3425, 4.4, 3.99 },
        { "Pizza Pizza — 5921 Leslie St",     "5921 Leslie St",
          "Open until 11:30 PM", ":/logos/images/logos.png",   43.7905, -79.3640, 4.5, 2.49 },
    };
}

double distanceKm(double lat1, double lon1, double lat2, double lon2)
{
    const double R = 6371.0;
    const double dLat = qDegreesToRadians(lat2 - lat1);
    const double dLon = qDegreesToRadians(lon2 - lon1);
    const double a = qSin(dLat / 2) * qSin(dLat / 2)
                   + qCos(qDegreesToRadians(lat1)) * qCos(qDegreesToRadians(lat2))
                     * qSin(dLon / 2) * qSin(dLon / 2);
    return 2 * R * qAsin(qSqrt(a));
}

int etaMinutesFor(const Restaurant &r)
{
    return 18 + qRound(distanceKm(kUserLat, kUserLon, r.lat, r.lon) * 4);
}

// ===================================================================
// MapWidget
// ===================================================================
MapWidget::MapWidget(QWidget *parent) : QWidget(parent)
{
    setMouseTracking(true);
    setMinimumSize(320, 320);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto mk = [this](const QString &t) {
        auto *b = new QPushButton(t, this);
        b->setObjectName("mapBtn");
        b->setFixedSize(36, 36);
        b->setCursor(Qt::PointingHandCursor);
        return b;
    };
    m_zin  = mk("+");
    m_zout = mk(QString::fromUtf8("\u2212"));
    m_fit  = mk(QString::fromUtf8("\u2316"));
    m_fit->setToolTip("Show all restaurants");
    connect(m_zin,  &QPushButton::clicked, this, [this]{ zoomBy(1.4,  QPointF(width() / 2.0, height() / 2.0)); });
    connect(m_zout, &QPushButton::clicked, this, [this]{ zoomBy(1/1.4, QPointF(width() / 2.0, height() / 2.0)); });
    connect(m_fit,  &QPushButton::clicked, this, [this]{ fitToContent(); update(); });
}

void MapWidget::setRestaurants(const QList<Restaurant> &items)
{
    m_items = items;
    m_fitted = false;
    if (width() > 0) fitToContent();
    update();
}

void MapWidget::setSelected(int i)
{
    if (i == m_sel) return;
    m_sel = i;
    update();
}

void MapWidget::resizeEvent(QResizeEvent *)
{
    const int x = width() - 36 - 12;
    m_zin->move(x, 12);
    m_zout->move(x, 12 + 40);
    m_fit->move(x, 12 + 80);
    if (!m_fitted) fitToContent();
}

QPointF MapWidget::toScreen(double lat, double lon) const
{
    const double k = qCos(qDegreesToRadians(m_cLat));
    return { width() / 2.0 + (lon - m_cLon) * m_scale * k,
             height() / 2.0 - (lat - m_cLat) * m_scale };
}

void MapWidget::fromScreen(const QPointF &pt, double &lat, double &lon) const
{
    const double k = qCos(qDegreesToRadians(m_cLat));
    lon = m_cLon + (pt.x() - width() / 2.0) / (m_scale * k);
    lat = m_cLat - (pt.y() - height() / 2.0) / m_scale;
}

void MapWidget::fitToContent()
{
    if (width() < 50 || height() < 50) return;
    double minLat = kUserLat, maxLat = kUserLat, minLon = kUserLon, maxLon = kUserLon;
    for (const auto &r : m_items) {
        minLat = qMin(minLat, r.lat); maxLat = qMax(maxLat, r.lat);
        minLon = qMin(minLon, r.lon); maxLon = qMax(maxLon, r.lon);
    }
    m_cLat = (minLat + maxLat) / 2;
    m_cLon = (minLon + maxLon) / 2;
    const double k = qCos(qDegreesToRadians(m_cLat));
    const double sx = (width()  - 160) / qMax(0.002, (maxLon - minLon) * k);
    const double sy = (height() - 140) / qMax(0.002, (maxLat - minLat));
    m_scale = m_baseScale = qBound(3000.0, qMin(sx, sy), 60000.0);
    m_fitted = true;
}

void MapWidget::zoomBy(double factor, const QPointF &anchor)
{
    double lat, lon;
    fromScreen(anchor, lat, lon);
    m_scale = qBound(m_baseScale * 0.5, m_scale * factor, m_baseScale * 6.0);
    const QPointF p2 = toScreen(lat, lon);
    const double k = qCos(qDegreesToRadians(m_cLat));
    m_cLon += (p2.x() - anchor.x()) / (m_scale * k);
    m_cLat -= (p2.y() - anchor.y()) / m_scale;
    update();
}

void MapWidget::wheelEvent(QWheelEvent *e)
{
    zoomBy(e->angleDelta().y() > 0 ? 1.2 : 1 / 1.2, e->position());
    e->accept();
}

int MapWidget::pinAt(const QPoint &pos) const
{
    for (int i = m_items.size() - 1; i >= 0; --i) {
        const QPointF tip = toScreen(m_items[i].lat, m_items[i].lon);
        const double s = (i == m_sel) ? 1.25 : 1.0;
        if (QLineF(pos, tip + QPointF(0, -18 * s)).length() < 16 * s) return i;
    }
    return -1;
}

void MapWidget::mousePressEvent(QMouseEvent *e)
{
    m_press = m_last = e->pos();
    m_dragging = false;
}

void MapWidget::mouseMoveEvent(QMouseEvent *e)
{
    if (e->buttons() & Qt::LeftButton) {
        if (!m_dragging && (e->pos() - m_press).manhattanLength() > 5) m_dragging = true;
        if (m_dragging) {
            const double k = qCos(qDegreesToRadians(m_cLat));
            m_cLon -= (e->pos().x() - m_last.x()) / (m_scale * k);
            m_cLat += (e->pos().y() - m_last.y()) / m_scale;
            m_last = e->pos();
            setCursor(Qt::ClosedHandCursor);
            update();
        }
        return;
    }
    const int h = pinAt(e->pos());
    if (h != m_hover) { m_hover = h; update(); }
    setCursor(h >= 0 ? Qt::PointingHandCursor : Qt::OpenHandCursor);
}

void MapWidget::mouseReleaseEvent(QMouseEvent *e)
{
    if (!m_dragging) {
        const int i = pinAt(e->pos());
        if (i >= 0) { setSelected(i); emit restaurantClicked(i); }
    }
    m_dragging = false;
    setCursor(Qt::OpenHandCursor);
}

void MapWidget::drawPin(QPainter &p, const QPointF &tip, const QColor &c, double s, const QString &label) const
{
    p.save();
    p.translate(tip);
    p.scale(s, s);

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 0, 40));               // ground shadow
    p.drawEllipse(QPointF(0, 0), 8, 3);

    QPainterPath pin;
    pin.addEllipse(QPointF(0, -18), 13, 13);
    QPainterPath tail;
    tail.moveTo(-9, -10); tail.lineTo(0, 0); tail.lineTo(9, -10); tail.closeSubpath();
    pin = pin.united(tail);
    p.setBrush(c);
    p.setPen(QPen(Qt::white, 2));
    p.drawPath(pin);

    p.setPen(Qt::NoPen);
    p.setBrush(Qt::white);
    p.drawEllipse(QPointF(0, -18), 5, 5);
    p.restore();

    if (!label.isEmpty()) {
        QFont f = font(); f.setPixelSize(13); f.setBold(true); p.setFont(f);
        const QFontMetrics fm(f);
        const int w = fm.horizontalAdvance(label) + 20;
        QRectF bubble(tip.x() - w / 2.0, tip.y() - 18 * s - 30 * s - 28, w, 26);
        p.setPen(QPen(QColor("#d0d7de"), 1));
        p.setBrush(Qt::white);
        p.drawRoundedRect(bubble, 8, 8);
        p.setPen(QColor("#1b2733"));
        p.drawText(bubble, Qt::AlignCenter, label);
    }
}

void MapWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setClipRegion(QRegion(rect()));

    // rounded clip so the map matches the card corners
    QPainterPath clip; clip.addRoundedRect(rect(), 12, 12);
    p.setClipPath(clip);
    p.fillRect(rect(), QColor("#e8eef3"));

    const double zoom = m_scale / m_baseScale;
    double lat0, lon0, lat1, lon1;
    fromScreen(QPointF(0, height()), lat0, lon0);
    fromScreen(QPointF(width(), 0), lat1, lon1);

    auto P = [this](double lat, double lon) { return toScreen(lat, lon); };

    // parks
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#cde7cd"));
    const double parks[][4] = { {43.7835,-79.3575,43.7895,-79.3505}, {43.7950,-79.3385,43.8010,-79.3320},
                                {43.7685,-79.3385,43.7740,-79.3290}, {43.8000,-79.3720,43.8060,-79.3650} };
    for (auto &pk : parks) {
        const QPointF a = P(pk[2], pk[1]), b = P(pk[0], pk[3]);
        p.drawRoundedRect(QRectF(a, b).normalized(), 10, 10);
    }

    // river (smooth curve)
    const double river[][2] = { {43.812,-79.3560},{43.803,-79.3500},{43.795,-79.3545},{43.786,-79.3500},{43.777,-79.3545},{43.768,-79.3510} };
    QPainterPath rp;
    rp.moveTo(P(river[0][0], river[0][1]));
    for (int i = 1; i < 6; ++i) {
        const QPointF a = P(river[i - 1][0], river[i - 1][1]), b = P(river[i][0], river[i][1]);
        rp.cubicTo(a + QPointF(0, (b.y() - a.y()) / 2), b - QPointF(0, (b.y() - a.y()) / 2), b);
    }
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(QColor("#b9d7ee"), qMax(6.0, 14 * zoom), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.drawPath(rp);

    // minor street grid
    QPen minor(QColor("#ffffff"), qMax(1.0, 2.2 * zoom));
    p.setPen(minor);
    const double stepLat = 0.0035, stepLon = 0.0050;
    for (double la = qFloor(lat0 / stepLat) * stepLat; la <= lat1; la += stepLat)
        p.drawLine(P(la, lon0), P(la, lon1));
    for (double lo = qFloor(lon0 / stepLon) * stepLon; lo <= lon1; lo += stepLon)
        p.drawLine(P(lat0, lo), P(lat1, lo));

    // major roads
    struct Road { double a, b, c, d; const char *name; };       // a,b = start lat,lon  c,d = end lat,lon
    const Road roads[] = {
        {43.760,-79.3425,43.825,-79.3425,"Don Mills Rd"},
        {43.760,-79.3640,43.825,-79.3640,"Leslie St"},
        {43.760,-79.3225,43.825,-79.3225,"Victoria Park Ave"},
        {43.7765,-79.385,43.7765,-79.305,"Sheppard Ave E"},
        {43.7955,-79.385,43.7955,-79.305,"Finch Ave E"},
        {43.8160,-79.385,43.8160,-79.305,"Steeles Ave E"},
    };
    for (const auto &r : roads) {
        p.setPen(QPen(QColor("#c9d2da"), qMax(5.0, 11 * zoom), Qt::SolidLine, Qt::RoundCap));
        p.drawLine(P(r.a, r.b), P(r.c, r.d));
        p.setPen(QPen(Qt::white, qMax(3.5, 8 * zoom), Qt::SolidLine, Qt::RoundCap));
        p.drawLine(P(r.a, r.b), P(r.c, r.d));
    }
    // highway
    p.setPen(QPen(QColor("#f4d58d"), qMax(5.0, 10 * zoom), Qt::SolidLine, Qt::RoundCap));
    p.drawLine(P(43.760, -79.3300), P(43.825, -79.3345));

    // road labels
    if (zoom > 0.75) {
        QFont lf = font(); lf.setPixelSize(12); lf.setBold(true);
        p.setFont(lf);
        p.setPen(QColor("#7d8a96"));
        for (const auto &r : roads) {
            const bool vertical = qFuzzyCompare(r.b, r.d);
            const QPointF mid = vertical ? P(m_cLat + 0.004, r.b) : P(r.a, m_cLon - 0.012);
            if (!rect().adjusted(40, 20, -60, -20).contains(mid.toPoint())) continue;
            p.save();
            p.translate(mid);
            if (vertical) p.rotate(-90);
            p.drawText(QRectF(-70, -16, 140, 14), Qt::AlignCenter, r.name);
            p.restore();
        }
    }

    // "you are here"
    {
        const QPointF u = P(kUserLat, kUserLon);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(21, 101, 192, 45));
        p.drawEllipse(u, 20, 20);
        p.setBrush(Qt::white);
        p.drawEllipse(u, 9, 9);
        p.setBrush(QColor("#1565C0"));
        p.drawEllipse(u, 6.5, 6.5);
        QFont f = font(); f.setPixelSize(12); p.setFont(f);
        p.setPen(QColor("#1565C0"));
        p.drawText(QRectF(u.x() - 40, u.y() + 12, 80, 16), Qt::AlignCenter, "You");
    }

    // pins (selected last so it sits on top)
    for (int i = 0; i < m_items.size(); ++i) {
        if (i == m_sel) continue;
        drawPin(p, P(m_items[i].lat, m_items[i].lon), QColor("#4CAF50"),
                i == m_hover ? 1.12 : 1.0, i == m_hover ? m_items[i].name.section(QString::fromUtf8(" —"), 0, 0) : QString());
    }
    if (m_sel >= 0 && m_sel < m_items.size())
        drawPin(p, P(m_items[m_sel].lat, m_items[m_sel].lon), QColor("#2196F3"), 1.25,
                m_items[m_sel].name);

    // border
    p.setClipping(false);
    p.setPen(QPen(QColor("#d0d7de"), 1));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), 12, 12);

    QFont cf = font(); cf.setPixelSize(11); p.setFont(cf);
    p.setPen(QColor("#9aa5b1"));
    p.drawText(rect().adjusted(10, 0, -10, -6), Qt::AlignLeft | Qt::AlignBottom,
               "Demo map · drag to pan, scroll to zoom");
}

// ===================================================================
// TrackMap
// ===================================================================
TrackMap::TrackMap(QWidget *parent) : QWidget(parent)
{
    setMinimumHeight(230);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void TrackMap::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    QPainterPath clip; clip.addRoundedRect(rect(), 12, 12);
    p.setClipPath(clip);
    p.fillRect(rect(), QColor("#e8eef3"));

    p.setPen(QPen(Qt::white, 3));
    for (int x = 30; x < width(); x += 70) p.drawLine(x, 0, x, height());
    for (int y = 25; y < height(); y += 60) p.drawLine(0, y, width(), y);
    p.setPen(QPen(QColor("#cde7cd"), 1));
    p.setBrush(QColor("#cde7cd"));
    p.drawRoundedRect(QRectF(width() * 0.42, height() * 0.58, width() * 0.2, height() * 0.28), 10, 10);

    const QPointF a(46, height() - 44), b(width() - 46, 44);
    QPainterPath path(a);
    path.cubicTo(QPointF(width() * 0.28, height() * 0.05), QPointF(width() * 0.62, height() * 0.95), b);

    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(QColor("#9fb3c5"), 4, Qt::DashLine, Qt::RoundCap));
    p.drawPath(path);

    // travelled part
    QPainterPath done(a);
    const int steps = int(m_p * 80);
    for (int i = 1; i <= steps; ++i) done.lineTo(path.pointAtPercent(i / 80.0));
    p.setPen(QPen(QColor("#2196F3"), 5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.drawPath(done);

    auto marker = [&](const QPointF &c, const QColor &col, const QString &emoji) {
        p.setPen(QPen(Qt::white, 3));
        p.setBrush(col);
        p.drawEllipse(c, 19, 19);
        QFont f = font(); f.setPixelSize(18); p.setFont(f);
        p.setPen(Qt::white);
        p.drawText(QRectF(c.x() - 19, c.y() - 19, 38, 38), Qt::AlignCenter, emoji);
    };
    marker(a, QColor("#E53935"), QString::fromUtf8("\U0001F355"));
    marker(b, QColor("#4CAF50"), QString::fromUtf8("\U0001F3E0"));

    const QPointF s = path.pointAtPercent(m_p);
    p.setPen(QPen(Qt::white, 3));
    p.setBrush(QColor("#2196F3"));
    p.drawEllipse(s, 17, 17);
    QFont f = font(); f.setPixelSize(18); p.setFont(f);
    p.setPen(Qt::white);
    p.drawText(QRectF(s.x() - 17, s.y() - 17, 34, 34), Qt::AlignCenter, QString::fromUtf8("\U0001F6F5"));

    p.setClipping(false);
    p.setPen(QPen(QColor("#d0d7de"), 1));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), 12, 12);
}
