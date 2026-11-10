#include "pages.h"
#include "uihelpers.h"

#include <QtWidgets>
#include <QCryptographicHash>
#include <QSettings>
#include <QRegularExpression>
#include <QUuid>
#include <QDateTime>
#include <QRandomGenerator>

// ------------------------------------------------------------------
// small helpers
// ------------------------------------------------------------------
static QPushButton *button(const QString &text, const char *objectName = nullptr)
{
    auto *b = new QPushButton(QString(text).replace("&", "&&"));
    if (objectName) b->setObjectName(objectName);
    b->setCursor(Qt::PointingHandCursor);
    return b;
}

static QPushButton *segment(const QString &text)
{
    auto *b = button(text, "segBtn");
    b->setCheckable(true);
    return b;
}

// label above a field
static QWidget *labeled(const QString &text, QWidget *field)
{
    auto *w = new QWidget;
    auto *v = new QVBoxLayout(w);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(6);
    auto *l = new QLabel(text);
    l->setObjectName("fieldLabel");
    v->addWidget(l);
    v->addWidget(field);
    w->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    return w;
}

static QLabel *errorLabel()
{
    auto *l = new QLabel;
    l->setObjectName("errorLabel");
    l->setWordWrap(true);
    l->hide();
    return l;
}

static void showError(QLabel *l, const QString &text)
{
    l->setText(text);
    l->setVisible(!text.isEmpty());
}

static QString money(double v) { return "$" + QString::number(v, 'f', 2); }

static bool validEmail(const QString &e)
{
    static const QRegularExpression re(R"(^[A-Z0-9._%+-]+@[A-Z0-9.-]+\.[A-Z]{2,}$)",
                                       QRegularExpression::CaseInsensitiveOption);
    return re.match(e).hasMatch();
}

static QString hashPw(const QString &salt, const QString &pw)
{
    return QString::fromLatin1(
        QCryptographicHash::hash((salt + pw).toUtf8(), QCryptographicHash::Sha256).toHex());
}

static QString accountKey(const QString &email)
{
    return "accounts/" + QString::fromLatin1(
        QCryptographicHash::hash(email.toLower().toUtf8(), QCryptographicHash::Md5).toHex());
}

// ============================================================
// 0. Sign in / Create account / Guest
// ============================================================
SignInPage::SignInPage(QWidget *parent) : QScrollArea(parent)
{
    auto *v = ui::beginPage(this);

    auto *row = new QHBoxLayout;
    row->setSpacing(28);

    // ---- brand panel -------------------------------------------------
    auto *hero = new QFrame;
    hero->setObjectName("hero");
    hero->setMinimumWidth(290);
    hero->setMaximumWidth(360);
    auto *hv = new QVBoxLayout(hero);
    hv->setContentsMargins(34, 36, 34, 36);
    hv->setSpacing(14);
    auto *logo = new QLabel;
    logo->setPixmap(ui::logoPixmap(72));
    hv->addWidget(logo);
    auto *brand = new QLabel("PizzaGo");
    brand->setObjectName("heroBrand");
    hv->addWidget(brand);
    auto *tag = new QLabel("Hot, fresh pizza — tracked all the way to your door.");
    tag->setObjectName("heroTag");
    tag->setWordWrap(true);
    hv->addWidget(tag);
    hv->addSpacing(14);
    for (const QString &perk : {QString::fromUtf8("\U0001F4CD  Find the closest restaurant on the map"),
                                QString::fromUtf8("\U0001F355  Build your pizza your way"),
                                QString::fromUtf8("\U0001F4B3  Card, wallets, PayPal or cash"),
                                QString::fromUtf8("\U0001F6F5  Live order tracking")}) {
        auto *l = new QLabel(perk);
        l->setObjectName("heroPerk");
        l->setWordWrap(true);
        hv->addWidget(l);
    }
    hv->addStretch();
    row->addWidget(hero);

    // ---- form column -------------------------------------------------
    auto *right = new QVBoxLayout;
    right->setSpacing(16);
    right->addWidget(ui::pageHeader(QString::fromUtf8("\U0001F44B  Welcome!"),
                                    "Log in or create an account to save your details — or just continue as a guest."));

    // tabs
    auto *tabs = new QHBoxLayout;
    tabs->setSpacing(0);
    m_tabLogin = segment("Log in");
    m_tabReg   = segment("Create account");
    m_tabLogin->setChecked(true);
    tabs->addWidget(m_tabLogin);
    tabs->addWidget(m_tabReg);
    tabs->addStretch();
    right->addLayout(tabs);

    QVBoxLayout *body = nullptr;
    auto *formCard = ui::card(QString(), &body);
    m_forms = new QStackedWidget;

    // log in form
    auto *lw = new QWidget;
    auto *lv = new QVBoxLayout(lw);
    lv->setContentsMargins(0, 0, 0, 0);
    lv->setSpacing(14);
    m_loginEmail = new QLineEdit; m_loginEmail->setPlaceholderText("you@example.com");
    m_loginPass  = new QLineEdit; m_loginPass->setEchoMode(QLineEdit::Password);
    m_loginPass->setPlaceholderText("Your password");
    lv->addWidget(labeled("Email", m_loginEmail));
    lv->addWidget(labeled("Password", m_loginPass));
    auto *opts = new QHBoxLayout;
    m_remember = new QCheckBox("Remember me");
    auto *forgot = button("Forgot password?", "linkButton");
    opts->addWidget(m_remember);
    opts->addStretch();
    opts->addWidget(forgot);
    lv->addLayout(opts);
    m_loginErr = errorLabel();
    lv->addWidget(m_loginErr);
    auto *loginBtn = button(QString::fromUtf8("Log in \u2192"), "primaryButton");
    lv->addWidget(loginBtn);
    m_forms->addWidget(lw);

    // sign up form
    auto *rw = new QWidget;
    auto *rv = new QVBoxLayout(rw);
    rv->setContentsMargins(0, 0, 0, 0);
    rv->setSpacing(14);
    m_regName  = new QLineEdit; m_regName->setPlaceholderText("Your full name");
    m_regEmail = new QLineEdit; m_regEmail->setPlaceholderText("you@example.com");
    m_regPass  = new QLineEdit; m_regPass->setEchoMode(QLineEdit::Password);
    m_regPass->setPlaceholderText("At least 8 characters");
    m_regPass2 = new QLineEdit; m_regPass2->setEchoMode(QLineEdit::Password);
    m_regPass2->setPlaceholderText("Repeat your password");
    m_strength = new QLabel;
    m_strength->setObjectName("noteLabel");
    m_terms = new QCheckBox("I agree to the Terms && Privacy Policy");
    m_regErr = errorLabel();
    rv->addWidget(labeled("Full name", m_regName));
    rv->addWidget(labeled("Email", m_regEmail));
    rv->addWidget(labeled("Password", m_regPass));
    rv->addWidget(m_strength);
    rv->addWidget(labeled("Confirm password", m_regPass2));
    rv->addWidget(m_terms);
    rv->addWidget(m_regErr);
    auto *regBtn = button(QString::fromUtf8("Create account \u2192"), "primaryButton");
    rv->addWidget(regBtn);
    m_forms->addWidget(rw);

    body->addWidget(m_forms);
    m_showPass = new QCheckBox("Show passwords");
    body->addWidget(m_showPass);

    // social + guest
    auto *or1 = new QLabel("— or continue with —");
    or1->setObjectName("noteLabel");
    or1->setAlignment(Qt::AlignCenter);
    body->addWidget(or1);
    auto *social = new QHBoxLayout;
    auto *google = button("G   Google", "ghostButton");
    auto *apple  = button(QString::fromUtf8("\U0001F34E  Apple"), "ghostButton");
    social->addWidget(google);
    social->addWidget(apple);
    body->addLayout(social);
    right->addWidget(formCard);

    auto *guestRow = new QHBoxLayout;
    auto *guestNote = new QLabel("In a hurry? You don't need an account to order.");
    guestNote->setObjectName("noteLabel");
    auto *guest = button(QString::fromUtf8("Continue as guest \u2192"), "ghostButton");
    guestRow->addWidget(guestNote);
    guestRow->addStretch();
    guestRow->addWidget(guest);
    right->addLayout(guestRow);
    right->addStretch();

    row->addLayout(right, 1);
    v->addLayout(row);
    v->addStretch();

    // ---- behaviour ---------------------------------------------------
    QSettings s;
    m_loginEmail->setText(s.value("lastEmail").toString());
    m_remember->setChecked(!m_loginEmail->text().isEmpty());

    connect(m_tabLogin, &QPushButton::clicked, this, [this]{ showMode(0); });
    connect(m_tabReg,   &QPushButton::clicked, this, [this]{ showMode(1); });
    connect(loginBtn, &QPushButton::clicked, this, &SignInPage::doLogin);
    connect(m_loginPass,  &QLineEdit::returnPressed, this, &SignInPage::doLogin);
    connect(regBtn,   &QPushButton::clicked, this, &SignInPage::doRegister);
    connect(m_regPass2,   &QLineEdit::returnPressed, this, &SignInPage::doRegister);
    connect(m_regPass, &QLineEdit::textChanged, this, &SignInPage::updateStrength);
    connect(m_showPass, &QCheckBox::toggled, this, [this](bool on){
        const auto mode = on ? QLineEdit::Normal : QLineEdit::Password;
        for (auto *e : {m_loginPass, m_regPass, m_regPass2}) e->setEchoMode(mode);
    });
    connect(forgot, &QPushButton::clicked, this, [this]{
        QMessageBox::information(this, "Reset password",
            "Demo app: in a real app we would email a reset link to the address you typed.");
    });
    for (auto *b : {google, apple})
        connect(b, &QPushButton::clicked, this, [this]{
            QMessageBox::information(this, "Social sign-in",
                "Demo app: this button would open the provider's sign-in page (OAuth).");
        });
    connect(guest, &QPushButton::clicked, this, [this]{
        m_name = "Guest"; m_guest = true;
        emit accountChanged(m_name, true);
        emit nextRequested();
    });
}

