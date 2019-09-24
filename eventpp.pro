TEMPLATE = app
CONFIG += console g++17
CONFIG -= app_bundle
CONFIG -= qt

QMAKE_CXXFLAGS += -std=c++17 -O3 -fPIC -Wall -pedantic -Wall -Wextra -Wcast-align \
    -Wcast-qual -Wctor-dtor-privacy -Wdisabled-optimization -Wformat=2 -Winit-self \
    -Wlogical-op -Wmissing-declarations -Wmissing-include-dirs -Wnoexcept \
    -Woverloaded-virtual -Wredundant-decls -Wshadow -Wsign-conversion \
    -Wsign-promo -Wstrict-null-sentinel -Wstrict-overflow=5 -Wswitch-default -Wundef -Wunused

SOURCES += main.cpp \
    looper.cpp \
    threadpool.cpp \
    application.cpp \
    task.cpp \
    taskqueue.cpp

HEADERS += \
    looper.h \
    threadpool.h \
    promise.h \
    application.h \
    task.h \
    event.h \
    threadpoolbase.h \
    taskqueue.h

LIBS += -lpthread
