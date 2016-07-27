#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QKeyEvent>

#include "AppConfig.h"
#include "PS2VM.h"
#include "PS2VM_Preferences.h"
#include "StatsManager.h"
#include "PH_HidUnix.h"


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    void setOpenGlPanelSize();
    void createStatusBar();
    void initEmu();
    void setupSoundHandler();
    void Setupfpscounter();
    void createOpenGlPanel();
    void setupSaveLoadStateSlots();
    QString saveStateInfo(int);
    boost::filesystem::path GetStateDirectoryPath();
    boost::filesystem::path GenerateStatePath(int);
    void OnRunningStateChange();
    void OnExecutableChange();
    void UpdateUI();

    Ui::MainWindow *ui;

    QWindow* openglpanel;
    QLabel* fpsLabel;
    QLabel* dcLabel;
    QLabel* stateLabel;
    CStatsManager* StatsManager;
    CPH_HidUnix* padhandler = nullptr;
    QTimer *fpstimer = nullptr;
    CPS2VM* g_virtualMachine = nullptr;

protected:
    void closeEvent(QCloseEvent*) Q_DECL_OVERRIDE;
    void showEvent(QShowEvent*) Q_DECL_OVERRIDE;

public slots:
    void openGLWindow_resized();
    void setFPS();

private slots:
    void on_actionOpen_Game_triggered();
    void on_actionBoot_ELF_triggered();
    void on_actionExit_triggered();
    void keyPressEvent(QKeyEvent *);
    void keyReleaseEvent(QKeyEvent *);
    void on_actionSettings_triggered();
    void saveState();
    void loadState();
    void on_actionPause_Resume_triggered();
    void on_actionAbout_triggered();
};

#endif // MAINWINDOW_H
