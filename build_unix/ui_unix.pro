#-------------------------------------------------
#
# Project created by QtCreator 2016-06-12T16:07:55
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Play-Ui
DEFINES += PLAY_VERSION=\\\"0.30\\\"
VERSION = PLAY_VERSION
TEMPLATE = app

INCLUDEPATH +=../ \
              ../Source \
              ../../CodeGen/include \
              ../../Framework/include \

SOURCES += ../Source/ui_unix/main.cpp\
        ../Source/ui_unix/mainwindow.cpp \
        ../Source/ui_unix/GSH_OpenGLQt.cpp \
        ../Source/ui_unix/StatsManager.cpp \
        ../Source/ui_unix/PH_HidUnix.cpp \
        ../Source/ui_unix/settingsdialog.cpp \
        ../Source/ui_unix/openglwindow.cpp \
        ../Source/ui_unix/memorycardmanagerdialog.cpp \
        ../Source/ui_unix/MemoryCard.cpp \
    ../Source/ui_unix/vfsmanagerdialog.cpp \
    ../Source/ui_unix/vfsmodel.cpp \
    ../Source/ui_unix/vfsdiscselectordialog.cpp \
    ../Source/ui_unix/VfsDevice.cpp



HEADERS  += ../Source/ui_unix/mainwindow.h \
            ../Source/ui_unix/GSH_OpenGLQt.h \
            ../Source/ui_unix/StatsManager.h \
            ../Source/ui_unix/PH_HidUnix.h \
            ../Source/ui_unix/settingsdialog.h \
            ../Source/ui_unix/PreferenceDefs.h \
            ../Source/ui_unix/openglwindow.h \
            ../Source/ui_unix/memorycardmanagerdialog.h \
            ../Source/ui_unix/MemoryCard.h \
    ../Source/ui_unix/vfsmanagerdialog.h \
    ../Source/ui_unix/vfsmodel.h \
    ../Source/ui_unix/vfsdiscselectordialog.h \
    ../Source/ui_unix/VfsDevice.h


FORMS    += mainwindow.ui \
    settingsdialog.ui \
    memorycardmanager.ui \
    vfsmanagerdialog.ui \
    vfsdiscselectordialog.ui

RESOURCES     = resources.qrc

unix:!macx: LIBS += -L$$PWD/build/ -lPlay

DEPENDPATH += $$PWD/../Source/

unix:!macx: PRE_TARGETDEPS += $$PWD/build/libPlay.a

unix:!macx: LIBS += -L$$PWD/../../Framework/build_unix/build/ -lFramework

DEPENDPATH += $$PWD/../../Framework/include

unix:!macx: PRE_TARGETDEPS += $$PWD/../../Framework/build_unix/build/libFramework.a

unix:!macx: LIBS += -L$$PWD/../../CodeGen/build_unix/build/ -lCodeGen

DEPENDPATH += $$PWD/../../CodeGen/include

unix:!macx: PRE_TARGETDEPS += $$PWD/../../CodeGen/build_unix/build/libCodeGen.a

LIBS += -lboost_system -lboost_filesystem -lboost_chrono -lGLEW -lz -lbz2 -lopenal -licuuc

QMAKE_CXXFLAGS += -std=c++11