void SignInPage::showMode(int mode)
{
    m_forms->setCurrentIndex(mode);
    m_tabLogin->setChecked(mode == 0);
    m_tabReg->setChecked(mode == 1);
}

void SignInPage::updateStrength(const QString &pw)
{
    if (pw.isEmpty()) { m_strength->clear(); return; }
    int score = 0;
    if (pw.size() >= 8) ++score;
    if (pw.contains(QRegularExpression("[a-z]")) && pw.contains(QRegularExpression("[A-Z]"))) ++score;
    if (pw.contains(QRegularExpression("\\d"))) ++score;
    if (pw.contains(QRegularExpression("[^A-Za-z0-9]"))) ++score;
    static const char *txt[] = {"Weak", "Weak", "Okay", "Good", "Strong"};
    static const char *col[] = {"#c62828", "#c62828", "#ef6c00", "#2e7d32", "#2e7d32"};
    m_strength->setText(QString("Password strength: <b style='color:%1'>%2</b>").arg(col[score], txt[score]));
}

void SignInPage::loginAs(const QString &name)
{
    m_name = name; m_guest = false;
    emit accountChanged(m_name, false);
    emit nextRequested();
}

void SignInPage::doLogin()
{
    const QString email = m_loginEmail->text().trimmed();
    if (!validEmail(email))            { showError(m_loginErr, "Enter a valid email address."); return; }
    if (m_loginPass->text().isEmpty()) { showError(m_loginErr, "Enter your password."); return; }

    QSettings s;
    const QString key = accountKey(email);
    if (!s.contains(key + "/hash")) {
        showError(m_loginErr, "No account found for that email — use \"Create account\" or continue as a guest.");
        return;
    }
    if (hashPw(s.value(key + "/salt").toString(), m_loginPass->text()) != s.value(key + "/hash").toString()) {
        showError(m_loginErr, "Incorrect password.");
        return;
    }
    if (m_remember->isChecked()) s.setValue("lastEmail", email); else s.remove("lastEmail");
    showError(m_loginErr, QString());
    m_loginPass->clear();
    loginAs(s.value(key + "/name").toString());
}

void SignInPage::doRegister()
{
    const QString name = m_regName->text().trimmed();
    const QString email = m_regEmail->text().trimmed();
    QString err;
    if (name.isEmpty())                       err = "Please enter your name.";
    else if (!validEmail(email))              err = "Enter a valid email address.";
    else if (m_regPass->text().size() < 8)    err = "Password must be at least 8 characters.";
    else if (m_regPass->text() != m_regPass2->text()) err = "Passwords do not match.";
    else if (!m_terms->isChecked())           err = "Please accept the Terms & Privacy Policy.";
    if (!err.isEmpty()) { showError(m_regErr, err); return; }

    QSettings s;
    const QString key = accountKey(email);
    if (s.contains(key + "/hash")) {
        showError(m_regErr, "An account with that email already exists — try logging in.");
        return;
    }
    // Demo only: salted SHA-256 kept in local settings. A real app needs a server
    // and a slow password hash (bcrypt / argon2).
    const QString salt = QUuid::createUuid().toString(QUuid::WithoutBraces);
    s.setValue(key + "/salt", salt);
    s.setValue(key + "/hash", hashPw(salt, m_regPass->text()));
    s.setValue(key + "/name", name);

    showError(m_regErr, QString());
    m_regPass->clear(); m_regPass2->clear();
    loginAs(name);
}

void SignInPage::signOut()
{
    m_name = "Guest"; m_guest = true;
    m_loginPass->clear();
    showError(m_loginErr, QString());
    showMode(0);
    emit accountChanged(m_name, true);
}

// ============================================================
// 1. Restaurant (map + list)
// ============================================================
RestaurantPage::RestaurantPage(QWidget *parent) : QScrollArea(parent)
{
    auto *v = ui::beginPage(this);
    v->addWidget(ui::pageHeader(QString::fromUtf8("\U0001F4CD  Choose a restaurant"),
                                "Tap a pin on the map or pick from the list. Distances are measured from your location."));

    m_items = defaultRestaurants();

    auto *row = new QHBoxLayout;
    row->setSpacing(18);

    auto *left = new QVBoxLayout;
    left->setSpacing(10);
    auto *search = new QLineEdit;
    search->setPlaceholderText(QString::fromUtf8("\U0001F50D  Search restaurants…"));
    search->setClearButtonEnabled(true);
    left->addWidget(search);

    m_list = new QListWidget;
    m_list->setObjectName("restList");
    m_list->setIconSize(QSize(44, 44));
    m_list->setSpacing(0);
    m_list->setMinimumHeight(380);
    m_list->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_list->setFocusPolicy(Qt::NoFocus);
    for (const auto &r : m_items) {
        QIcon ic(r.icon);
        if (ic.availableSizes().isEmpty()) ic = QIcon(ui::logoPixmap(64));
        const double km = distanceKm(kUserLat, kUserLon, r.lat, r.lon);
        auto *it = new QListWidgetItem(
            ic, QString("%1\n%2 km  •  ★ %3  •  %4")
                    .arg(r.name, QString::number(km, 'f', 1), QString::number(r.rating, 'f', 1), r.hours));
        it->setSizeHint(QSize(0, 82));
        m_list->addItem(it);
    }
    left->addWidget(m_list, 1);
    row->addLayout(left, 2);

    m_map = new MapWidget;
    m_map->setMinimumHeight(420);
    m_map->setRestaurants(m_items);
    row->addWidget(m_map, 3);
    v->addLayout(row);

    QVBoxLayout *body = nullptr;
    auto *detailCard = ui::card("Selected restaurant", &body);
    m_detail = new QLabel;
    m_detail->setTextFormat(Qt::RichText);
    m_detail->setWordWrap(true);
    body->addWidget(m_detail);
    v->addWidget(detailCard);

    auto *next = button(QString::fromUtf8("Continue \u2192"), "primaryButton");
    v->addWidget(next, 0, Qt::AlignRight);
    v->addStretch();

    connect(m_list, &QListWidget::currentRowChanged, this, [this](int row){
        if (row < 0) return;
        m_map->setSelected(row);
        updateDetail(row);
    });
    connect(m_map, &MapWidget::restaurantClicked, this, [this](int i){ m_list->setCurrentRow(i); });
    connect(search, &QLineEdit::textChanged, this, &RestaurantPage::applyFilter);
    connect(next, &QPushButton::clicked, this, &RestaurantPage::nextRequested);

    m_list->setCurrentRow(0);
}

