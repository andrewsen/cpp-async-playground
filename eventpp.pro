TEMPLATE = app
CONFIG += console g++17
CONFIG -= app_bundle
CONFIG -= qt

QMAKE_CXXFLAGS += -std=c++17 -O3

SOURCES += main.cpp \
    looper.cpp \
    threadpool.cpp \
    promise.cpp \
    application.cpp \
    task.cpp

HEADERS += \
    looper.h \
    threadpool.h \
    promise.h \
    application.h \
    task.h

LIBS += -lpthread
