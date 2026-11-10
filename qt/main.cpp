#include "mainwindow.h"
#include "uihelpers.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setStyle("Fusion");
    a.setApplicationName("PizzaGo");
    a.setOrganizationName("PizzaGo");
    a.setWindowIcon(ui::appIcon());

    MainWindow w;
    w.setMinimumSize(900, 640);
    w.showMaximized();          // full-screen-sized window (press F11 for true full screen)

    return a.exec();
}
