#include "mainwindow.h"
#include "settingsdialog.h"
#include "memorycardmanagerdialog.h"

#include "openglwindow.h"

#include <QDateTime>
#include <QFileDialog>
#include <QTimer>
#include <QWindow>
#include <QMessageBox>
#include <QStorageInfo>

#include "GSH_OpenGLQt.h"
#include "tools/PsfPlayer/Source/SH_OpenAL.h"
#include "DiskUtils.h"
#include "PathUtils.h"
#include <zlib.h>
#include <boost/version.hpp>

#include "PreferenceDefs.h"
#include "ScreenShotUtils.h"

#include "ui_mainwindow.h"
#include "vfsmanagerdialog.h"
#include "ControllerConfig/controllerconfigdialog.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
	ui->setupUi(this);

	m_openglpanel = new OpenGLWindow;
	QWidget* container = QWidget::createWindowContainer(m_openglpanel);
	ui->gridLayout->addWidget(container);
	connect(m_openglpanel, SIGNAL(heightChanged(int)), this, SLOT(openGLWindow_resized()));
	connect(m_openglpanel, SIGNAL(widthChanged(int)), this, SLOT(openGLWindow_resized()));

	connect(m_openglpanel, SIGNAL(keyUp(QKeyEvent*)), this, SLOT(keyReleaseEvent(QKeyEvent*)));
	connect(m_openglpanel, SIGNAL(keyDown(QKeyEvent*)), this, SLOT(keyPressEvent(QKeyEvent*)));

	connect(m_openglpanel, SIGNAL(focusOut(QFocusEvent*)), this, SLOT(focusOutEvent(QFocusEvent*)));
	connect(m_openglpanel, SIGNAL(focusIn(QFocusEvent*)), this, SLOT(focusInEvent(QFocusEvent*)));

	connect(m_openglpanel, SIGNAL(doubleClick(QMouseEvent*)), this, SLOT(doubleClickEvent(QMouseEvent*)));

	RegisterPreferences();

	m_pauseFocusLost = CAppConfig::GetInstance().GetPreferenceBoolean(PREF_UI_PAUSEWHENFOCUSLOST);
	auto last_path = CAppConfig::GetInstance().GetPreferencePath(PREF_PS2_CDROM0_PATH);
	if(boost::filesystem::exists(last_path))
	{
		last_path = last_path.parent_path();
		m_lastpath = QString(last_path.native().c_str());
	}

	CreateStatusBar();
	UpdateUI();

	InitVirtualMachine();
}

MainWindow::~MainWindow()
{
	CAppConfig::GetInstance().Save();
#ifdef HAS_LIBEVDEV
	m_GPDL.reset();
#endif
	if(m_virtualMachine != nullptr)
	{
		m_virtualMachine->Pause();
		m_virtualMachine->DestroyPadHandler();
		m_virtualMachine->DestroyGSHandler();
		m_virtualMachine->DestroySoundHandler();
		m_virtualMachine->Destroy();
		delete m_virtualMachine;
		m_virtualMachine = nullptr;
	}
	delete m_InputBindingManager;
	delete ui;
}

void MainWindow::showEvent(QShowEvent* event)
{
	QMainWindow::showEvent(event);
	SetupGsHandler();
}

void MainWindow::InitVirtualMachine()
{
	assert(!m_virtualMachine);

	m_virtualMachine = new CPS2VM();
	m_virtualMachine->Initialize();

	SetupSoundHandler();

	m_InputBindingManager = new CInputBindingManager(CAppConfig::GetInstance());
	m_virtualMachine->CreatePadHandler(CPH_HidUnix::GetFactoryFunction(m_InputBindingManager));

#ifdef HAS_LIBEVDEV
	auto onInput = [=](std::array<uint32, 6> device, int code, int value, int type, const input_absinfo* abs) -> void {
		if(m_InputBindingManager != nullptr)
		{
			m_InputBindingManager->OnInputEventReceived(device, code, value);
		}
	};
	m_GPDL = std::make_unique<CGamePadDeviceListener>(onInput);
#endif

	m_statsManager = new CStatsManager();

	m_virtualMachine->m_ee->m_os->OnExecutableChange.connect(std::bind(&MainWindow::OnExecutableChange, this));
}

void MainWindow::SetOpenGlPanelSize()
{
	openGLWindow_resized();
}

void MainWindow::SetupGsHandler()
{
	assert(m_virtualMachine);
	auto gsHandler = m_virtualMachine->GetGSHandler();
	if(!gsHandler)
	{
		m_virtualMachine->CreateGSHandler(CGSH_OpenGLQt::GetFactoryFunction(m_openglpanel));
		m_virtualMachine->m_ee->m_gs->OnNewFrame.connect(std::bind(&CStatsManager::OnNewFrame, m_statsManager, std::placeholders::_1));
	}
}

