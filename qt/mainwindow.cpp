#include "mainwindow.h"
#include "pages.h"
#include "uihelpers.h"

#include <QtWidgets>
#include <QRandomGenerator>

static QString money(double v) { return "$" + QString::number(v, 'f', 2); }

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle("PizzaGo — Order pizza");
    setWindowIcon(ui::appIcon());
    buildUi();
    applyStyle();
    goToPage(0);

    auto *fs = new QShortcut(QKeySequence(Qt::Key_F11), this);
    connect(fs, &QShortcut::activated, this, &MainWindow::toggleFullScreen);
    auto *esc = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(esc, &QShortcut::activated, this, [this]{ if (isFullScreen()) showMaximized(); });
}

MainWindow::~MainWindow() = default;

void MainWindow::toggleFullScreen()
{
    if (isFullScreen()) showMaximized(); else showFullScreen();
}

// Takes the content out of a page and appends it as a section of the single
// scrolling column. The page object stays alive (hidden): it owns signals/timers.
void MainWindow::addSection(QScrollArea *page)
{
    QWidget *inner = page->takeWidget();
    page->hide();
    m_sections->addWidget(inner);
    m_sectionWidgets.append(inner);
}

void MainWindow::buildUi()
{
    auto *central = new QWidget(this);
    auto *h = new QHBoxLayout(central);
    h->setContentsMargins(0, 0, 0, 0);
    h->setSpacing(0);

    // ---- LEFT: brand + step rail + account ------------------------------
    auto *railFrame = new QFrame;
    railFrame->setObjectName("railFrame");
    railFrame->setFixedWidth(268);
    auto *rl = new QVBoxLayout(railFrame);
    rl->setContentsMargins(0, 0, 0, 0);
    rl->setSpacing(0);

    auto *brandRow = new QHBoxLayout;
    brandRow->setContentsMargins(26, 24, 20, 6);
    brandRow->setSpacing(12);
    auto *logo = new QLabel;
    logo->setPixmap(ui::logoPixmap(40));
    auto *brand = new QLabel("PizzaGo");
    brand->setObjectName("brandLabel");
    brandRow->addWidget(logo);
    brandRow->addWidget(brand);
    brandRow->addStretch();
    rl->addLayout(brandRow);

    m_rail = new StepRail;
    m_rail->setFixedWidth(268);
    m_rail->setSteps({"Sign in", "Choose restaurant", "Build your pizza", "Delivery details",
                      "Payment", "Review & confirm", "Order tracking"});
    m_rail->setSubtitles({"Log in or continue as guest", "Pick a spot on the map", "Size, crust & toppings",
                          "Where should we deliver?", "Card, wallet, cash…", "Check & add promo code",
                          "Follow your pizza live"});
    rl->addWidget(m_rail, 1);

    auto *acct = new QFrame;
    acct->setObjectName("accountBox");
    auto *av = new QVBoxLayout(acct);
    av->setContentsMargins(16, 12, 16, 12);
    av->setSpacing(8);
    m_userLabel = new QLabel;
    m_userLabel->setObjectName("userLabel");
    m_authBtn = new QPushButton;
    m_authBtn->setCursor(Qt::PointingHandCursor);
    av->addWidget(m_userLabel);
    av->addWidget(m_authBtn);
    auto *hint = new QLabel("F11 — toggle full screen");
    hint->setObjectName("noteLabel");
    av->addWidget(hint);
    auto *accOuter = new QVBoxLayout;
    accOuter->setContentsMargins(14, 0, 14, 16);
    accOuter->addWidget(acct);
    rl->addLayout(accOuter);
    h->addWidget(railFrame);

    // ---- RIGHT: one scroll area holding all steps ---------------------------
    m_scroll = new QScrollArea;
    m_scroll->setObjectName("mainScroll");
    m_scroll->setWidgetResizable(true);
    m_scroll->setFrameShape(QFrame::NoFrame);
    m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_content  = new QWidget;
    m_content->setObjectName("pageBg");
    m_sections = new QVBoxLayout(m_content);
    m_sections->setContentsMargins(0, 0, 0, 0);
    m_sections->setSpacing(0);
    m_scroll->setWidget(m_content);
    h->addWidget(m_scroll, 1);
    setCentralWidget(central);

    m_signIn     = new SignInPage(this);
    m_restaurant = new RestaurantPage(this);
    m_pizza      = new PizzaPage(this);
    m_delivery   = new DeliveryPage(this);
    m_payment    = new PaymentPage(this);
    m_review     = new ReviewPage(this);
    m_progress   = new ProgressPage(this);
    m_thanks     = new ThankYouPage(this);

    addSection(m_signIn);      // 0
    addSection(m_restaurant);  // 1
    addSection(m_pizza);       // 2
    addSection(m_delivery);    // 3
    addSection(m_payment);     // 4
    addSection(m_review);      // 5
    addSection(m_progress);    // 6
    addSection(m_thanks);      // 7

    for (int i = 1; i < m_sectionWidgets.size(); ++i) m_sectionWidgets[i]->hide();
    m_frontier = 0;

    m_scroll->viewport()->installEventFilter(this);
    updateSectionHeights();

    // wheel over a combo box / spin box should scroll the page, not change the value
    for (auto *cb : m_content->findChildren<QComboBox*>()) { cb->setFocusPolicy(Qt::StrongFocus); cb->installEventFilter(this); }
    for (auto *sp : m_content->findChildren<QAbstractSpinBox*>()) { sp->setFocusPolicy(Qt::StrongFocus); sp->installEventFilter(this); }

    m_scrollAnim = new QPropertyAnimation(m_scroll->verticalScrollBar(), "value", this);
    m_scrollAnim->setDuration(450);
    m_scrollAnim->setEasingCurve(QEasingCurve::InOutCubic);
    connect(m_scrollAnim, &QPropertyAnimation::finished, this, &MainWindow::onScrollFinished);
    connect(m_scroll->verticalScrollBar(), &QScrollBar::valueChanged, this, &MainWindow::onScrolled);

    // forward navigation
    connect(m_signIn,     &SignInPage::nextRequested,     this, [this]{ goToPage(1); });
    connect(m_restaurant, &RestaurantPage::nextRequested, this, [this]{ goToPage(2); });
    connect(m_pizza,      &PizzaPage::nextRequested,      this, [this]{ goToPage(3); });
    connect(m_delivery,   &DeliveryPage::nextRequested,   this, [this]{ goToPage(4); });
    connect(m_payment,    &PaymentPage::nextRequested,    this, [this]{ goToPage(5); refreshReview(); });
    connect(m_review,     &ReviewPage::placeRequested,    this, [this]{
        if (!m_payment->valid()) { goToPage(4); return; }
        m_orderNo = QString("PG-%1").arg(QRandomGenerator::global()->bounded(10000, 99999));
        const Totals t = totals();

        OrderInfo info;
        info.orderNo    = m_orderNo;
        info.restaurant = m_restaurant->selectedRestaurant();
        info.deliverTo  = QString("%1, %2 %3").arg(m_delivery->address(), m_delivery->city(), m_delivery->postalCode());
        info.schedule   = m_delivery->schedule();
        info.itemsHtml  = itemsHtml();
        info.payment    = m_payment->paymentSummary();
        info.total      = t.total;
        info.etaMinutes = m_restaurant->etaMinutes();

        setOrderPlaced(true);
        goToPage(6);
        m_progress->start(info);
    });
    connect(m_review,   &ReviewPage::promoEntered,     this, &MainWindow::applyPromo);
    connect(m_progress, &ProgressPage::cancelRequested,   this, &MainWindow::cancelOrder);
    connect(m_progress, &ProgressPage::newOrderRequested, this, &MainWindow::startNewOrder);

    // backwards
    connect(m_pizza,    &PizzaPage::backRequested,    this, [this]{ goToPage(1); });
    connect(m_delivery, &DeliveryPage::backRequested, this, [this]{ goToPage(2); });
    connect(m_payment,  &PaymentPage::backRequested,  this, [this]{ goToPage(3); });
    connect(m_review,   &ReviewPage::backRequested,   this, [this]{ goToPage(4); });

    connect(m_rail, &StepRail::stepClicked, this, &MainWindow::onStepClicked);

    // account
    connect(m_signIn, &SignInPage::accountChanged, this, &MainWindow::onAccountChanged);
    connect(m_authBtn, &QPushButton::clicked, this, [this]{
        if (m_signIn->isGuest()) goToPage(0); else m_signIn->signOut();
    });
    onAccountChanged("Guest", true);
}