void RestaurantPage::applyFilter(const QString &text)
{
    int firstVisible = -1;
    for (int i = 0; i < m_list->count(); ++i) {
        const bool match = text.trimmed().isEmpty()
            || m_items[i].name.contains(text.trimmed(), Qt::CaseInsensitive)
            || m_items[i].address.contains(text.trimmed(), Qt::CaseInsensitive);
        m_list->item(i)->setHidden(!match);
        if (match && firstVisible < 0) firstVisible = i;
    }
    auto *cur = m_list->currentItem();
    if ((!cur || cur->isHidden()) && firstVisible >= 0) m_list->setCurrentRow(firstVisible);
}

void RestaurantPage::updateDetail(int row)
{
    const Restaurant &r = m_items[row];
    const double km = distanceKm(kUserLat, kUserLon, r.lat, r.lon);
    m_detail->setText(QString("<span style='font-size:16px'><b>%1</b></span><br>"
                              "%2<br><span style='color:#2e7d32'><b>%3</b></span> &nbsp;•&nbsp; ★ %4 "
                              "&nbsp;•&nbsp; %5 km away &nbsp;•&nbsp; ~%6 min &nbsp;•&nbsp; Delivery %7")
                          .arg(r.name, r.address, r.hours, QString::number(r.rating, 'f', 1),
                               QString::number(km, 'f', 1), QString::number(etaMinutesFor(r)), money(r.fee)));
}

QString RestaurantPage::selectedRestaurant() const
{
    const int r = m_list->currentRow();
    return r >= 0 ? m_items[r].name : QString();
}
QString RestaurantPage::selectedAddress() const
{
    const int r = m_list->currentRow();
    return r >= 0 ? m_items[r].address : QString();
}
double RestaurantPage::deliveryFee() const
{
    const int r = m_list->currentRow();
    return r >= 0 ? m_items[r].fee : 2.99;
}
int RestaurantPage::etaMinutes() const
{
    const int r = m_list->currentRow();
    return r >= 0 ? etaMinutesFor(m_items[r]) : 30;
}

// ============================================================
// 2. Pizza Builder
// ============================================================
namespace {
struct Preset { const char *emoji; const char *name; const char *desc; QStringList tops; };
const QList<Preset> &presets()
{
    static const QList<Preset> p = {
        {"✨", "Start from scratch", "Pick every topping yourself", {}},
        {"🧀", "Margherita",         "Classic cheese & sauce",       {}},
        {"🍕", "Pepperoni Classic",  "Loaded with pepperoni",        {"Pepperoni"}},
        {"🥬", "Veggie Delight",     "Peppers, mushroom, onion, olives", {"Green Pepper", "Mushroom", "Onion", "Olives"}},
        {"🍍", "Hawaiian",           "Ham & pineapple",              {"Ham", "Pineapple"}},
        {"🥩", "Meat Lovers",        "Pepperoni, ham & beef",        {"Pepperoni", "Ham", "Beef"}},
    };
    return p;
}
struct TopDef { const char *name; const char *emoji; };
const TopDef kTops[] = {
    {"Pepperoni","🔴"}, {"Ham","🥓"}, {"Beef","🥩"}, {"Green Pepper","🫑"}, {"Mushroom","🍄"},
    {"Pineapple","🍍"}, {"Onion","🧅"}, {"Olives","🫒"}, {"Artichokes","🥬"}, {"Extra Sauce","🥫"},
};
const char  *kSizes[]  = {"Small", "Medium", "Large"};
const double kSizeP[]  = {8.99, 10.99, 12.99};
const char  *kCrusts[] = {"Regular", "Thin", "Stuffed", "Gluten-free"};
const double kCrustP[] = {0.0, 0.0, 2.0, 2.5};
const double kTopP = 1.25, kCheeseP = 1.50;
}

