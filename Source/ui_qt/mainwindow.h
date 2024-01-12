#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QKeyEvent>
#include <QDir>

#include "AppConfig.h"
#include "PS2VM.h"
#include "ElidedLabel.h"
#include "ContinuationChecker.h"

#include "InputProviderQtKey.h"
#include "InputProviderQtMouse.h"
#include "ScreenShotUtils.h"

namespace Ui
{
	class MainWindow;
}

class OutputWindow;

#ifdef DEBUGGER_INCLUDED
class QtDebugger;
class QtFramedebugger;

namespace Ui
{
	class DebugDockMenu;
	class DebugMenu;
}
#endif

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget* parent = nullptr);
	~MainWindow();

	void BootElf(fs::path);
	void BootCDROM();
	void LoadCDROM(fs::path filePath);
	void BootArcadeMachine(fs::path);
	void loadState(int);

#ifdef DEBUGGER_INCLUDED
	void ShowMainWindow();
	void ShowDebugger();
	void ShowFrameDebugger();
	fs::path GetFrameDumpDirectoryPath();
	void DumpNextFrame();
	void ToggleGsDraw();
#endif

private:
	enum class BootType
	{
		CD,
		ELF,
		ARCADE,
	};

	struct LastOpenCommand
	{
		LastOpenCommand() = default;
		LastOpenCommand(BootType type, fs::path path)
		    : type(type)
		    , path(path)
		{
		}
		BootType type = BootType::CD;
		fs::path path;
	};

	void SetOutputWindowSize();
	void CreateStatusBar();
	void InitVirtualMachine();
	void SetupGsHandler();
	void SetupSoundHandler();
	void SetupSaveLoadStateSlots();
	QString GetSaveStateInfo(int);
	void EmitOnExecutableChange();
	bool IsExecutableLoaded() const;
	void UpdateUI();
	void UpdateCpuUsageLabel();
	void RegisterPreferences();
	void saveState(int);
	void buildResizeWindowMenu();
	void resizeWindow(unsigned int, unsigned int);
	void UpdateGSHandlerLabel(int);
	void SetupBootableView();
	void SetupDebugger();

	Ui::MainWindow* ui = nullptr;

	OutputWindow* m_outputwindow = nullptr;
	QLabel* m_fpsLabel = nullptr;
	QLabel* m_cpuUsageLabel = nullptr;
	QLabel* m_gsLabel = nullptr;
#ifdef PROFILE
	QLabel* m_profileStatsLabel = nullptr;
#endif
	ElidedLabel* m_msgLabel = nullptr;
	QTimer* m_fpsTimer = nullptr;
	CContinuationChecker* m_continuationChecker = nullptr;
	CPS2VM* m_virtualMachine = nullptr;
	bool m_deactivatePause = false;
	bool m_pauseFocusLost = true;
	std::shared_ptr<CInputProviderQtKey> m_qtKeyInputProvider;
	std::shared_ptr<CInputProviderQtMouse> m_qtMouseInputProvider;
	LastOpenCommand m_lastOpenCommand;
	fs::path m_lastPath;

	Framework::CSignal<void()>::Connection m_OnExecutableChangeConnection;
	CPS2VM::NewFrameEvent::Connection m_OnNewFrameConnection;
	CGSHandler::NewFrameEvent::Connection m_OnGsNewFrameConnection;
	CScreenShotUtils::Connection m_screenShotCompleteConnection;
	CVirtualMachine::RunningStateChangeEvent::Connection m_onRunningStateChangeConnection;

#ifdef DEBUGGER_INCLUDED
	std::unique_ptr<QtDebugger> m_debugger;
	std::unique_ptr<QtFramedebugger> m_frameDebugger;
	Ui::DebugDockMenu* debugDockMenuUi = nullptr;
	Ui::DebugMenu* debugMenuUi = nullptr;
#endif

protected:
	void closeEvent(QCloseEvent*) Q_DECL_OVERRIDE;
	void changeEvent(QEvent*) Q_DECL_OVERRIDE;

signals:
	void onExecutableChange();

public slots:
	void outputWindow_resized();
	void updateStats();

private slots:
	void on_actionBoot_DiscImage_triggered();
	void on_actionBoot_DiscImage_S3_triggered();
	void on_actionBoot_cdrom0_triggered();
	void on_actionBoot_ELF_triggered();
	void on_actionExit_triggered();
	void keyPressEvent(QKeyEvent*) Q_DECL_OVERRIDE;
	void keyReleaseEvent(QKeyEvent*) Q_DECL_OVERRIDE;
	void on_actionSettings_triggered();
	void on_actionPause_Resume_triggered();
	void on_actionAbout_triggered();
	void focusOutEvent(QFocusEvent*) Q_DECL_OVERRIDE;
	void focusInEvent(QFocusEvent*) Q_DECL_OVERRIDE;
	void outputWindow_doubleClickEvent(QMouseEvent*);
	void outputWindow_mouseMoveEvent(QMouseEvent*);
	void outputWindow_mousePressEvent(QMouseEvent*);
	void outputWindow_mouseReleaseEvent(QMouseEvent*);
	void on_actionPause_when_focus_is_lost_triggered(bool checked);
	void on_actionReset_triggered();
	void on_actionMemory_Card_Manager_triggered();
	void on_actionVFS_Manager_triggered();
	void on_actionController_Manager_triggered();
	void on_actionToggleFullscreen_triggered();
	void on_actionCapture_Screen_triggered();
	void HandleOnExecutableChange();
	void on_actionList_Bootables_triggered();
};

#endif // MAINWINDOW_H
