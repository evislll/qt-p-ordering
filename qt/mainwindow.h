#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QList>
#include "steprail.h"

class QScrollArea;
class QVBoxLayout;
class QPropertyAnimation;
class QEvent;
class QLabel;
class QPushButton;

class SignInPage;
class RestaurantPage;
class PizzaPage;
class DeliveryPage;
class PaymentPage;
class ReviewPage;
class ProgressPage;
class ThankYouPage;

// Single vertically-scrolling page: every wizard step is a "section" stacked
// in one QScrollArea. The step rail stays fixed on the left and works as a
// table of contents. Sections below the current step stay hidden (locked)
// until the user reaches them with the Continue buttons.
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void goToPage(int index);
    void onStepClicked(int index);
    void refreshReview();
    void onScrolled();
    void onScrollFinished();
    void cancelOrder();
    void startNewOrder();
    void applyPromo(const QString &code);
    void onAccountChanged(const QString &name, bool guest);
    void toggleFullScreen();

private:
    struct Totals { double subtotal = 0, discount = 0, delivery = 0, tax = 0, tip = 0, total = 0; };

    void buildUi();
    void applyStyle();
    void addSection(QScrollArea *page);
    void updateSectionHeights();
    void scrollToSection(int index);
    void setOrderPlaced(bool placed);
    Totals totals() const;
    QString itemsHtml() const;
    QString composedOrderHtml() const;

    StepRail *m_rail = nullptr;
    QLabel      *m_userLabel = nullptr;
    QPushButton *m_authBtn   = nullptr;

    QScrollArea        *m_scroll   = nullptr;
    QWidget            *m_content  = nullptr;
    QVBoxLayout        *m_sections = nullptr;
    QList<QWidget*>     m_sectionWidgets;
    QPropertyAnimation *m_scrollAnim = nullptr;

    SignInPage      *m_signIn      = nullptr;
    RestaurantPage  *m_restaurant  = nullptr;
    PizzaPage       *m_pizza       = nullptr;
    DeliveryPage    *m_delivery    = nullptr;
    PaymentPage     *m_payment     = nullptr;
    ReviewPage      *m_review      = nullptr;
    ProgressPage    *m_progress    = nullptr;
    ThankYouPage    *m_thanks      = nullptr;

    int  m_currentStep = 0;
    int  m_viewIndex   = 0;
    int  m_frontier    = 0;
    int  m_target      = 0;
    bool m_orderPlaced = false;
    QString m_promo;
    QString m_orderNo;
};

#endif // MAINWINDOW_H