PizzaPage::PizzaPage(QWidget *parent) : QScrollArea(parent)
{
    auto *v = ui::beginPage(this);
    v->addWidget(ui::pageHeader(QString::fromUtf8("Build your pizza  \U0001F355"),
                                "Start with a favourite or make it your own."));

    // ---- popular pizzas (its own roomy card) ---------------------------
    QVBoxLayout *pb = nullptr;
    auto *presetCard = ui::card("Popular pizzas", &pb);
    auto *grid = new QGridLayout;
    grid->setHorizontalSpacing(14);
    grid->setVerticalSpacing(14);
    for (int i = 0; i < presets().size(); ++i) {
        const Preset &p = presets()[i];
        auto *b = button(QString("%1  %2\n%3").arg(p.emoji, p.name, p.desc), "presetCard");
        b->setCheckable(true);
        b->setMinimumHeight(86);
        b->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        grid->addWidget(b, i / 3, i % 3);
        m_presetBtns << b;
        connect(b, &QPushButton::clicked, this, [this, i]{ applyPreset(i); });
    }
    pb->addLayout(grid);
    v->addWidget(presetCard);
    v->addSpacing(18);                                   // extra air before the ingredients

    // ---- size / crust / quantity ---------------------------------------
    QVBoxLayout *sb = nullptr;
    auto *sizeCard = ui::card("Size & crust", &sb);
    m_sizeGroup = new QButtonGroup(this);
    auto *sizeRow = new QHBoxLayout;
    sizeRow->setSpacing(0);
    for (int i = 0; i < 3; ++i) {
        auto *b = segment(QString("%1  ·  %2").arg(kSizes[i], money(kSizeP[i])));
        m_sizeGroup->addButton(b, i);
        sizeRow->addWidget(b);
    }
    m_sizeGroup->button(1)->setChecked(true);
    sizeRow->addStretch();
    sb->addWidget(new QLabel("Size"));
    sb->addLayout(sizeRow);

    m_crustGroup = new QButtonGroup(this);
    auto *crustRow = new QHBoxLayout;
    crustRow->setSpacing(0);
    for (int i = 0; i < 4; ++i) {
        auto *b = segment(kCrustP[i] > 0 ? QString("%1  (+%2)").arg(kCrusts[i], money(kCrustP[i]))
                                         : QString(kCrusts[i]));
        m_crustGroup->addButton(b, i);
        crustRow->addWidget(b);
    }
    m_crustGroup->button(0)->setChecked(true);
    crustRow->addStretch();
    sb->addWidget(new QLabel("Crust"));
    sb->addLayout(crustRow);

    auto *qtyRow = new QHBoxLayout;
    qtyRow->addWidget(new QLabel("Quantity"));
    m_qty = new QSpinBox;
    m_qty->setRange(1, 10);
    m_qty->setFixedWidth(60);
    m_qty->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_qty->setAlignment(Qt::AlignCenter);
    auto *minus = button(QString::fromUtf8("\u2212"), "ghostButton");
    auto *plus  = button("+", "ghostButton");
    minus->setFixedWidth(44); plus->setFixedWidth(44);
    connect(minus, &QPushButton::clicked, m_qty, [this]{ m_qty->stepBy(-1); });
    connect(plus,  &QPushButton::clicked, m_qty, [this]{ m_qty->stepBy(1); });
    qtyRow->addWidget(minus);
    qtyRow->addWidget(m_qty);
    qtyRow->addWidget(plus);
    qtyRow->addStretch();
    auto *reset = button(QString::fromUtf8("Reset pizza \u27F3"), "ghostButton");
    qtyRow->addWidget(reset);
    sb->addLayout(qtyRow);
    v->addWidget(sizeCard);

    // ---- toppings --------------------------------------------------------
    QVBoxLayout *tb = nullptr;
    auto *topCard = ui::card(QString("Toppings  ·  %1 each").arg(money(kTopP)), &tb);
    auto *tg = new QGridLayout;
    tg->setHorizontalSpacing(18);
    tg->setVerticalSpacing(6);
    int idx = 0;
    for (const auto &t : kTops) {
        auto *cb = new QCheckBox(QString("%1  %2").arg(t.emoji, t.name));
        cb->setProperty("tname", QString(t.name));
        m_tops << cb;
        tg->addWidget(cb, idx / 3, idx % 3);
        ++idx;
    }
    m_extraCheese = new QCheckBox(QString::fromUtf8("\U0001F9C0  Extra Cheese  (+$1.50)"));
    tg->addWidget(m_extraCheese, (idx + 2) / 3, 0, 1, 3);
    tb->addLayout(tg);
    v->addWidget(topCard);

    // ---- summary ----------------------------------------------------------
    QVBoxLayout *mb = nullptr;
    auto *sumCard = ui::card("Your pizza", &mb);
    m_summary = new QLabel;
    m_summary->setTextFormat(Qt::RichText);
    m_summary->setWordWrap(true);
    mb->addWidget(m_summary);
    m_notes = new QLineEdit;
    m_notes->setPlaceholderText("Special instructions (optional) — e.g. well done, cut in squares");
    mb->addWidget(m_notes);
    v->addWidget(sumCard);

    auto *nav = new QHBoxLayout;
    auto *back = button(QString::fromUtf8("\u2190 Back"));
    auto *next = button(QString::fromUtf8("Continue \u2192"), "primaryButton");
    nav->addWidget(back);
    nav->addStretch();
    nav->addWidget(next);
    v->addLayout(nav);
    v->addStretch();

    // ---- wiring -------------------------------------------------------------
    for (auto *cb : m_tops) connect(cb, &QCheckBox::toggled, this, &PizzaPage::userChangedRecipe);
    connect(m_extraCheese, &QCheckBox::toggled, this, &PizzaPage::userChangedRecipe);
    connect(m_sizeGroup,  &QButtonGroup::idClicked, this, [this]{ refreshSummary(); });
    connect(m_crustGroup, &QButtonGroup::idClicked, this, [this]{ refreshSummary(); });
    connect(m_qty, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]{ refreshSummary(); });
    connect(reset, &QPushButton::clicked, this, &PizzaPage::resetPizza);
    connect(back, &QPushButton::clicked, this, &PizzaPage::backRequested);
    connect(next, &QPushButton::clicked, this, &PizzaPage::nextRequested);

    applyPreset(0);
}

void PizzaPage::applyPreset(int index)
{
    m_applying = true;
    for (auto *cb : m_tops) cb->setChecked(presets()[index].tops.contains(cb->property("tname").toString()));
    m_extraCheese->setChecked(false);
    m_preset = index;
    for (int i = 0; i < m_presetBtns.size(); ++i) m_presetBtns[i]->setChecked(i == index);
    m_applying = false;
    refreshSummary();
}

void PizzaPage::userChangedRecipe()
{
    if (m_applying) return;
    if (m_preset != -1) {                    // user edited a preset -> now a custom pizza
        m_preset = -1;
        for (auto *b : m_presetBtns) b->setChecked(false);
    }
    refreshSummary();
}

void PizzaPage::resetPizza()
{
    m_sizeGroup->button(1)->setChecked(true);
    m_crustGroup->button(0)->setChecked(true);
    m_qty->setValue(1);
    m_notes->clear();
    applyPreset(0);
}

QString PizzaPage::pizzaName() const
{
    return (m_preset > 0) ? QString(presets()[m_preset].name) : QString("Custom pizza");
}
QString PizzaPage::size() const  { return kSizes[qMax(0, m_sizeGroup->checkedId())]; }
QString PizzaPage::crust() const { return kCrusts[qMax(0, m_crustGroup->checkedId())]; }
QStringList PizzaPage::toppings() const
{
    QStringList out;
    for (auto *cb : m_tops) if (cb->isChecked()) out << cb->property("tname").toString();
    return out;
}
bool PizzaPage::extraCheese() const { return m_extraCheese->isChecked(); }
int  PizzaPage::quantity() const    { return m_qty->value(); }
QString PizzaPage::notes() const    { return m_notes->text().trimmed(); }

double PizzaPage::unitPrice() const
{
    double p = kSizeP[qMax(0, m_sizeGroup->checkedId())] + kCrustP[qMax(0, m_crustGroup->checkedId())];
    p += toppings().size() * kTopP;
    if (m_extraCheese->isChecked()) p += kCheeseP;
    return p;
}
double PizzaPage::subtotal() const { return unitPrice() * m_qty->value(); }

void PizzaPage::refreshSummary()
{
    const QStringList tops = toppings();
    QString h = QString("<b style='font-size:15px'>%1</b><br>").arg(pizzaName());
    h += QString("<b>Size:</b> %1 &nbsp; <b>Crust:</b> %2 &nbsp; <b>Qty:</b> %3<br>")
             .arg(size(), crust()).arg(quantity());
    h += QString("<b>Toppings:</b> %1<br>").arg(tops.isEmpty() ? "None" : tops.join(", "));
    if (extraCheese()) h += "<b>Extra cheese:</b> yes (+$1.50)<br>";
    h += QString("<div style='margin-top:6px;color:#2e7d32;font-size:17px'><b>Subtotal: %1</b></div>")
             .arg(money(subtotal()));
    m_summary->setText(h);
    emit summaryChanged();
}