void MainWindow::SetupSoundHandler()
{
	assert(m_virtualMachine);
	bool audioEnabled = CAppConfig::GetInstance().GetPreferenceBoolean(PREFERENCE_AUDIO_ENABLEOUTPUT);
	if(audioEnabled)
	{
		m_virtualMachine->CreateSoundHandler(&CSH_OpenAL::HandlerFactory);
	}
	else
	{
		m_virtualMachine->DestroySoundHandler();
	}
}

void MainWindow::openGLWindow_resized()
{
	if(m_virtualMachine != nullptr && m_virtualMachine->m_ee != nullptr && m_virtualMachine->m_ee->m_gs != nullptr)
	{
		GLint w = m_openglpanel->size().width(), h = m_openglpanel->size().height();

		auto scale = devicePixelRatioF();
		CGSHandler::PRESENTATION_PARAMS presentationParams;
		presentationParams.mode = (CGSHandler::PRESENTATION_MODE)CAppConfig::GetInstance().GetPreferenceInteger(PREF_CGSHANDLER_PRESENTATION_MODE);
		presentationParams.windowWidth = w * scale;
		presentationParams.windowHeight = h * scale;
		m_virtualMachine->m_ee->m_gs->SetPresentationParams(presentationParams);
		m_virtualMachine->m_ee->m_gs->Flip();
	}
}

void MainWindow::on_actionOpen_Game_triggered()
{
	QFileDialog dialog(this);
	dialog.setDirectory(m_lastpath);
	dialog.setFileMode(QFileDialog::ExistingFile);
	dialog.setNameFilter(tr("All supported types(*.iso *.bin *.isz *.cso);;UltraISO Compressed Disk Images (*.isz);;CISO Compressed Disk Images (*.cso);;All files (*.*)"));
	if(dialog.exec())
	{
		auto fileName = dialog.selectedFiles().first();
		m_lastpath = QFileInfo(fileName).path();
		CAppConfig::GetInstance().SetPreferencePath(PREF_PS2_CDROM0_PATH, fileName.toStdString());

		if(m_virtualMachine != nullptr)
		{
			try
			{
				m_lastOpenCommand = lastOpenCommand(BootType::CD, fileName.toStdString());
				BootCDROM();
			}
			catch(const std::exception& e)
			{
				QMessageBox messageBox;
				messageBox.critical(nullptr, "Error", e.what());
				messageBox.show();
			}
		}
	}
}

void MainWindow::on_actionBoot_ELF_triggered()
{
	QFileDialog dialog(this);
	dialog.setDirectory(m_lastpath);
	dialog.setFileMode(QFileDialog::ExistingFile);
	dialog.setNameFilter(tr("ELF files (*.elf)"));
	if(dialog.exec())
	{
		auto fileName = dialog.selectedFiles().first();
		m_lastpath = QFileInfo(fileName).path();
		if(m_virtualMachine != nullptr)
		{
			try
			{
				m_lastOpenCommand = lastOpenCommand(BootType::ELF, fileName.toStdString());
				BootElf(fileName.toStdString().c_str());
			}
			catch(const std::exception& e)
			{
				QMessageBox messageBox;
				messageBox.critical(nullptr, "Error", e.what());
				messageBox.show();
			}
		}
	}
}

void MainWindow::BootElf(const char* fileName)
{
	m_virtualMachine->Pause();
	m_virtualMachine->Reset();
	m_virtualMachine->m_ee->m_os->BootFromFile(fileName);
	m_virtualMachine->Resume();
}

void MainWindow::BootCDROM()
{
	m_virtualMachine->Pause();
	m_virtualMachine->Reset();
	m_virtualMachine->m_ee->m_os->BootFromCDROM();
	m_virtualMachine->Resume();
}

void MainWindow::on_actionExit_triggered()
{
	close();
}

void MainWindow::keyPressEvent(QKeyEvent* event)
{
	if(event->key() != Qt::Key_Escape && m_InputBindingManager != nullptr)
	{
		m_InputBindingManager->OnInputEventReceived({0}, event->key(), 1);
	}
}

void MainWindow::keyReleaseEvent(QKeyEvent* event)
{
	if(event->key() == Qt::Key_Escape)
	{
		if(isFullScreen())
		{
			toggleFullscreen();
		}
		return;
	}
	if(m_InputBindingManager != nullptr)
	{
		m_InputBindingManager->OnInputEventReceived({0}, event->key(), 0);
	}
}

