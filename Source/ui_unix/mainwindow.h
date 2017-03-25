#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QKeyEvent>
#include <QXmlStreamReader>
#include <QDir>

#include "AppConfig.h"
#include "PS2VM.h"
#include "PS2VM_Preferences.h"
#include "StatsManager.h"
#include "PH_HidUnix.h"
#include "ElidedLabel.h"


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
    void SetOpenGlPanelSize();
    void CreateStatusBar();
    void InitEmu();
    void SetupSoundHandler();
    void Setupfpscounter();
    void SetupSaveLoadStateSlots();
    QString SaveStateInfo(int);
    boost::filesystem::path GetStateDirectoryPath();
    boost::filesystem::path GenerateStatePath(int);
    void OnRunningStateChange();
    void OnExecutableChange();
    void UpdateUI();
    void RegisterPreferences();
    void BootElf(const char*);
    void BootCDROM();
    void saveState(int);
    void loadState(int);

    Ui::MainWindow *ui;

    QWindow* m_openglpanel;
    QLabel *gameIDLabel;
    QLabel* fpsLabel;
    QLabel* m_dcLabel;
    QLabel* m_stateLabel;
    ElidedLabel* m_msgLabel;
    CStatsManager* StatsManager;
    CPH_HidUnix* m_padhandler = nullptr;
    QTimer *m_fpstimer = nullptr;
    CPS2VM* g_virtualMachine = nullptr;
    bool m_deactivatePause = false;
    bool m_pauseFocusLost = true;
    enum BootType { CD, ELF };
    struct lastOpenCommand
    {
        BootType type;
        const char* filename;

        lastOpenCommand(BootType m_type, const char* m_filename) : type(m_type),filename(m_filename){}

    };
    lastOpenCommand* m_lastOpenCommand = nullptr;
    QString m_lastpath = QDir::homePath();

    QString ReadElementValue(QXmlStreamReader &Rxml);
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
    void on_actionPause_Resume_triggered();
    void on_actionAbout_triggered();
    void focusOutEvent(QFocusEvent*) Q_DECL_OVERRIDE;
    void focusInEvent(QFocusEvent*) Q_DECL_OVERRIDE;
    void on_actionPause_when_focus_is_lost_triggered(bool checked);
    void on_actionReset_triggered();
    void on_actionMemory_Card_Manager_triggered();
    void on_actionVFS_Manager_triggered();
    void on_actionController_Manager_triggered();
};

#endif // MAINWINDOW_H