// ============================================================
// 3. Delivery details
// ============================================================
DeliveryPage::DeliveryPage(QWidget *parent) : QScrollArea(parent)
{
    auto *v = ui::beginPage(this);
    v->addWidget(ui::pageHeader(QString::fromUtf8("Delivery details  \U0001F3E0"),
                                "Almost done — where should the driver go?"));

    QVBoxLayout *wb = nullptr;
    auto *whereCard = ui::card("Where to?", &wb);
    m_name   = new QLineEdit; m_name->setPlaceholderText("Your full name");
    m_phone  = new QLineEdit; m_phone->setPlaceholderText("(416) 555-0123");
    m_street = new QLineEdit; m_street->setPlaceholderText("Street address");
    m_apt    = new QLineEdit; m_apt->setPlaceholderText("Apt / unit / buzzer (optional)");
    m_city   = new QLineEdit; m_city->setPlaceholderText("City");
    m_postal = new QLineEdit; m_postal->setPlaceholderText("A1A 1A1");
    m_postal->setMaxLength(7);

    auto *g = new QGridLayout;
    g->setHorizontalSpacing(16);
    g->setVerticalSpacing(14);
    g->addWidget(labeled("Name", m_name),            0, 0);
    g->addWidget(labeled("Phone", m_phone),          0, 1);
    g->addWidget(labeled("Street", m_street),        1, 0);
    g->addWidget(labeled("Apartment", m_apt),        1, 1);
    g->addWidget(labeled("City", m_city),            2, 0);
    g->addWidget(labeled("Postal code", m_postal),   2, 1);
    wb->addLayout(g);
    v->addWidget(whereCard);

    QVBoxLayout *ob = nullptr;
    auto *optCard = ui::card("Delivery options", &ob);
    m_when = new QComboBox;
    m_when->addItems({"As soon as possible", "In about 1 hour", "In about 2 hours", "Tonight at 7:00 PM"});
    m_contactless = new QCheckBox(QString::fromUtf8("\U0001F6AA  Contactless — leave at the door"));
    m_instr = new QLineEdit;
    m_instr->setPlaceholderText("Delivery instructions (optional) — e.g. ring twice, gate code 1234");
    ob->addWidget(labeled("When", m_when));
    ob->addWidget(m_contactless);
    ob->addWidget(m_instr);
    v->addWidget(optCard);

    m_error = errorLabel();
    v->addWidget(m_error);

    auto *row = new QHBoxLayout;
    auto *back = button(QString::fromUtf8("\u2190 Back"));
    auto *next = button(QString::fromUtf8("Continue to payment \u2192"), "primaryButton");
    row->addWidget(back);
    row->addStretch();
    row->addWidget(next);
    v->addLayout(row);
    v->addStretch();

    connect(m_postal, &QLineEdit::textEdited, this, [this](const QString &t){
        QString u = t.toUpper();
        if (u != t) m_postal->setText(u);
        const bool bad = !u.isEmpty() && u.size() >= 6
            && !QRegularExpression("^[A-Z]\\d[A-Z] ?\\d[A-Z]\\d$").match(u).hasMatch();
        m_postal->setProperty("invalid", bad);
        m_postal->style()->unpolish(m_postal);
        m_postal->style()->polish(m_postal);
    });
    connect(back, &QPushButton::clicked, this, &DeliveryPage::backRequested);
    connect(next, &QPushButton::clicked, this, [this]{ if (valid()) emit nextRequested(); });
}

bool DeliveryPage::valid() const
{
    QString err;
    const QString digits = QString(m_phone->text()).remove(QRegularExpression("\\D"));
    if (m_name->text().trimmed().isEmpty())        err = "Please enter your name.";
    else if (digits.size() < 10)                   err = "Please enter a phone number the driver can call (10 digits).";
    else if (m_street->text().trimmed().isEmpty()) err = "Please enter your street address.";
    else if (m_city->text().trimmed().isEmpty())   err = "Please enter your city.";
    else if (!QRegularExpression("^[A-Za-z]\\d[A-Za-z] ?\\d[A-Za-z]\\d$").match(m_postal->text().trimmed()).hasMatch())
        err = "Postal code should look like A1A 1A1.";
    showError(m_error, err);
    return err.isEmpty();
}

void DeliveryPage::prefillName(const QString &name)
{
    if (m_name->text().trimmed().isEmpty()) m_name->setText(name);
}

QString DeliveryPage::name() const       { return m_name->text().trimmed(); }
QString DeliveryPage::address() const
{
    QString a = m_street->text().trimmed();
    if (!m_apt->text().trimmed().isEmpty()) a += ", " + m_apt->text().trimmed();
    return a;
}
QString DeliveryPage::city() const       { return m_city->text().trimmed(); }
QString DeliveryPage::postalCode() const { return m_postal->text().trimmed(); }
QString DeliveryPage::phone() const      { return m_phone->text().trimmed(); }
QString DeliveryPage::schedule() const   { return m_when->currentText(); }
QString DeliveryPage::instructions() const
{
    QStringList parts;
    if (m_contactless->isChecked()) parts << "Contactless drop-off";
    if (!m_instr->text().trimmed().isEmpty()) parts << m_instr->text().trimmed();
    return parts.join(" — ");
}

void DeliveryPage::clear()
{
    m_name->clear(); m_phone->clear(); m_street->clear(); m_apt->clear();
    m_city->clear(); m_postal->clear(); m_instr->clear();
    m_contactless->setChecked(false);
    m_when->setCurrentIndex(0);
    showError(m_error, QString());
}

// ============================================================
// 4. Payment
// ============================================================
namespace {
QString cardBrand(const QString &d)
{
    if (d.startsWith('4')) return "Visa";
    if (d.size() >= 2) {
        const int p2 = d.left(2).toInt();
        if (p2 >= 51 && p2 <= 55) return "Mastercard";
        if (p2 == 34 || p2 == 37) return "Amex";
    }
    if (d.size() >= 4) {
        const int p4 = d.left(4).toInt();
        if (p4 >= 2221 && p4 <= 2720) return "Mastercard";
    }
    return QString();
}
bool luhn(const QString &d)
{
    int sum = 0; bool dbl = false;
    for (int i = d.size() - 1; i >= 0; --i) {
        int n = d[i].digitValue();
        if (dbl) { n *= 2; if (n > 9) n -= 9; }
        sum += n; dbl = !dbl;
    }
    return sum % 10 == 0;
}
const char *kMethods[] = {"💳  Card", "🍎  Apple Pay", "📱  Google Pay", "🅿️  PayPal", "💵  Cash", "🎁  Gift card"};
const int   kTipPct[]  = {0, 10, 15, 20};
}

