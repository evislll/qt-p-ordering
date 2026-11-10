#ifndef STEPRAIL_H
#define STEPRAIL_H

#include <QWidget>
#include <QStringList>
#include <QVector>

// Vertical progress rail: circles + connecting line + short descriptions.
// Clicking a step that is <= currentStep emits stepClicked(index).
class StepRail : public QWidget
{
    Q_OBJECT
public:
    explicit StepRail(QWidget *parent = nullptr);

    void setSteps(const QStringList &labels);
    void setSubtitles(const QStringList &subs);   // small grey line under each label
    void setCurrentStep(int index);
    void setCompletedUpTo(int index);
    void setAllCompleted(bool on);                // whole rail stays green (order placed)

    QSize sizeHint() const override;

signals:
    void stepClicked(int index);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    int  rowAt(const QPoint &pos) const;
    bool clickable(int i) const { return i <= m_current || m_allDone; }

    QStringList m_labels;
    QStringList m_subs;
    QVector<QRect> m_hitRects;
    int m_current = 0;
    int m_completedUpTo = -1;
    bool m_allDone = false;

    int m_radius = 15;
    int m_rowHeight = 78;
    int m_leftMargin = 46;
    int m_top = 26;
};

#endif // STEPRAIL_H
