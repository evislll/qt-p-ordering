#ifndef PAGES_H
#define PAGES_H

#include <QScrollArea>
#include <QList>
#include <QString>
#include <QStringList>
#include "mapwidget.h"

class QLineEdit;
class QComboBox;
class QCheckBox;
class QLabel;
class QPushButton;
class QButtonGroup;
class QStackedWidget;
class QListWidget;
class QSpinBox;
class QProgressBar;
class QTimer;

// Every page is a QScrollArea so content can always scroll vertically.
// Pages emit signals; MainWindow wires them together.

// ---------- everything the tracking page needs to know ----------
struct OrderInfo {
    QString orderNo;
    QString restaurant;
    QString deliverTo;
    QString schedule;
    QString itemsHtml;
    QString payment;
    double  total = 0;
    int     etaMinutes = 35;
};

// ---------- 0. Sign in / Sign up / Guest ----------
class SignInPage : public QScrollArea {
    Q_OBJECT
public:
    explicit SignInPage(QWidget *parent = nullptr);
    QString displayName() const { return m_name; }
    bool    isGuest() const     { return m_guest; }
    void    signOut();
signals:
    void nextRequested();
    void accountChanged(const QString &displayName, bool guest);
private:
    void doLogin();
    void doRegister();
    void showMode(int mode);
    void loginAs(const QString &name);
    void updateStrength(const QString &pw);

    QStackedWidget *m_forms;
    QPushButton *m_tabLogin, *m_tabReg;
    QLineEdit *m_loginEmail, *m_loginPass, *m_regName, *m_regEmail, *m_regPass, *m_regPass2;
    QLabel *m_loginErr, *m_regErr, *m_strength;
    QCheckBox *m_remember, *m_showPass, *m_terms;
    QString m_name = "Guest";
    bool m_guest = true;
};

// ---------- 1. Restaurant (map + list) ----------
class RestaurantPage : public QScrollArea {
    Q_OBJECT
public:
    explicit RestaurantPage(QWidget *parent = nullptr);
    QString selectedRestaurant() const;
    QString selectedAddress() const;
    double  deliveryFee() const;
    int     etaMinutes() const;
signals:
    void nextRequested();
private:
    void updateDetail(int row);
    void applyFilter(const QString &text);

    QList<Restaurant> m_items;
    QListWidget *m_list;
    MapWidget   *m_map;
    QLabel      *m_detail;
};

// ---------- 2. Pizza Builder ----------
class PizzaPage : public QScrollArea {
    Q_OBJECT
public:
    explicit PizzaPage(QWidget *parent = nullptr);

    QString     pizzaName() const;
    QString     size() const;
    QString     crust() const;
    QStringList toppings() const;
    bool        extraCheese() const;
    int         quantity() const;
    QString     notes() const;
    double      subtotal() const;
    void        resetPizza();

signals:
    void backRequested();
    void nextRequested();
    void summaryChanged();

private:
    void applyPreset(int index);
    void userChangedRecipe();
    void refreshSummary();
    double unitPrice() const;

    QList<QPushButton*> m_presetBtns;
    QButtonGroup *m_sizeGroup, *m_crustGroup;
    QList<QCheckBox*> m_tops;
    QCheckBox   *m_extraCheese;
    QSpinBox    *m_qty;
    QLineEdit   *m_notes;
    QLabel      *m_summary;
    int  m_preset = 0;
    bool m_applying = false;
};

// ---------- 3. Delivery Details ----------
class DeliveryPage : public QScrollArea {
    Q_OBJECT
public:
    explicit DeliveryPage(QWidget *parent = nullptr);
    void clear();
    void prefillName(const QString &name);
    bool valid() const;

    QString name() const;
    QString address() const;      // street (+ apt)
    QString city() const;
    QString postalCode() const;
    QString phone() const;
    QString schedule() const;
    QString instructions() const;

signals:
    void nextRequested();
    void backRequested();

private:
    QLineEdit *m_name, *m_phone, *m_street, *m_apt, *m_city, *m_postal, *m_instr;
    QComboBox *m_when;
    QCheckBox *m_contactless;
    QLabel    *m_error;
};

// ---------- 4. Payment ----------
class PaymentPage : public QScrollArea {
    Q_OBJECT
public:
    explicit PaymentPage(QWidget *parent = nullptr);
    void clear();
    bool    valid() const;
    QString maskedCard() const;
    QString paymentSummary() const;
    int     tipPercent() const;

signals:
    void nextRequested();
    void backRequested();

private:
    void formatCard(const QString &text);
    void formatExpiry(const QString &text);
    void selectMethod(int m);

    int m_method = 0;
    QList<QPushButton*> m_methodBtns;
    QStackedWidget *m_stack;
    QLabel *m_walletInfo, *m_brand, *m_error;
    QLineEdit *m_holder, *m_card, *m_expiry, *m_cvv, *m_paypal, *m_gift, *m_change;
    QCheckBox *m_save;
    QButtonGroup *m_tipGroup;
};

// ---------- 5. Review ----------
class ReviewPage : public QScrollArea {
    Q_OBJECT
public:
    explicit ReviewPage(QWidget *parent = nullptr);
    void setSummary(const QString &html);
    void setPromoStatus(const QString &text, bool ok);
    void clearPromo();
signals:
    void placeRequested();
    void backRequested();
    void promoEntered(const QString &code);
private:
    QLabel    *m_body, *m_promoStatus;
    QLineEdit *m_promo;
};

// ---------- 6. Order tracking ----------
class ProgressPage : public QScrollArea {
    Q_OBJECT
public:
    explicit ProgressPage(QWidget *parent = nullptr);
    void start(const OrderInfo &info);
    void stop();
    bool isActive() const { return m_active && !m_done; }
signals:
    void cancelRequested();
    void newOrderRequested();
private:
    void tick();
    void render();

    QLabel *m_headline, *m_sub, *m_eta, *m_status, *m_driver, *m_details, *m_cancelNote;
    QProgressBar *m_bar;
    QList<QLabel*> m_icons, m_texts, m_times;
    TrackMap *m_track;
    QPushButton *m_cancel, *m_contact, *m_newOrder;
    QWidget *m_rateBox;
    QList<QPushButton*> m_stars;
    QTimer *m_timer;
    OrderInfo m_info;
    qint64 m_elapsed = 0;
    int  m_reached = -1;
    bool m_active = false, m_done = false;
};

// ---------- 7. Thank-you ----------
class ThankYouPage : public QScrollArea {
    Q_OBJECT
public:
    explicit ThankYouPage(QWidget *parent = nullptr);
};

#endif // PAGES_H