PaymentPage::PaymentPage(QWidget *parent) : QScrollArea(parent)
{
    auto *v = ui::beginPage(this);
    v->addWidget(ui::pageHeader(QString::fromUtf8("Payment  \U0001F4B3"),
                                "Choose how you'd like to pay. All transactions are encrypted."));

    QVBoxLayout *mb = nullptr;
    auto *methodCard = ui::card("Payment method", &mb);
    auto *mg = new QGridLayout;
    mg->setHorizontalSpacing(12);
    mg->setVerticalSpacing(12);
    for (int i = 0; i < 6; ++i) {
        auto *b = button(kMethods[i], "payMethod");
        b->setCheckable(true);
        b->setMinimumHeight(56);
        b->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        mg->addWidget(b, i / 3, i % 3);
        m_methodBtns << b;
        connect(b, &QPushButton::clicked, this, [this, i]{ selectMethod(i); });
    }
    mb->addLayout(mg);
    mb->addSpacing(6);

    m_stack = new QStackedWidget;

    // 0: card
    auto *cw = new QWidget;
    auto *cv = new QVBoxLayout(cw);
    cv->setContentsMargins(0, 0, 0, 0);
    cv->setSpacing(14);
    m_holder = new QLineEdit; m_holder->setPlaceholderText("Name on card");
    m_card   = new QLineEdit; m_card->setPlaceholderText("1234 5678 9012 3456");
    m_expiry = new QLineEdit; m_expiry->setPlaceholderText("MM/YY"); m_expiry->setMaxLength(5);
    m_cvv    = new QLineEdit; m_cvv->setPlaceholderText("123"); m_cvv->setEchoMode(QLineEdit::Password);
    m_cvv->setMaxLength(4);
    m_brand = new QLabel;
    m_brand->setObjectName("brandChip");
    auto *numRow = new QHBoxLayout;
    numRow->addWidget(m_card, 1);
    numRow->addWidget(m_brand);
    auto *numWrap = new QWidget; numWrap->setLayout(numRow); numRow->setContentsMargins(0, 0, 0, 0);
    auto *ec = new QHBoxLayout;
    ec->setSpacing(16);
    ec->addWidget(labeled("Expiry", m_expiry));
    ec->addWidget(labeled("CVV", m_cvv));
    m_save = new QCheckBox("Save this card for next time");
    cv->addWidget(labeled("Card holder", m_holder));
    cv->addWidget(labeled("Card number", numWrap));
    cv->addLayout(ec);
    cv->addWidget(m_save);
    m_stack->addWidget(cw);

    // 1: wallets (Apple Pay / Google Pay)
    auto *ww = new QWidget;
    auto *wv = new QVBoxLayout(ww);
    wv->setContentsMargins(0, 0, 0, 0);
    m_walletInfo = new QLabel;
    m_walletInfo->setWordWrap(true);
    m_walletInfo->setObjectName("infoBox");
    wv->addWidget(m_walletInfo);
    m_stack->addWidget(ww);

    // 2: PayPal
    auto *pw = new QWidget;
    auto *pv = new QVBoxLayout(pw);
    pv->setContentsMargins(0, 0, 0, 0);
    m_paypal = new QLineEdit; m_paypal->setPlaceholderText("PayPal email");
    pv->addWidget(labeled("PayPal account", m_paypal));
    m_stack->addWidget(pw);

    // 3: cash
    auto *kw = new QWidget;
    auto *kv = new QVBoxLayout(kw);
    kv->setContentsMargins(0, 0, 0, 0);
    kv->setSpacing(12);
    auto *cashInfo = new QLabel("Pay your driver in cash when the pizza arrives.");
    cashInfo->setObjectName("infoBox");
    m_change = new QLineEdit; m_change->setPlaceholderText("e.g. 50 (optional — so the driver brings change)");
    kv->addWidget(cashInfo);
    kv->addWidget(labeled("Paying with a bill of", m_change));
    m_stack->addWidget(kw);

    // 4: gift card
    auto *gw = new QWidget;
    auto *gv = new QVBoxLayout(gw);
    gv->setContentsMargins(0, 0, 0, 0);
    m_gift = new QLineEdit; m_gift->setPlaceholderText("Gift card code (8+ letters / digits)");
    gv->addWidget(labeled("Gift card code", m_gift));
    m_stack->addWidget(gw);

    mb->addWidget(m_stack);
    v->addWidget(methodCard);

    // tip
    QVBoxLayout *tb = nullptr;
    auto *tipCard = ui::card("Tip your driver", &tb);
    m_tipGroup = new QButtonGroup(this);
    auto *tr = new QHBoxLayout;
    tr->setSpacing(0);
    for (int i = 0; i < 4; ++i) {
        auto *b = segment(kTipPct[i] == 0 ? QString("No tip") : QString("%1%").arg(kTipPct[i]));
        m_tipGroup->addButton(b, i);
        tr->addWidget(b);
    }
    m_tipGroup->button(2)->setChecked(true);
    tr->addStretch();
    tb->addLayout(tr);
    v->addWidget(tipCard);

    m_error = errorLabel();
    v->addWidget(m_error);

    auto *row = new QHBoxLayout;
    auto *back = button(QString::fromUtf8("\u2190 Back"));
    auto *next = button(QString::fromUtf8("Review order \u2192"), "primaryButton");
    row->addWidget(back);
    row->addStretch();
    row->addWidget(next);
    v->addLayout(row);
    v->addStretch();

    connect(m_card,   &QLineEdit::textChanged, this, &PaymentPage::formatCard);
    connect(m_expiry, &QLineEdit::textChanged, this, &PaymentPage::formatExpiry);
    connect(back, &QPushButton::clicked, this, &PaymentPage::backRequested);
    connect(next, &QPushButton::clicked, this, [this]{ if (valid()) emit nextRequested(); });

    selectMethod(0);
}

void PaymentPage::selectMethod(int m)
{
    m_method = m;
    for (int i = 0; i < m_methodBtns.size(); ++i) m_methodBtns[i]->setChecked(i == m);
    static const int stackIdx[] = {0, 1, 1, 2, 3, 4};
    m_stack->setCurrentIndex(stackIdx[m]);
    if (m == 1) m_walletInfo->setText("You'll confirm the payment with Apple Pay (Face ID / Touch ID) when you place the order.");
    if (m == 2) m_walletInfo->setText("You'll confirm the payment with Google Pay on your device when you place the order.");
    showError(m_error, QString());
}

void PaymentPage::formatCard(const QString &text)
{
    QString digits = text;
    digits.remove(QRegularExpression("[^0-9]"));
    const QString brand = cardBrand(digits);
    digits.truncate(brand == "Amex" ? 15 : 16);
    QString out;
    for (int i = 0; i < digits.size(); ++i) {
        const bool gap = (brand == "Amex") ? (i == 4 || i == 10) : (i && i % 4 == 0);
        if (gap) out += ' ';
        out += digits[i];
    }
    m_brand->setText(brand);
    m_brand->setVisible(!brand.isEmpty());
    m_cvv->setMaxLength(brand == "Amex" ? 4 : 3);
    if (out != text) {
        QSignalBlocker b(m_card);
        m_card->setText(out);
    }
}

void PaymentPage::formatExpiry(const QString &text)
{
    QString digits = text;
    digits.remove(QRegularExpression("[^0-9]"));
    digits.truncate(4);

    if (digits.size() >= 2) {                           // month 01-12
        const int month = digits.left(2).toInt();
        if (month < 1 || month > 12) digits.remove(1, 1);
    }
    if (digits.size() == 4) {                           // not expired, max 10 years ahead
        const int month = digits.left(2).toInt();
        const int year = 2000 + digits.mid(2, 2).toInt();
        const QDate now = QDate::currentDate();
        if (year < now.year() || (year == now.year() && month < now.month()) || year > now.year() + 10)
            digits.truncate(2);
    }
    QString out;
    for (int i = 0; i < digits.size(); ++i) {
        if (i == 2) out += '/';
        out += digits[i];
    }
    if (out != text) {
        QSignalBlocker b(m_expiry);
        m_expiry->setText(out);
        m_expiry->setCursorPosition(out.size());
    }
}

bool PaymentPage::valid() const
{
    QString err;
    switch (m_method) {
    case 0: {
        QString card = m_card->text(); card.remove(' ');
        const int need = (cardBrand(card) == "Amex") ? 15 : 16;
        if (m_holder->text().trimmed().isEmpty())                  err = "Card holder is required.";
        else if (card.size() != need || !luhn(card))               err = "That card number doesn't look valid.";
        else if (!QRegularExpression("^\\d{2}/\\d{2}$").match(m_expiry->text()).hasMatch()) err = "Expiry must be MM/YY.";
        else if (m_cvv->text().size() < 3)                         err = "CVV must be 3–4 digits.";
        break; }
    case 3:
        if (!validEmail(m_paypal->text().trimmed())) err = "Enter the email of your PayPal account.";
        break;
    case 5: {
        QString g = m_gift->text(); g.remove(QRegularExpression("[^A-Za-z0-9]"));
        if (g.size() < 8) err = "Gift card codes have at least 8 letters or digits.";
        break; }
    default: break;
    }
    showError(m_error, err);
    return err.isEmpty();
}

