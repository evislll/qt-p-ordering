#include "steprail.h"

#include <QPainter>
#include <QMouseEvent>

StepRail::StepRail(QWidget *parent) : QWidget(parent)
{
    setMinimumWidth(240);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    setMouseTracking(true);
}

void StepRail::setSteps(const QStringList &labels)
{
    m_labels = labels;
    m_hitRects.clear();
    for (int i = 0; i < m_labels.size(); ++i) {
        const int y = m_top + i * m_rowHeight;
        m_hitRects.push_back(QRect(m_leftMargin - m_radius, y, m_radius * 2, m_radius * 2));
    }
    updateGeometry();
    update();
}

void StepRail::setSubtitles(const QStringList &subs) { m_subs = subs; update(); }

void StepRail::setCurrentStep(int index)
{
    m_current = index;
    if (index > m_completedUpTo) m_completedUpTo = index - 1;
    update();
}

void StepRail::setCompletedUpTo(int index) { m_completedUpTo = index; update(); }
void StepRail::setAllCompleted(bool on)    { m_allDone = on; update(); }

QSize StepRail::sizeHint() const
{
    return QSize(250, m_top + m_labels.size() * m_rowHeight + 20);
}

void StepRail::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.fillRect(rect(), QColor("#ffffff"));

    QFont f = font();
    f.setPixelSize(15);

    for (int i = 0; i < m_labels.size(); ++i) {
        const QRect r = m_hitRects[i];
        const QPoint c = r.center();
        const bool done = m_allDone || i < m_current;
        const bool cur  = (i == m_current);

        // connector
        if (i < m_labels.size() - 1) {
            const QPoint next = m_hitRects[i + 1].center();
            QPen linePen(QColor("#dfe3e8"), 3, Qt::SolidLine, Qt::RoundCap);
            if (m_allDone || i < m_current) linePen.setColor(QColor("#4CAF50"));
            p.setPen(linePen);
            p.drawLine(c.x(), c.y() + m_radius + 4, next.x(), next.y() - m_radius - 4);
        }

        // soft halo on the current step
        if (cur && !m_allDone) {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(33, 150, 243, 40));
            p.drawEllipse(c, m_radius + 6, m_radius + 6);
        }

        QColor fill("#f1f3f5"), border("#ced4da"), text("#868e96");
        if (cur && !m_allDone) { fill = border = QColor("#2196F3"); text = Qt::white; }
        else if (done)         { fill = border = QColor("#4CAF50"); text = Qt::white; }

        p.setPen(QPen(border, 2));
        p.setBrush(fill);
        p.drawEllipse(c, m_radius, m_radius);

        p.setFont(f);
        p.setPen(text);
        p.drawText(r, Qt::AlignCenter,
                   (done && !(cur && !m_allDone)) ? QString::fromUtf8("\u2713") : QString::number(i + 1));

        // label + subtitle
        const int lx = m_leftMargin + m_radius + 16;
        const int lw = width() - lx - 10;
        QFont lf = f;
        lf.setBold(cur && !m_allDone);
        p.setFont(lf);
        p.setPen(cur ? QColor("#1b2733") : QColor("#495057"));
        const bool hasSub = i < m_subs.size() && !m_subs[i].isEmpty();
        QRect labelRect(lx, r.top() - (hasSub ? 8 : 0), lw, r.height() + (hasSub ? -2 : 0));
        p.drawText(labelRect, Qt::AlignLeft | Qt::AlignVCenter, m_labels[i]);

        if (hasSub) {
            QFont sf = font();
            sf.setPixelSize(12);
            p.setFont(sf);
            p.setPen(QColor("#8a949e"));
            p.drawText(QRect(lx, r.center().y() + 3, lw, 20), Qt::AlignLeft | Qt::AlignTop, m_subs[i]);
        }
    }
}

int StepRail::rowAt(const QPoint &pos) const
{
    for (int i = 0; i < m_hitRects.size(); ++i) {
        QRect row(0, m_hitRects[i].top() - 12, width(), m_rowHeight);
        if (row.contains(pos)) return i;
    }
    return -1;
}

void StepRail::mouseMoveEvent(QMouseEvent *event)
{
    const int i = rowAt(event->pos());
    setCursor((i >= 0 && clickable(i)) ? Qt::PointingHandCursor : Qt::ArrowCursor);
}

void StepRail::mousePressEvent(QMouseEvent *event)
{
    const int i = rowAt(event->pos());
    if (i >= 0 && clickable(i)) emit stepClicked(i);
}
