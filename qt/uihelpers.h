#ifndef UIHELPERS_H
#define UIHELPERS_H

#include <QPixmap>
#include <QIcon>
#include <QString>

class QWidget;
class QFrame;
class QVBoxLayout;
class QScrollArea;

namespace ui {

QPixmap logoPixmap(int size);                 // painted PizzaGo logo (no image file needed)
QIcon   appIcon();

// White rounded "card" with an optional title. *body receives the card's layout.
QFrame *card(const QString &title, QVBoxLayout **body);

// Title + subtitle block used at the top of every page.
QWidget *pageHeader(const QString &title, const QString &subtitle = QString());

// Centers `content` and caps its width, so full-screen windows stay readable.
QWidget *centered(QWidget *content, int maxWidth = 1040);

// Prepares a QScrollArea page and returns the vertical layout to fill.
QVBoxLayout *beginPage(QScrollArea *page);

// Little helper: checkable "segmented" button (size / tip / crust pickers).
class QPushButtonHelper;

} // namespace ui

#endif // UIHELPERS_H