QString PaymentPage::maskedCard() const
{
    QString card = m_card->text(); card.remove(' ');
    if (card.size() < 4) return "****";
    return "**** **** **** " + card.right(4);
}

QString PaymentPage::paymentSummary() const
{
    switch (m_method) {
    case 0: { QString c = m_card->text(); c.remove(' ');
              const QString b = cardBrand(c);
              return (b.isEmpty() ? QString("Card") : b) + " " + maskedCard(); }
    case 1: return "Apple Pay";
    case 2: return "Google Pay";
    case 3: return "PayPal (" + m_paypal->text().trimmed() + ")";
    case 4: return m_change->text().trimmed().isEmpty() ? QString("Cash on delivery")
                                                        : "Cash on delivery (bill of $" + m_change->text().trimmed() + ")";
    default: { QString g = m_gift->text(); g.remove(QRegularExpression("[^A-Za-z0-9]"));
               return "Gift card ••••" + g.right(4); }
    }
}

int PaymentPage::tipPercent() const { return kTipPct[qMax(0, m_tipGroup->checkedId())]; }

void PaymentPage::clear()
{
    m_holder->clear(); m_card->clear(); m_expiry->clear(); m_cvv->clear();
    m_paypal->clear(); m_gift->clear(); m_change->clear();
    m_save->setChecked(false);
    m_tipGroup->button(2)->setChecked(true);
    selectMethod(0);
}

// ============================================================
// 5. Review / Confirm
// ============================================================
ReviewPage::ReviewPage(QWidget *parent) : QScrollArea(parent)
{
    auto *v = ui::beginPage(this);
    v->addWidget(ui::pageHeader(QString::fromUtf8("Confirm your order  \U0001F6D2"),
                                "Check everything below, add a promo code if you have one, then place your order."));

    QVBoxLayout *sb = nullptr;
    auto *sumCard = ui::card("Summary", &sb);
    m_body = new QLabel;
    m_body->setTextFormat(Qt::RichText);
    m_body->setWordWrap(true);
    sb->addWidget(m_body);
    v->addWidget(sumCard);

    QVBoxLayout *pb = nullptr;
    auto *promoCard = ui::card("Promo code", &pb);
    auto *pr = new QHBoxLayout;
    m_promo = new QLineEdit;
    m_promo->setPlaceholderText("Try PIZZA10, SAVE5 or FREEDEL");
    auto *apply = button("Apply", "ghostButton");
    pr->addWidget(m_promo, 1);
    pr->addWidget(apply);
    pb->addLayout(pr);
    m_promoStatus = new QLabel;
    m_promoStatus->hide();
    pb->addWidget(m_promoStatus);
    v->addWidget(promoCard);

    auto *row = new QHBoxLayout;
    auto *back  = button(QString::fromUtf8("\u2190 Modify order"));
    auto *place = button(QString::fromUtf8("Place order \u2714"), "primaryButton");
    row->addWidget(back);
    row->addStretch();
    row->addWidget(place);
    v->addLayout(row);
    v->addStretch();

    connect(apply, &QPushButton::clicked, this, [this]{ emit promoEntered(m_promo->text()); });
    connect(m_promo, &QLineEdit::returnPressed, this, [this]{ emit promoEntered(m_promo->text()); });
    connect(back,  &QPushButton::clicked, this, &ReviewPage::backRequested);
    connect(place, &QPushButton::clicked, this, &ReviewPage::placeRequested);
}

void ReviewPage::setSummary(const QString &html) { m_body->setText(html); }

void ReviewPage::setPromoStatus(const QString &text, bool ok)
{
    m_promoStatus->setText(text);
    m_promoStatus->setStyleSheet(ok ? "color:#2e7d32;font-weight:600;" : "color:#c62828;font-weight:600;");
    m_promoStatus->setVisible(!text.isEmpty());
}

void ReviewPage::clearPromo()
{
    m_promo->clear();
    setPromoStatus(QString(), true);
}

// ============================================================
// 6. Order tracking
// ============================================================
namespace {
struct Stage { const char *title; const char *desc; const char *emoji; double at; };
const Stage kStages[] = {
    {"Order received",       "The restaurant has your order",          "📝", 0.00},
    {"Preparing your order", "Rolling dough and adding toppings",      "🔥", 0.08},
    {"Baking",               "In the oven — about 10 minutes",         "🍕", 0.28},
    {"Quality check",        "Cut, boxed and ready for the driver",    "✅", 0.50},
    {"Out for delivery",     "Your driver is on the way",              "🛵", 0.60},
    {"Delivered",            "Enjoy your pizza!",                      "🎉", 1.00},
};
const int kStageCount = 6;
const double kTotalMs = 50000.0;                  // demo speed: the whole delivery takes 50 s
}

ProgressPage::ProgressPage(QWidget *parent) : QScrollArea(parent)
{
    auto *v = ui::beginPage(this);

    m_headline = new QLabel(QString::fromUtf8("Order placed \u2714"));
    m_headline->setObjectName("pageTitle");
    m_sub = new QLabel;
    m_sub->setObjectName("pageSubtitle");
    auto *hv = new QVBoxLayout;
    hv->setSpacing(4);
    hv->addWidget(m_headline);
    hv->addWidget(m_sub);
    v->addLayout(hv);

    // ---- ETA banner ------------------------------------------------------
    QVBoxLayout *eb = nullptr;
    auto *etaCard = ui::card(QString(), &eb);
    m_eta = new QLabel;
    m_eta->setObjectName("etaBig");
    m_status = new QLabel;
    m_status->setObjectName("pageSubtitle");
    m_bar = new QProgressBar;
    m_bar->setRange(0, 100);
    m_bar->setTextVisible(false);
    m_bar->setFixedHeight(10);
    eb->addWidget(m_eta);
    eb->addWidget(m_status);
    eb->addWidget(m_bar);
    v->addWidget(etaCard);

    // ---- two columns -----------------------------------------------------
    auto *cols = new QHBoxLayout;
    cols->setSpacing(18);

    auto *leftCol = new QVBoxLayout;
    leftCol->setSpacing(18);
    QVBoxLayout *tb = nullptr;
    auto *timeCard = ui::card("Order status", &tb);
    tb->setSpacing(10);
    for (int i = 0; i < kStageCount; ++i) {
        auto *r = new QHBoxLayout;
        r->setSpacing(12);
        auto *ic = new QLabel;
        ic->setFixedSize(28, 28);
        ic->setAlignment(Qt::AlignCenter);
        auto *tx = new QLabel;
        tx->setTextFormat(Qt::RichText);
        auto *tm = new QLabel;
        tm->setObjectName("noteLabel");
        r->addWidget(ic, 0, Qt::AlignTop);
        r->addWidget(tx, 1);
        r->addWidget(tm, 0, Qt::AlignTop);
        tb->addLayout(r);
        m_icons << ic; m_texts << tx; m_times << tm;
    }
    leftCol->addWidget(timeCard);

    QVBoxLayout *db = nullptr;
    auto *detCard = ui::card("Order details", &db);
    m_details = new QLabel;
    m_details->setTextFormat(Qt::RichText);
    m_details->setWordWrap(true);
    db->addWidget(m_details);
    leftCol->addWidget(detCard);
    leftCol->addStretch();
    cols->addLayout(leftCol, 1);

    auto *rightCol = new QVBoxLayout;
    rightCol->setSpacing(18);
    QVBoxLayout *mb = nullptr;
    auto *mapCard = ui::card("Live tracking", &mb);
    m_track = new TrackMap;
    mb->addWidget(m_track);
    rightCol->addWidget(mapCard);

    QVBoxLayout *drb = nullptr;
    auto *drvCard = ui::card("Your driver", &drb);
    m_driver = new QLabel;
    m_driver->setTextFormat(Qt::RichText);
    m_driver->setWordWrap(true);
    drb->addWidget(m_driver);
    m_contact = button(QString::fromUtf8("\U0001F4DE  Contact driver"), "ghostButton");
    drb->addWidget(m_contact, 0, Qt::AlignLeft);
    rightCol->addWidget(drvCard);

    // rating (shown after delivery)
    m_rateBox = new QWidget;
    auto *rv = new QVBoxLayout(m_rateBox);
    rv->setContentsMargins(0, 0, 0, 0);
    QVBoxLayout *rb = nullptr;
    auto *rateCard = ui::card("How was your order?", &rb);
    auto *sr = new QHBoxLayout;
    for (int i = 0; i < 5; ++i) {
        auto *s = button(QString::fromUtf8("\u2606"), "starBtn");
        m_stars << s;
        sr->addWidget(s);
        connect(s, &QPushButton::clicked, this, [this, i]{
            for (int j = 0; j < m_stars.size(); ++j)
                m_stars[j]->setText(QString::fromUtf8(j <= i ? "\u2605" : "\u2606"));
        });
    }
    sr->addStretch();
    rb->addLayout(sr);
    rv->addWidget(rateCard);
    m_rateBox->hide();
    rightCol->addWidget(m_rateBox);
    rightCol->addStretch();
    cols->addLayout(rightCol, 1);
    v->addLayout(cols);

    // ---- actions ---------------------------------------------------------
    auto *act = new QHBoxLayout;
    m_cancel = button("Cancel order", "dangerButton");
    m_cancelNote = new QLabel;
    m_cancelNote->setObjectName("noteLabel");
    m_cancelNote->setWordWrap(true);
    m_newOrder = button(QString::fromUtf8("Start a new order \u2192"), "primaryButton");
    m_newOrder->hide();
    act->addWidget(m_cancel);
    act->addWidget(m_cancelNote, 1);
    act->addWidget(m_newOrder);
    v->addLayout(act);
    v->addStretch();

    m_timer = new QTimer(this);
    m_timer->setInterval(100);
    connect(m_timer, &QTimer::timeout, this, &ProgressPage::tick);
    connect(m_cancel, &QPushButton::clicked, this, &ProgressPage::cancelRequested);
    connect(m_newOrder, &QPushButton::clicked, this, &ProgressPage::newOrderRequested);
    connect(m_contact, &QPushButton::clicked, this, [this]{
        QMessageBox::information(this, "Contact driver",
            "Demo app: this would call or message your driver.");
    });
}

