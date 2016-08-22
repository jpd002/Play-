#-------------------------------------------------
#
# Project created by QtCreator 2016-06-12T16:07:55
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ui_unix
TEMPLATE = app

INCLUDEPATH +=../ \
              ../Source \
              ../../CodeGen/include \
              ../../Framework/include \

SOURCES += ../Source/ui_unix/main.cpp\
        ../Source/ui_unix/mainwindow.cpp \
        ../Source/ui_unix/GSH_OpenGLUnix.cpp \
        ../Source/ui_unix/StatsManager.cpp \
        ../Source/ui_unix/PH_HidUnix.cpp \
        ../Source/ui_unix/settingsdialog.cpp

HEADERS  += ../Source/ui_unix/mainwindow.h \
            ../Source/ui_unix/GSH_OpenGLUnix.h \
            ../Source/ui_unix/global.h \
            ../Source/ui_unix/StatsManager.h \
            ../Source/ui_unix/PH_HidUnix.h \
            ../Source/ui_unix/settingsdialog.h \
            ../Source/ui_unix/PreferenceDefs.h

FORMS    += mainwindow.ui \
    settingsdialog.ui

unix:!macx: LIBS += -L$$PWD/build/ -lPlay

DEPENDPATH += $$PWD/../Source/

unix:!macx: PRE_TARGETDEPS += $$PWD/build/libPlay.a

unix:!macx: LIBS += -L$$PWD/../../Framework/build_unix/build/ -lFramework

DEPENDPATH += $$PWD/../../Framework/include

unix:!macx: PRE_TARGETDEPS += $$PWD/../../Framework/build_unix/build/libFramework.a

unix:!macx: LIBS += -L$$PWD/../../CodeGen/build_unix/build/ -lCodeGen

DEPENDPATH += $$PWD/../../CodeGen/include

unix:!macx: PRE_TARGETDEPS += $$PWD/../../CodeGen/build_unix/build/libCodeGen.a

LIBS += -lboost_system -lboost_filesystem -lboost_chrono -lX11 -lGLEW -lz -lbz2 -lopenal

QMAKE_CXXFLAGS += -std=c++11