void MainWindow::CreateStatusBar()
{
	m_fpsLabel = new QLabel("");
	m_fpsLabel->setAlignment(Qt::AlignHCenter);
	m_fpsLabel->setMinimumSize(m_fpsLabel->sizeHint());

	m_msgLabel = new ElidedLabel();
	m_msgLabel->setAlignment(Qt::AlignLeft);
	QFontMetrics fm(m_msgLabel->font());
	m_msgLabel->setMinimumSize(fm.boundingRect("...").size());
	m_msgLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);

	statusBar()->addWidget(m_msgLabel, 1);
	statusBar()->addWidget(m_fpsLabel);

	m_fpsTimer = new QTimer(this);
	connect(m_fpsTimer, SIGNAL(timeout()), this, SLOT(setFPS()));
	m_fpsTimer->start(1000);
}

void MainWindow::setFPS()
{
	uint32 frames = m_statsManager->GetFrames();
	uint32 drawCalls = m_statsManager->GetDrawCalls();
	uint32 dcpf = (frames != 0) ? (drawCalls / frames) : 0;
	m_statsManager->ClearStats();
	m_fpsLabel->setText(QString("%1 f/s, %2 dc/f").arg(frames).arg(dcpf));
}

void MainWindow::on_actionSettings_triggered()
{
	SettingsDialog sd;
	sd.exec();
	SetupSoundHandler();
}

void MainWindow::SetupSaveLoadStateSlots()
{
	bool enable = (m_virtualMachine != nullptr ? (m_virtualMachine->m_ee->m_os->GetELF() != nullptr) : false);
	ui->menuSave_States->clear();
	ui->menuLoad_States->clear();
	for(int i = 1; i <= 10; i++)
	{
		QString info = enable ? SaveStateInfo(i) : "Empty";

		QAction* saveaction = new QAction(this);
		saveaction->setText(QString("Save Slot %1 - %2").arg(i).arg(info));
		saveaction->setEnabled(enable);
		ui->menuSave_States->addAction(saveaction);

		QAction* loadaction = new QAction(this);
		loadaction->setText(QString("Load Slot %1 - %2").arg(i).arg(info));
		loadaction->setEnabled(enable);
		ui->menuLoad_States->addAction(loadaction);

		if(enable)
		{
			connect(saveaction, &QAction::triggered, std::bind(&MainWindow::saveState, this, i));
			connect(loadaction, &QAction::triggered, std::bind(&MainWindow::loadState, this, i));
		}
	}
}

void MainWindow::saveState(int stateSlot)
{
	Framework::PathUtils::EnsurePathExists(CPS2VM::GetStateDirectoryPath());

	auto stateFilePath = m_virtualMachine->GenerateStatePath(stateSlot);
	m_virtualMachine->SaveState(stateFilePath);

	QDateTime* dt = new QDateTime;
	QString datetime = dt->currentDateTime().toString("hh:mm dd.MM.yyyy");
	ui->menuSave_States->actions().at(stateSlot - 1)->setText(QString("Save Slot %1 - %2").arg(stateSlot).arg(datetime));
	ui->menuLoad_States->actions().at(stateSlot - 1)->setText(QString("Load Slot %1 - %2").arg(stateSlot).arg(datetime));
}

void MainWindow::loadState(int stateSlot)
{
	auto stateFilePath = m_virtualMachine->GenerateStatePath(stateSlot);
	m_virtualMachine->LoadState(stateFilePath);
	m_virtualMachine->Resume();
}

QString MainWindow::SaveStateInfo(int stateSlot)
{
	auto stateFilePath = m_virtualMachine->GenerateStatePath(stateSlot);
	QFileInfo file(stateFilePath.string().c_str());
	if(file.exists() && file.isFile())
	{
		return file.created().toString("hh:mm dd.MM.yyyy");
	}
	else
	{
		return "Empty";
	}
}

void MainWindow::on_actionPause_Resume_triggered()
{
	if(m_virtualMachine != nullptr)
	{
		if(m_virtualMachine->GetStatus() == CVirtualMachine::PAUSED)
		{
			m_msgLabel->setText("Virtual machine resumed.");
			m_virtualMachine->Resume();
		}
		else
		{
			m_msgLabel->setText("Virtual machine paused.");
			m_virtualMachine->Pause();
		}
	}
}