void MainWindow::onAccountChanged(const QString &name, bool guest)
{
    if (guest) {
        m_userLabel->setText(QString::fromUtf8("\U0001F464  Guest"));
        m_authBtn->setText("Log in / Sign up");
    } else {
        m_userLabel->setText(QString::fromUtf8("\U0001F464  ") + name);
        m_authBtn->setText("Sign out");
        m_delivery->prefillName(name);
    }
}

void MainWindow::updateSectionHeights()
{
    const int h = m_scroll->viewport()->height();
    for (auto *w : m_sectionWidgets) w->setMinimumHeight(h);
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (m_scroll && obj == m_scroll->viewport() && event->type() == QEvent::Resize) {
        updateSectionHeights();
        QTimer::singleShot(0, this, [this]{
            if (m_scrollAnim->state() == QAbstractAnimation::Running) return;
            if (m_viewIndex >= 0 && m_viewIndex < m_sectionWidgets.size())
                m_scroll->verticalScrollBar()->setValue(m_sectionWidgets[m_viewIndex]->y());
        });
    }
    else if (event->type() == QEvent::Wheel) {
        if (auto *w = qobject_cast<QWidget*>(obj)) {
            if ((qobject_cast<QComboBox*>(obj) || qobject_cast<QAbstractSpinBox*>(obj)) && !w->hasFocus()) {
                event->ignore();
                return true;
            }
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::applyStyle()
{
    setStyleSheet(R"(
        /* ---------- Global ---------- */
        QWidget { font-family: 'Segoe UI', 'Helvetica Neue', 'Noto Sans', sans-serif; font-size: 14px; color: #333333; }
        QWidget#pageBg, QScrollArea#mainScroll { background: #f4f6f8; }
        QScrollArea { background: transparent; border: none; }
        QLabel { background: transparent; }

        /* ---------- Left rail ---------- */
        QFrame#railFrame { background: #ffffff; border-right: 1px solid #e6e9ec; }
        QLabel#brandLabel { font-size: 22px; font-weight: 800; color: #1976D2; }
        QFrame#accountBox { background: #f4f6f8; border: 1px solid #e6e9ec; border-radius: 12px; }
        QLabel#userLabel { font-weight: 700; color: #1b2733; }

        /* ---------- Headings ---------- */
        QLabel#pageTitle    { font-size: 30px; font-weight: 800; color: #1b2733; }
        QLabel#pageSubtitle { font-size: 15px; color: #5f6b76; }
        QLabel#cardTitle    { font-size: 16px; font-weight: 700; color: #1b2733; }
        QLabel#fieldLabel   { font-size: 13px; font-weight: 600; color: #495057; }
        QLabel#noteLabel    { color: #6c757d; font-size: 13px; }
        QLabel#errorLabel   { color: #c62828; font-weight: 600; background: #fdecea; border-radius: 8px; padding: 10px 12px; }
        QLabel#infoBox      { background: #eef6fe; border: 1px solid #cfe5fb; border-radius: 10px; padding: 14px; color: #1b4a72; }
        QLabel#etaBig       { font-size: 32px; font-weight: 800; color: #1b2733; }
        QLabel#brandChip    { background: #e3f2fd; color: #1565C0; font-weight: 700; border-radius: 8px; padding: 6px 12px; }

        /* ---------- Cards ---------- */
        QFrame#card { background: #ffffff; border: 1px solid #e3e7eb; border-radius: 16px; }
        QFrame#card QLabel { background: transparent; }
        QFrame#hero {
            border-radius: 20px; border: none;
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #42A5F5, stop:1 #1565C0);
        }
        QLabel#heroBrand { color: white; font-size: 34px; font-weight: 800; }
        QLabel#heroTag   { color: #e3f2fd; font-size: 17px; }
        QLabel#heroPerk  { color: white; font-size: 14px; padding: 3px 0; }

        /* ---------- Inputs ---------- */
        QLineEdit, QComboBox, QSpinBox {
            padding: 10px 12px; border: 1px solid #d1d5da; border-radius: 10px;
            background: #ffffff; min-height: 22px; selection-background-color: #2196F3;
        }
        QLineEdit:focus, QComboBox:focus, QSpinBox:focus { border: 2px solid #2196F3; background: #f8fbff; padding: 9px 11px; }
        QLineEdit[invalid="true"] { border: 2px solid #c62828; }
        QComboBox QAbstractItemView {
            background: #ffffff; border: 1px solid #d1d5da; selection-background-color: #2196F3;
            selection-color: #ffffff; outline: 0px; padding: 4px;
        }
        QComboBox::drop-down { border: none; width: 34px; subcontrol-origin: padding; subcontrol-position: center right; }
        QComboBox::down-arrow {
            image: none; width: 0px; height: 0px; margin-right: 12px;
            border-left: 5px solid transparent; border-right: 5px solid transparent; border-top: 6px solid #607d8b;
        }

        /* ---------- Buttons ---------- */
        QPushButton {
            padding: 10px 20px; border-radius: 10px; border: 1px solid #d1d5da;
            background: #ffffff; font-weight: 600; color: #333;
        }
        QPushButton:hover { background: #f1f3f5; border-color: #b0b8c1; }
        QPushButton:disabled { color: #aab2ba; background: #f5f6f7; }
        QPushButton#primaryButton { background: #4CAF50; color: white; border: none; font-weight: 700; padding: 13px 28px; font-size: 15px; }
        QPushButton#primaryButton:hover { background: #43a047; }
        QPushButton#ghostButton { border: 1px solid #cfd6dd; background: #ffffff; }
        QPushButton#ghostButton:hover { border-color: #2196F3; color: #1976D2; background: #f3f9ff; }
        QPushButton#linkButton { border: none; background: transparent; color: #1976D2; padding: 2px; font-weight: 600; }
        QPushButton#linkButton:hover { text-decoration: underline; background: transparent; }
        QPushButton#dangerButton { color: #c62828; border: 1px solid #e57373; }
        QPushButton#dangerButton:hover { background: #fdecea; border-color: #c62828; }
        QPushButton#dangerButton:disabled { color: #c9a3a3; border-color: #ecd0d0; background: #faf6f6; }
        QPushButton#starBtn { border: none; background: transparent; font-size: 28px; color: #f5a623; padding: 0 4px; }
        QPushButton#starBtn:hover { background: transparent; }
        QPushButton#mapBtn { padding: 0; border-radius: 8px; font-size: 18px; font-weight: 700; background: white; border: 1px solid #cfd6dd; }
        QPushButton#mapBtn:hover { background: #f3f9ff; }

        /* segmented pickers (size / crust / tip / sign-in tabs) */
        QPushButton#segBtn { border-radius: 0; margin-right: -1px; padding: 10px 18px; font-weight: 600; }
        QPushButton#segBtn:checked { background: #2196F3; color: white; border-color: #2196F3; }
        QPushButton#segBtn:checked:hover { background: #1e88e5; }

        /* preset pizza cards + payment methods */
        QPushButton#presetCard { text-align: left; padding: 14px 18px; border: 2px solid #e3e7eb; border-radius: 14px; background: #fbfcfd; font-weight: 500; }
        QPushButton#presetCard:hover { border-color: #90caf9; background: #f3f9ff; }
        QPushButton#presetCard:checked { border-color: #2196F3; background: #e8f3fe; color: #0d47a1; }
        QPushButton#payMethod { font-size: 15px; border: 2px solid #e3e7eb; border-radius: 14px; background: #fbfcfd; }
        QPushButton#payMethod:hover { border-color: #90caf9; background: #f3f9ff; }
        QPushButton#payMethod:checked { border-color: #2196F3; background: #e8f3fe; color: #0d47a1; }

        /* ---------- Restaurant list ---------- */
        QListWidget#restList { background: transparent; border: none; outline: 0; }
        QListWidget#restList::item {
            background: #ffffff; border: 1px solid #e3e7eb; border-radius: 12px; padding: 8px 10px; margin-bottom: 8px; color: #333;
        }
        QListWidget#restList::item:hover { border-color: #90caf9; }
        QListWidget#restList::item:selected { border: 2px solid #2196F3; background: #f3f9ff; color: #1b2733; }

        /* ---------- Checkboxes / progress ---------- */
        QCheckBox { padding: 6px 2px; spacing: 8px; }
        QCheckBox::indicator { width: 20px; height: 20px; border: 2px solid #b8c0c8; border-radius: 6px; background: white; }
        QCheckBox::indicator:hover { border-color: #2196F3; }
        QCheckBox::indicator:checked { background: #4CAF50; border-color: #4CAF50; }
        QProgressBar { border: none; border-radius: 5px; background: #e6eaee; }
        QProgressBar::chunk { border-radius: 5px; background: qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 #2196F3, stop:1 #4CAF50); }

        /* ---------- Scrollbars ---------- */
        QScrollBar:vertical { border: none; background: transparent; width: 10px; margin: 2px; }
        QScrollBar::handle:vertical { background: #c7cdd3; min-height: 30px; border-radius: 5px; }
        QScrollBar::handle:vertical:hover { background: #a8b0b8; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }
    )");
}

// Unlocks sections up to `index` (if needed) and scrolls to it. Going backwards
// re-locks every section below the target once the scroll has finished.
void MainWindow::goToPage(int index)
{
    if (index < 0 || index >= m_sectionWidgets.size()) return;

    // Leaving an order that is still in progress: confirm cancelling
    if (m_orderPlaced && index < 6) {
        if (m_progress->isActive()) {
            auto reply = QMessageBox::question(this, "Cancel order?",
                "Are you sure you want to cancel the order?", QMessageBox::Yes | QMessageBox::No);
            if (reply != QMessageBox::Yes) return;
        }
        m_progress->stop();
        setOrderPlaced(false);
    }

    m_viewIndex   = index;
    m_target      = index;
    m_currentStep = qMin(index, 6);
    m_rail->setCurrentStep(m_currentStep);

    while (m_frontier < index) m_sectionWidgets[++m_frontier]->show();

    QTimer::singleShot(0, this, [this, index]{
        if (index == m_target) scrollToSection(index);
    });
}

void MainWindow::setOrderPlaced(bool placed)
{
    m_orderPlaced = placed;
    m_rail->setAllCompleted(placed);
}

void MainWindow::cancelOrder()
{
    auto reply = QMessageBox::question(this, "Cancel order?",
        "Cancel your order? You won't be charged, and your pizza, delivery and payment details will be cleared.",
        QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes) return;
    startNewOrder();
}

void MainWindow::startNewOrder()
{
    m_progress->stop();
    setOrderPlaced(false);
    m_pizza->resetPizza();
    m_delivery->clear();
    if (!m_signIn->isGuest()) m_delivery->prefillName(m_signIn->displayName());
    m_payment->clear();
    m_promo.clear();
    m_review->clearPromo();
    m_review->setSummary(QString());
    goToPage(1);
}

void MainWindow::scrollToSection(int index)
{
    if (index < 0 || index >= m_sectionWidgets.size()) return;
    QScrollBar *bar = m_scroll->verticalScrollBar();
    const int target = m_sectionWidgets[index]->y();

    m_scrollAnim->stop();
    if (bar->value() == target) { onScrollFinished(); return; }
    m_scrollAnim->setStartValue(bar->value());
    m_scrollAnim->setEndValue(target);
    m_scrollAnim->start();
}

void MainWindow::onScrollFinished()
{
    while (m_frontier > m_target) m_sectionWidgets[m_frontier--]->hide();
    if (m_target >= 0 && m_target < m_sectionWidgets.size())
        m_scroll->verticalScrollBar()->setValue(m_sectionWidgets[m_target]->y());
}

void MainWindow::onScrolled()
{
    if (m_scrollAnim->state() == QAbstractAnimation::Running) return;
    const int probe = m_scroll->verticalScrollBar()->value() + m_scroll->viewport()->height() / 2;
    int idx = 0;
    for (int i = 0; i <= m_frontier && i < m_sectionWidgets.size(); ++i)
        if (m_sectionWidgets[i]->y() <= probe) idx = i;
    if (idx == m_viewIndex) return;
    m_viewIndex   = idx;
    m_currentStep = qMin(idx, 6);
    m_rail->setCurrentStep(m_currentStep);
    if (idx == 5) refreshReview();
}

void MainWindow::onStepClicked(int index)
{
    if (index < 0 || index > m_frontier) return;
    goToPage(index);
}

// ---------------------------------------------------------------------
// pricing + summary
// ---------------------------------------------------------------------
MainWindow::Totals MainWindow::totals() const
{
    Totals t;
    t.subtotal = m_pizza->subtotal();
    t.delivery = m_restaurant->deliveryFee();
    if (m_promo == "PIZZA10")      t.discount = t.subtotal * 0.10;
    else if (m_promo == "SAVE5")   t.discount = qMin(5.0, t.subtotal);
    else if (m_promo == "FREEDEL") t.delivery = 0.0;
    t.tax = (t.subtotal - t.discount + t.delivery) * 0.13;                 // Ontario HST
    t.tip = (t.subtotal - t.discount) * m_payment->tipPercent() / 100.0;
    t.total = t.subtotal - t.discount + t.delivery + t.tax + t.tip;
    return t;
}

void MainWindow::applyPromo(const QString &codeIn)
{
    const QString code = codeIn.trimmed().toUpper();
    if (code.isEmpty()) { m_promo.clear(); m_review->setPromoStatus(QString(), true); refreshReview(); return; }
    static const QHash<QString, QString> codes = {
        {"PIZZA10", "10% off your pizza"}, {"SAVE5", "$5 off your pizza"}, {"FREEDEL", "free delivery"}};
    if (codes.contains(code)) {
        m_promo = code;
        m_review->setPromoStatus(QString::fromUtf8("\u2714 ") + code + " applied — " + codes[code], true);
    } else {
        m_promo.clear();
        m_review->setPromoStatus("Sorry, that code isn't valid.", false);
    }
    refreshReview();
}

void MainWindow::refreshReview() { m_review->setSummary(composedOrderHtml()); }

QString MainWindow::itemsHtml() const
{
    const auto tops = m_pizza->toppings();
    QString h = QString("<b>%1 × %2</b> — %3, %4 crust<br>")
                    .arg(m_pizza->quantity()).arg(m_pizza->pizzaName(), m_pizza->size(), m_pizza->crust());
    h += QString("Toppings: %1").arg(tops.isEmpty() ? "none" : tops.join(", "));
    if (m_pizza->extraCheese()) h += " + extra cheese";
    if (!m_pizza->notes().isEmpty()) h += QString("<br><i>“%1”</i>").arg(m_pizza->notes().toHtmlEscaped());
    return h;
}

QString MainWindow::composedOrderHtml() const
{
    const Totals t = totals();
    auto row = [](const QString &l, const QString &r, const QString &style = QString()) {
        return QString("<tr><td style='padding:3px 0;%3'>%1</td><td align='right' style='%3'>%2</td></tr>").arg(l, r, style);
    };
    QString h;
    h += QString("<b>Restaurant:</b> %1<br>").arg(m_restaurant->selectedRestaurant());
    h += "<hr style='border:1px solid #eee'>";
    h += itemsHtml();
    h += "<hr style='border:1px solid #eee'>";
    h += QString("<b>Recipient:</b> %1 &nbsp; (%2)<br>").arg(m_delivery->name(), m_delivery->phone());
    h += QString("<b>Address:</b> %1, %2 %3<br>").arg(m_delivery->address(), m_delivery->city(), m_delivery->postalCode());
    h += QString("<b>When:</b> %1<br>").arg(m_delivery->schedule());
    if (!m_delivery->instructions().isEmpty())
        h += QString("<b>Notes:</b> %1<br>").arg(m_delivery->instructions().toHtmlEscaped());
    h += QString("<b>Payment:</b> %1<br>").arg(m_payment->paymentSummary());
    h += "<hr style='border:1px solid #eee'><table width='100%' cellspacing='0'>";
    h += row("Subtotal", money(t.subtotal));
    if (t.discount > 0) h += row("Discount (" + m_promo + ")", "−" + money(t.discount), "color:#2e7d32");
    h += row("Delivery fee" + QString(m_promo == "FREEDEL" ? " (FREEDEL)" : ""), t.delivery == 0 ? "Free" : money(t.delivery));
    h += row("HST (13%)", money(t.tax));
    if (t.tip > 0) h += row(QString("Driver tip (%1%)").arg(m_payment->tipPercent()), money(t.tip));
    h += row("Total", money(t.total), "font-size:17px;font-weight:bold;color:#2e7d32");
    h += "</table>";
    return h;
}