void ProgressPage::start(const OrderInfo &info)
{
    m_info = info;
    m_elapsed = 0;
    m_reached = -1;
    m_active = true;
    m_done = false;

    m_headline->setText(QString::fromUtf8("Order placed \u2714"));
    m_sub->setText(QString("Order <b>#%1</b> · %2").arg(info.orderNo, info.restaurant));
    m_sub->setTextFormat(Qt::RichText);
    for (auto *t : m_times) t->clear();
    for (auto *s : m_stars) s->setText(QString::fromUtf8("\u2606"));
    m_rateBox->hide();
    m_newOrder->hide();
    m_cancel->show();
    m_cancelNote->show();

    m_details->setText(QString("%1<hr style='border:1px solid #eee'>"
                               "<b>Restaurant:</b> %2<br><b>Deliver to:</b> %3<br><b>When:</b> %4<br>"
                               "<b>Payment:</b> %5<br>"
                               "<span style='color:#2e7d32;font-size:16px'><b>Total: %6</b></span>")
                           .arg(info.itemsHtml, info.restaurant, info.deliverTo, info.schedule,
                                info.payment, money(info.total)));
    render();
    m_timer->start();
}

void ProgressPage::stop()
{
    m_timer->stop();
    m_active = false;
}

void ProgressPage::tick()
{
    m_elapsed += m_timer->interval();
    render();
    if (m_elapsed >= kTotalMs) {
        m_timer->stop();
        m_done = true;
        m_headline->setText(QString::fromUtf8("Delivered \u2714"));
        m_cancel->hide();
        m_cancelNote->hide();
        m_rateBox->show();
        m_newOrder->show();
        render();
    }
}

void ProgressPage::render()
{
    const double p = qMin(1.0, m_elapsed / kTotalMs);
    int stage = 0;
    for (int i = 0; i < kStageCount; ++i) if (p >= kStages[i].at) stage = i;

    if (stage > m_reached) {
        for (int i = m_reached + 1; i <= stage; ++i)
            m_times[i]->setText(QTime::currentTime().toString("h:mm AP"));
        m_reached = stage;
    }

    for (int i = 0; i < kStageCount; ++i) {
        const bool done = (i < stage) || p >= 1.0;
        const bool cur  = (i == stage) && p < 1.0;
        QString icStyle, color = "#9aa5b1";
        if (done)      { icStyle = "background:#4CAF50;color:white;"; color = "#212529"; m_icons[i]->setText(QString::fromUtf8("\u2713")); }
        else if (cur)  { icStyle = "background:#2196F3;color:white;"; color = "#212529"; m_icons[i]->setText(kStages[i].emoji); }
        else           { icStyle = "background:#e9ecef;color:#868e96;"; m_icons[i]->setText(QString::number(i + 1)); }
        m_icons[i]->setStyleSheet(icStyle + "border-radius:14px;font-weight:bold;");
        m_texts[i]->setText(QString("<span style='color:%1;%2'>%3</span><br><span style='color:#8a949e;font-size:12px'>%4</span>")
                                .arg(color, cur ? "font-weight:bold" : "", kStages[i].title, kStages[i].desc));
    }

    m_bar->setValue(int(p * 100));
    if (p >= 1.0) {
        m_eta->setText(QString::fromUtf8("Delivered \u2014 enjoy! \U0001F355"));
        m_status->setText("Thanks for ordering with PizzaGo. Rate your order below.");
    } else {
        m_eta->setText(QString("Arriving in %1 min").arg(qCeil(m_info.etaMinutes * (1.0 - p))));
        m_status->setText(QString("%1  %2").arg(kStages[stage].emoji, kStages[stage].title));
    }

    if (stage >= 3) {
        static const char *names[] = {"Marco R.", "Aisha K.", "Daniel L.", "Priya S.", "Chen W."};
        static const char *rides[] = {"Red scooter", "Blue e-bike", "Silver Honda Fit", "White scooter", "Green e-bike"};
        const int n = int(qHash(m_info.orderNo) % 5);
        m_driver->setText(QString("<span style='font-size:16px'><b>%1</b></span> &nbsp; ★ 4.9<br>%2 · plate PZ-%3")
                              .arg(names[n], rides[n], QString::number(4000 + n * 137)));
        m_contact->setEnabled(p < 1.0);
    } else {
        m_driver->setText("<i>Finding the nearest driver…</i>");
        m_contact->setEnabled(false);
    }

    m_track->setProgress((p - 0.60) / 0.40);

    const bool canCancel = p < 0.60 && !m_done;
    m_cancel->setEnabled(canCancel);
    m_cancelNote->setText(canCancel ? "Cancel before the driver picks it up and you won't be charged."
                                    : "Your order is on its way and can no longer be cancelled.");
}

// ============================================================
// 7. Thank-you
// ============================================================
ThankYouPage::ThankYouPage(QWidget *parent) : QScrollArea(parent)
{
    auto *v = ui::beginPage(this);
    v->addWidget(ui::pageHeader("Thank you!", "Your order is on its way. You can close this window."));
    v->addStretch();
}
