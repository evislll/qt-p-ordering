#include "uihelpers.h"

#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QScrollArea>

namespace ui {

QPixmap logoPixmap(int s)
{
    QPixmap pm(s, s);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);

    QLinearGradient g(0, 0, s, s);
    g.setColorAt(0, QColor("#42A5F5"));
    g.setColorAt(1, QColor("#1976D2"));
    p.setPen(Qt::NoPen);
    p.setBrush(g);
    p.drawRoundedRect(QRectF(0, 0, s, s), s * 0.26, s * 0.26);

    QPainterPath slice;                      // cheese triangle
    slice.moveTo(s * 0.22, s * 0.32);
    slice.lineTo(s * 0.78, s * 0.32);
    slice.lineTo(s * 0.50, s * 0.84);
    slice.closeSubpath();
    p.setBrush(QColor("#FFD54F"));
    p.drawPath(slice);

    p.setBrush(QColor("#E6A23C"));           // crust
    p.drawRoundedRect(QRectF(s * 0.18, s * 0.22, s * 0.64, s * 0.13), s * 0.065, s * 0.065);

    p.setBrush(QColor("#E53935"));           // pepperoni
    p.drawEllipse(QPointF(s * 0.41, s * 0.46), s * 0.055, s * 0.055);
    p.drawEllipse(QPointF(s * 0.59, s * 0.46), s * 0.055, s * 0.055);
    p.drawEllipse(QPointF(s * 0.50, s * 0.62), s * 0.05,  s * 0.05);
    return pm;
}

QIcon appIcon() { return QIcon(logoPixmap(256)); }

QFrame *card(const QString &title, QVBoxLayout **body)
{
    auto *f = new QFrame;
    f->setObjectName("card");
    auto *v = new QVBoxLayout(f);
    v->setContentsMargins(26, 22, 26, 24);
    v->setSpacing(14);
    if (!title.isEmpty()) {
        auto *t = new QLabel(title);
        t->setObjectName("cardTitle");
        v->addWidget(t);
    }
    if (body) *body = v;
    return f;
}

QWidget *pageHeader(const QString &title, const QString &subtitle)
{
    auto *w = new QWidget;
    auto *v = new QVBoxLayout(w);
    v->setContentsMargins(0, 0, 0, 4);
    v->setSpacing(4);
    auto *t = new QLabel(title);
    t->setObjectName("pageTitle");
    v->addWidget(t);
    if (!subtitle.isEmpty()) {
        auto *s = new QLabel(subtitle);
        s->setObjectName("pageSubtitle");
        s->setWordWrap(true);
        v->addWidget(s);
    }
    return w;
}

QWidget *centered(QWidget *content, int maxWidth)
{
    content->setMaximumWidth(maxWidth);
    auto *wrap = new QWidget;
    auto *h = new QHBoxLayout(wrap);
    h->setContentsMargins(0, 0, 0, 0);
    h->setSpacing(0);
    h->addStretch(1);
    h->addWidget(content, 100);
    h->addStretch(1);
    return wrap;
}

QVBoxLayout *beginPage(QScrollArea *page)
{
    page->setWidgetResizable(true);
    page->setFrameShape(QFrame::NoFrame);
    auto *inner = new QWidget;
    auto *v = new QVBoxLayout(inner);
    v->setContentsMargins(44, 40, 44, 44);
    v->setSpacing(22);
    page->setWidget(centered(inner));
    return v;
}

} // namespace ui
