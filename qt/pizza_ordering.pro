QT       += core gui widgets
RESOURCES += resources.qrc

CONFIG   += c++17
TARGET    = pizza_ordering
TEMPLATE  = app

SOURCES += \
    animatedstack.cpp \
    main.cpp \
    mainwindow.cpp \
    mapwidget.cpp \
    pages.cpp \
    steprail.cpp \
    uihelpers.cpp

HEADERS += \
    animatedstack.h \
    mainwindow.h \
    mapwidget.h \
    pages.h \
    steprail.h \
    uihelpers.h
