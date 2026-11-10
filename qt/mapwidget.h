#ifndef MAPWIDGET_H
#define MAPWIDGET_H

#include <QWidget>
#include <QList>
#include <QString>

class QPushButton;

// Demo "user" position (used for the distance / ETA numbers and the blue ring).
constexpr double kUserLat = 43.7825;
constexpr double kUserLon = -79.3500;

struct Restaurant {
    QString name;
    QString address;
    QString hours;
    QString icon;      // Qt resource path (may be missing -> fallback logo)
    double  lat = 0;
    double  lon = 0;
    double  rating = 4.5;
    double  fee = 2.99;
};

QList<Restaurant> defaultRestaurants();
double distanceKm(double lat1, double lon1, double lat2, double lon2);
int    etaMinutesFor(const Restaurant &r);

// ------------------------------------------------------------------
// A self-contained, offline map drawn with QPainter (no web engine and
// no internet needed): pan with the mouse, zoom with the wheel or the
// +/- buttons, click a pin to select a restaurant.
// ------------------------------------------------------------------
class MapWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MapWidget(QWidget *parent = nullptr);

    void setRestaurants(const QList<Restaurant> &items);
    void setSelected(int index);
    int  selected() const { return m_sel; }
    QSize sizeHint() const override { return QSize(560, 400); }

signals:
    void restaurantClicked(int index);

protected:
    void paintEvent(QPaintEvent *) override;
    void resizeEvent(QResizeEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void wheelEvent(QWheelEvent *) override;

private:
    QPointF toScreen(double lat, double lon) const;
    void    fromScreen(const QPointF &pt, double &lat, double &lon) const;
    void    fitToContent();
    void    zoomBy(double factor, const QPointF &anchor);
    int     pinAt(const QPoint &pos) const;
    void    drawPin(QPainter &p, const QPointF &tip, const QColor &c, double scale, const QString &label) const;

    QList<Restaurant> m_items;
    int    m_sel = -1, m_hover = -1;
    double m_cLat = 43.79, m_cLon = -79.35, m_scale = 12000, m_baseScale = 12000;
    bool   m_fitted = false, m_dragging = false;
    QPoint m_press, m_last;
    QPushButton *m_zin, *m_zout, *m_fit;
};

// ------------------------------------------------------------------
// Small route preview for the order-tracking page: restaurant -> home
// with a scooter that moves as the delivery progresses (0..1).
// ------------------------------------------------------------------
class TrackMap : public QWidget
{
    Q_OBJECT
public:
    explicit TrackMap(QWidget *parent = nullptr);
    void setProgress(double p) { m_p = qBound(0.0, p, 1.0); update(); }
    QSize sizeHint() const override { return QSize(420, 260); }
protected:
    void paintEvent(QPaintEvent *) override;
private:
    double m_p = 0;
};

#endif // MAPWIDGET_H
