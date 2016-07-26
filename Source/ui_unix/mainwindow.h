#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QKeyEvent>

#include "AppConfig.h"
#include "PS2VM_Preferences.h"


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    WId getOpenGLPanel();
    void setOpenGlPanelSize();
    void initEmu();

public slots:
    void on_openGLWidget_resized();

private slots:
    void on_actionOpen_Game_triggered();

    void on_actionStart_Game_triggered();

    void on_actionBoot_ELF_triggered();

    void on_actionExit_triggered();
protected:

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
