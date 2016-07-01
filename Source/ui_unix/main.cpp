#include <QApplication>

#include "mainwindow.h"
#include <QWindow>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    w.createOpenGlPanel();
    w.initEmu();




    return a.exec();
}