void MainWindow::closeEvent(QCloseEvent* event)
{
	QMessageBox::StandardButton resBtn = QMessageBox::question(this, "Close Confirmation?",
	                                                           tr("Are you sure you want to exit?\nHave you saved your progress?\n"),
	                                                           QMessageBox::Yes | QMessageBox::No,
	                                                           QMessageBox::Yes);
	if(resBtn != QMessageBox::Yes)
	{
		event->ignore();
	}
	else
	{
		event->accept();
	}
}

void MainWindow::on_actionAbout_triggered()
{
	QMessageBox messageBox;
	messageBox.setIconPixmap(QPixmap(":/assets/app_icon.png"));
	QString about("Version %1 (%2)\nQt v%3 - zlib v%4 - boost v%5");
	QString ver("%1.%2.%3"), boostver, qtver;
	boostver = ver.arg(BOOST_VERSION / 100000).arg(BOOST_VERSION / 100 % 1000).arg(BOOST_VERSION % 100);
	messageBox.about(this, this->windowTitle(), about.arg(QString(PLAY_VERSION), __DATE__, QT_VERSION_STR, ZLIB_VERSION, boostver));
	messageBox.show();
}

void MainWindow::OnExecutableChange()
{
	UpdateUI();
	auto titleString = QString("Play! - [ %1 ]").arg(m_virtualMachine->m_ee->m_os->GetExecutableName());
	setWindowTitle(titleString);
}

void MainWindow::UpdateUI()
{
	ui->actionPause_when_focus_is_lost->setChecked(m_pauseFocusLost);
	ui->actionReset->setEnabled(!m_lastOpenCommand.filename.empty());
	SetOpenGlPanelSize();
	SetupSaveLoadStateSlots();
}

void MainWindow::RegisterPreferences()
{
	CAppConfig::GetInstance().RegisterPreferenceBoolean(PREFERENCE_AUDIO_ENABLEOUTPUT, true);
	CAppConfig::GetInstance().RegisterPreferenceBoolean(PREF_UI_PAUSEWHENFOCUSLOST, true);
}

void MainWindow::focusOutEvent(QFocusEvent* event)
{
	if(m_pauseFocusLost && m_virtualMachine->GetStatus() == CVirtualMachine::RUNNING)
	{
		if(!isActiveWindow() && !m_openglpanel->isActive())
		{
			if(m_virtualMachine != nullptr)
			{
				m_virtualMachine->Pause();
				m_deactivatePause = true;
			}
		}
	}
}
void MainWindow::focusInEvent(QFocusEvent* event)
{
	if(m_pauseFocusLost && m_virtualMachine->GetStatus() == CVirtualMachine::PAUSED)
	{
		if(m_deactivatePause && (isActiveWindow() || m_openglpanel->isActive()))
		{
			if(m_virtualMachine != nullptr)
			{
				m_virtualMachine->Resume();
				m_deactivatePause = false;
			}
		}
	}
}

void MainWindow::doubleClickEvent(QMouseEvent* ev)
{
	if(ev->button() == Qt::LeftButton)
	{
		toggleFullscreen();
	}
}

void MainWindow::toggleFullscreen()
{
	if(isFullScreen())
	{
		showNormal();
		statusBar()->show();
		menuBar()->show();
	}
	else
	{
		showFullScreen();
		statusBar()->hide();
		menuBar()->hide();
	}
}
void MainWindow::on_actionPause_when_focus_is_lost_triggered(bool checked)
{
	m_pauseFocusLost = checked;
	CAppConfig::GetInstance().SetPreferenceBoolean(PREF_UI_PAUSEWHENFOCUSLOST, m_pauseFocusLost);
}

void MainWindow::on_actionReset_triggered()
{
	if(!m_lastOpenCommand.filename.empty())
	{
		if(m_lastOpenCommand.type == BootType::CD)
		{
			BootCDROM();
		}
		else if(m_lastOpenCommand.type == BootType::ELF)
		{
			BootElf(m_lastOpenCommand.filename.c_str());
		}
	}
}

void MainWindow::on_actionMemory_Card_Manager_triggered()
{
	MemoryCardManagerDialog mcm;
	mcm.exec();
}

void MainWindow::on_actionVFS_Manager_triggered()
{
	VFSManagerDialog vfsmgr;
	vfsmgr.exec();
}

void MainWindow::on_actionController_Manager_triggered()
{
	ControllerConfigDialog ccd;
	ccd.SetInputBindingManager(m_InputBindingManager);
	ccd.exec();
}

void MainWindow::on_actionCapture_Screen_triggered()
{
	CScreenShotUtils::TriggerGetScreenshot(m_virtualMachine,
	                                       [&](int res, const char* msg) -> void {
		                                       m_msgLabel->setText(msg);
	                                       });
}
