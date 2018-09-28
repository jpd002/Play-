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

#ifdef HAS_LIBEVDEV
#include "GamePad/GamePadInputEventListener.h"
#include "GamePad/GamePadDeviceListener.h"
#endif

#include "../FutureContinuationManager.h"

namespace Ui
{
	class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget* parent = nullptr);
	~MainWindow();

private:
	void SetOpenGlPanelSize();
	void CreateStatusBar();
	void InitVirtualMachine();
	void SetupGsHandler();
	void SetupSoundHandler();
	void SetupSaveLoadStateSlots();
	QString SaveStateInfo(int);
	void OnExecutableChange();
	void UpdateUI();
	void RegisterPreferences();
	void BootElf(boost::filesystem::path);
	void BootCDROM();
	void saveState(int);
	void loadState(int);
	void toggleFullscreen();

	Ui::MainWindow* ui;

	QWindow* m_openglpanel = nullptr;
	QLabel* m_fpsLabel = nullptr;
	ElidedLabel* m_msgLabel = nullptr;
	CStatsManager* m_statsManager = nullptr;
	CInputBindingManager* m_InputBindingManager = nullptr;
	QTimer* m_fpsTimer = nullptr;
	QTimer* m_continuationTimer = nullptr;
	CPS2VM* m_virtualMachine = nullptr;
	bool m_deactivatePause = false;
	bool m_pauseFocusLost = true;
	CFutureContinuationManager m_futureContinuationManager;
#ifdef HAS_LIBEVDEV
	std::unique_ptr<CGamePadDeviceListener> m_GPDL;
#endif
	enum BootType
	{
		CD,
		ELF
	};
	struct LastOpenCommand
	{
		LastOpenCommand() = default;
		LastOpenCommand(BootType type, boost::filesystem::path path)
		    : type(type)
		    , path(path)
		{
		}
		BootType type;
		boost::filesystem::path path;
	};
	LastOpenCommand m_lastOpenCommand;
	boost::filesystem::path m_lastPath;

	QString ReadElementValue(QXmlStreamReader& Rxml);

protected:
	void closeEvent(QCloseEvent*) Q_DECL_OVERRIDE;
	void showEvent(QShowEvent*) Q_DECL_OVERRIDE;

public slots:
	void openGLWindow_resized();
	void setFPS();
	void updateContinuations();

private slots:
	void on_actionBoot_DiscImage_triggered();
	void on_actionBoot_ELF_triggered();
	void on_actionExit_triggered();
	void keyPressEvent(QKeyEvent*) Q_DECL_OVERRIDE;
	void keyReleaseEvent(QKeyEvent*) Q_DECL_OVERRIDE;
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
	void on_actionCapture_Screen_triggered();
	void doubleClickEvent(QMouseEvent*);
};

#endif // MAINWINDOW_H
