TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp \
    looper.cpp \
    threadpool.cpp \
    promise.cpp \
    application.cpp

HEADERS += \
    looper.h \
    threadpool.h \
    promise.h \
    application.h

LIBS += -lpthread
