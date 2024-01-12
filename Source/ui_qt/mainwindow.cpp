#include "mainwindow.h"
#include "QStringUtils.h"
#include "settingsdialog.h"
#include "memorycardmanagerdialog.h"
#ifdef HAS_AMAZON_S3
#include "S3FileBrowser.h"
#endif
#include "ui_shared/ArcadeUtils.h"
#include "ui_shared/BootablesProcesses.h"
#include "ui_shared/StatsManager.h"
#include "QtUtils.h"

#include "openglwindow.h"
#include "GSH_OpenGLQt.h"

#ifdef HAS_GSH_VULKAN
#include "vulkanwindow.h"
#include "GSH_VulkanQt.h"
#include "gs/GSH_Vulkan/GSH_VulkanDeviceInfo.h"
#endif

#include <ctime>

#include <QDateTime>
#include <QFileDialog>
#include <QTimer>
#include <QWindow>
#include <QMessageBox>
#include <QStorageInfo>
#include <QtGlobal>
#include <QCheckBox>

#include "StdStreamUtils.h"
#include "string_format.h"

#ifdef _WIN32
#include "../../tools/PsfPlayer/Source/ui_win32/SH_WaveOut.h"
#else
#include "tools/PsfPlayer/Source/SH_OpenAL.h"
#endif
#ifdef DEBUGGER_INCLUDED
#include "DebugSupport/DebugSupportSettings.h"
#include "DebugSupport/QtDebugger.h"
#include "DebugSupport/FrameDebugger/QtFramedebugger.h"
#include "ui_debugdockmenu.h"
#include "ui_debugmenu.h"
#endif
#include "input/PH_GenericInput.h"
#include "DiskUtils.h"
#include "PathUtils.h"
#include <zstd_zlibwrapper.h>

#include "CoverUtils.h"
#include "PreferenceDefs.h"
#include "PS2VM_Preferences.h"
#include "ScreenShotUtils.h"

#include "ui_mainwindow.h"
#include "vfsmanagerdialog.h"
#include "ControllerConfig/controllerconfigdialog.h"
#include "QBootablesView.h"

#ifdef __APPLE__
#include "macos/InputProviderMacOsHid.h"
#endif
#ifdef HAS_LIBEVDEV
#include "unix/InputProviderEvDev.h"
#endif
#ifdef WIN32
#include "win32/InputProviderDirectInput.h"
#include "win32/InputProviderXInput.h"
#endif

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
	ArcadeUtils::RegisterArcadeMachines();

	ui->setupUi(this);
	buildResizeWindowMenu();

	RegisterPreferences();

	m_continuationChecker = new CContinuationChecker(this);

#ifdef PROFILE
	{
		m_profileStatsLabel = new QLabel(this);
		QFont courierFont("Courier");
		m_profileStatsLabel->setFont(courierFont);
		m_profileStatsLabel->setAlignment(Qt::AlignTop);
		ui->gridLayout->addWidget(m_profileStatsLabel, 0, 1);
	}
#endif

	m_pauseFocusLost = CAppConfig::GetInstance().GetPreferenceBoolean(PREF_UI_PAUSEWHENFOCUSLOST);
	auto lastPath = CAppConfig::GetInstance().GetPreferencePath(PREF_PS2_CDROM0_PATH);
	std::error_code lastPathExistsErrorCode;
	if(fs::exists(lastPath, lastPathExistsErrorCode))
	{
		m_lastPath = lastPath.parent_path();
	}
	else
	{
		m_lastPath = QStringToPath(QDir::homePath());
	}

	CreateStatusBar();
	UpdateUI();
	SetupBootableView();

	//Add actions to window to make sure they can be activated with shortcuts in fullscreen mode.
	addAction(ui->actionPause_Resume);
	addAction(ui->actionToggleFullscreen);

#ifdef WIN32
	ui->actionToggleFullscreen->setShortcut(QKeySequence(Qt::ALT + Qt::Key_Return));
#endif

#ifdef HAS_AMAZON_S3
	ui->actionBoot_DiscImage_S3->setVisible(S3FileBrowser::IsAvailable());
#endif

	InitVirtualMachine();
	SetupGsHandler();
	SetupDebugger();

	m_onRunningStateChangeConnection = m_virtualMachine->OnRunningStateChange.Connect([&] {
		if(m_virtualMachine->GetStatus() == CVirtualMachine::RUNNING)
			ui->stackedWidget->setCurrentIndex(1);
	});
}

MainWindow::~MainWindow()
{
#ifdef DEBUGGER_INCLUDED
	m_debugger.reset();
	m_frameDebugger.reset();
#endif
	CAppConfig::GetInstance().Save();
	if(m_virtualMachine != nullptr)
	{
		m_virtualMachine->Pause();
		m_qtKeyInputProvider.reset();
		m_qtMouseInputProvider.reset();
		m_virtualMachine->DestroyPadHandler();
		m_virtualMachine->DestroyGSHandler();
		m_virtualMachine->DestroySoundHandler();
		m_virtualMachine->Destroy();
		delete m_virtualMachine;
		m_virtualMachine = nullptr;
	}
	delete ui;
#ifdef DEBUGGER_INCLUDED
	delete debugDockMenuUi;
	delete debugMenuUi;
#endif
}

void MainWindow::InitVirtualMachine()
{
	assert(!m_virtualMachine);

	m_virtualMachine = new CPS2VM();
	m_virtualMachine->Initialize();

	SetupSoundHandler();

	{
		m_virtualMachine->CreatePadHandler(CPH_GenericInput::GetFactoryFunction());
		auto padHandler = static_cast<CPH_GenericInput*>(m_virtualMachine->GetPadHandler());
		auto profile = CAppConfig::GetInstance().GetPreferenceString(PREF_INPUT_PAD1_PROFILE);

		auto& bindingManager = padHandler->GetBindingManager();
		bindingManager.Load(profile);

		//Create QtKeyInputProvider
		m_qtKeyInputProvider = std::make_shared<CInputProviderQtKey>();
		m_qtMouseInputProvider = std::make_shared<CInputProviderQtMouse>();
		bindingManager.RegisterInputProvider(m_qtKeyInputProvider);
		bindingManager.RegisterInputProvider(m_qtMouseInputProvider);
#ifdef __APPLE__
		bindingManager.RegisterInputProvider(std::make_shared<CInputProviderMacOsHid>());
#endif
#ifdef HAS_LIBEVDEV
		bindingManager.RegisterInputProvider(std::make_shared<CInputProviderEvDev>());
#endif
#ifdef WIN32
		bindingManager.RegisterInputProvider(std::make_shared<CInputProviderDirectInput>());
		bindingManager.RegisterInputProvider(std::make_shared<CInputProviderXInput>());
#endif
		if(!bindingManager.HasBindings())
		{
			ControllerConfigDialog::AutoConfigureKeyboard(0, &bindingManager);
		}
	}

	//OnExecutableChange might be called from another thread, we need to wrap it around a Qt signal
	m_OnExecutableChangeConnection = m_virtualMachine->m_ee->m_os->OnExecutableChange.Connect(std::bind(&MainWindow::EmitOnExecutableChange, this));
	connect(this, SIGNAL(onExecutableChange()), this, SLOT(HandleOnExecutableChange()));
}

void MainWindow::SetOutputWindowSize()
{
	outputWindow_resized();
}

void MainWindow::SetupGsHandler()
{
	assert(m_virtualMachine);
	switch(CAppConfig::GetInstance().GetPreferenceInteger(PREF_VIDEO_GS_HANDLER))
	{
#ifdef HAS_GSH_VULKAN
	case SettingsDialog::GS_HANDLERS::VULKAN:
	{
		assert(GSH_Vulkan::CDeviceInfo::GetInstance().HasAvailableDevices());
		m_outputwindow = new VulkanWindow;
		QWidget* container = QWidget::createWindowContainer(m_outputwindow);
		m_outputwindow->create();
		ui->stackedWidget->addWidget(container);
		m_virtualMachine->CreateGSHandler(CGSH_VulkanQt::GetFactoryFunction(m_outputwindow));
	}
	break;
	case SettingsDialog::GS_HANDLERS::OPENGL:
#endif
	default:
	{
		m_outputwindow = new OpenGLWindow;
		QWidget* container = QWidget::createWindowContainer(m_outputwindow);
		m_outputwindow->create();
		ui->stackedWidget->addWidget(container);
		m_virtualMachine->CreateGSHandler(CGSH_OpenGLQt::GetFactoryFunction(m_outputwindow));
	}
	}
	if(ui->stackedWidget->count() > 2)
	{
		auto oldContainer = ui->stackedWidget->widget(1);
		ui->stackedWidget->removeWidget(oldContainer);
		delete oldContainer;
	}

	connect(m_outputwindow, SIGNAL(heightChanged(int)), this, SLOT(outputWindow_resized()));
	connect(m_outputwindow, SIGNAL(widthChanged(int)), this, SLOT(outputWindow_resized()));

	connect(m_outputwindow, SIGNAL(keyUp(QKeyEvent*)), this, SLOT(keyReleaseEvent(QKeyEvent*)));
	connect(m_outputwindow, SIGNAL(keyDown(QKeyEvent*)), this, SLOT(keyPressEvent(QKeyEvent*)));

	connect(m_outputwindow, SIGNAL(focusOut(QFocusEvent*)), this, SLOT(focusOutEvent(QFocusEvent*)));
	connect(m_outputwindow, SIGNAL(focusIn(QFocusEvent*)), this, SLOT(focusInEvent(QFocusEvent*)));

	connect(m_outputwindow, SIGNAL(doubleClick(QMouseEvent*)), this, SLOT(outputWindow_doubleClickEvent(QMouseEvent*)));
	connect(m_outputwindow, SIGNAL(mouseMove(QMouseEvent*)), this, SLOT(outputWindow_mouseMoveEvent(QMouseEvent*)));
	connect(m_outputwindow, SIGNAL(mousePress(QMouseEvent*)), this, SLOT(outputWindow_mousePressEvent(QMouseEvent*)));
	connect(m_outputwindow, SIGNAL(mouseRelease(QMouseEvent*)), this, SLOT(outputWindow_mouseReleaseEvent(QMouseEvent*)));

	m_OnNewFrameConnection = m_virtualMachine->OnNewFrame.Connect(std::bind(&CStatsManager::OnNewFrame, &CStatsManager::GetInstance(), m_virtualMachine));
	m_OnGsNewFrameConnection = m_virtualMachine->m_ee->m_gs->OnNewFrame.Connect(std::bind(&CStatsManager::OnGsNewFrame, &CStatsManager::GetInstance(), std::placeholders::_1));
}

void MainWindow::SetupSoundHandler()
{
	assert(m_virtualMachine);
	bool audioEnabled = CAppConfig::GetInstance().GetPreferenceBoolean(PREFERENCE_AUDIO_ENABLEOUTPUT);
	if(audioEnabled)
	{
#ifdef _WIN32
		m_virtualMachine->CreateSoundHandler(&CSH_WaveOut::HandlerFactory);
#else
		m_virtualMachine->CreateSoundHandler(&CSH_OpenAL::HandlerFactory);
#endif
	}
	else
	{
		m_virtualMachine->DestroySoundHandler();
	}
}

void MainWindow::outputWindow_resized()
{
	if(m_virtualMachine != nullptr && m_virtualMachine->m_ee != nullptr && m_virtualMachine->m_ee->m_gs != nullptr)
	{
		uint32 w = m_outputwindow->size().width(), h = m_outputwindow->size().height();

		qreal scale = 1.0;
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
		scale = devicePixelRatioF();
#endif
		CGSHandler::PRESENTATION_PARAMS presentationParams;
		presentationParams.mode = static_cast<CGSHandler::PRESENTATION_MODE>(CAppConfig::GetInstance().GetPreferenceInteger(PREF_CGSHANDLER_PRESENTATION_MODE));
		presentationParams.windowWidth = w * scale;
		presentationParams.windowHeight = h * scale;
		m_virtualMachine->m_ee->m_gs->SetPresentationParams(presentationParams);
		if(m_virtualMachine->GetStatus() == CVirtualMachine::PAUSED)
		{
			m_virtualMachine->m_ee->m_gs->Flip(CGSHandler::FLIP_FLAG_WAIT | CGSHandler::FLIP_FLAG_FORCE);
		}
	}
}

void MainWindow::on_actionBoot_DiscImage_triggered()
{
	QStringList filters;
	filters.push_back(QtUtils::GetDiscImageFormatsFilter());
	filters.push_back("All files (*)");

	QFileDialog dialog(this);
	dialog.setDirectory(PathToQString(m_lastPath));
	dialog.setFileMode(QFileDialog::ExistingFile);
	dialog.setNameFilters(filters);
	if(dialog.exec())
	{
		auto filePath = QStringToPath(dialog.selectedFiles().first());
		if(m_virtualMachine != nullptr)
		{
			try
			{
				LoadCDROM(filePath);
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

void MainWindow::on_actionBoot_DiscImage_S3_triggered()
{
#ifdef HAS_AMAZON_S3
	S3FileBrowser browser(this);
	if(browser.exec())
	{
		auto filePath = browser.GetSelectedPath();
		if(m_virtualMachine != nullptr)
		{
			try
			{
				LoadCDROM(filePath);
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
#endif
}

void MainWindow::on_actionBoot_cdrom0_triggered()
{
	try
	{
		BootCDROM();
	}
	catch(const std::exception& e)
	{
		QMessageBox messageBox;
		messageBox.critical(nullptr, "Error", e.what());
		messageBox.show();
	}
}

void MainWindow::on_actionBoot_ELF_triggered()
{
	QFileDialog dialog(this);
	dialog.setDirectory(PathToQString(m_lastPath));
	dialog.setFileMode(QFileDialog::ExistingFile);
	dialog.setNameFilter(tr("ELF files (*.elf)"));
	if(dialog.exec())
	{
		auto filePath = QStringToPath(dialog.selectedFiles().first());
		m_lastPath = filePath.parent_path();
		if(m_virtualMachine != nullptr)
		{
			try
			{
				BootElf(filePath);
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

void MainWindow::BootElf(fs::path filePath)
{
	m_virtualMachine->Pause();
	m_virtualMachine->Reset();
	m_virtualMachine->m_ee->m_os->BootFromFile(filePath);
#ifndef DEBUGGER_INCLUDED
	m_virtualMachine->Resume();
#endif
	{
		TryRegisterBootable(filePath);
		TryUpdateLastBootedTime(filePath);
		m_lastOpenCommand = LastOpenCommand(BootType::ELF, filePath);
		UpdateUI();
	}
	m_msgLabel->setText(QString("Loaded executable '%1'.")
	                        .arg(m_virtualMachine->m_ee->m_os->GetExecutableName()));
}

void MainWindow::LoadCDROM(fs::path filePath)
{
	m_lastPath = filePath.parent_path();
	CAppConfig::GetInstance().SetPreferencePath(PREF_PS2_CDROM0_PATH, filePath);
}

void MainWindow::BootCDROM()
{
	m_virtualMachine->Pause();
	m_virtualMachine->Reset();
	m_virtualMachine->m_ee->m_os->BootFromCDROM();
#ifndef DEBUGGER_INCLUDED
	m_virtualMachine->Resume();
#endif
	{
		auto filePath = CAppConfig::GetInstance().GetPreferencePath(PREF_PS2_CDROM0_PATH);
		TryRegisterBootable(filePath);
		TryUpdateLastBootedTime(filePath);
		m_lastOpenCommand = LastOpenCommand(BootType::CD, filePath);
		UpdateUI();
	}
	m_msgLabel->setText(QString("Loaded executable '%1' from cdrom0.")
	                        .arg(m_virtualMachine->m_ee->m_os->GetExecutableName()));
}

void MainWindow::BootArcadeMachine(fs::path arcadeDefPath)
{
	try
	{
		ArcadeUtils::BootArcadeMachine(m_virtualMachine, arcadeDefPath);
		m_lastOpenCommand = LastOpenCommand(BootType::ARCADE, arcadeDefPath);
		m_msgLabel->setText(QString("Started arcade machine '%1'.").arg(arcadeDefPath.filename().c_str()));
		UpdateUI();
	}
	catch(const std::exception& e)
	{
		QMessageBox messageBox;
		messageBox.critical(nullptr, "Error", e.what());
		messageBox.show();
	}
}

void MainWindow::on_actionExit_triggered()
{
	close();
}

void MainWindow::keyPressEvent(QKeyEvent* event)
{
	if((event->key() != Qt::Key_Escape) && m_qtKeyInputProvider)
	{
		m_qtKeyInputProvider->OnKeyPress(event->key());
	}
}

void MainWindow::keyReleaseEvent(QKeyEvent* event)
{
	if(event->key() == Qt::Key_Escape)
	{
		if(isFullScreen())
		{
			on_actionToggleFullscreen_triggered();
		}
		return;
	}
	if(m_qtKeyInputProvider)
	{
		m_qtKeyInputProvider->OnKeyRelease(event->key());
	}
}

void MainWindow::CreateStatusBar()
{
	m_fpsLabel = new QLabel("");
	m_fpsLabel->setAlignment(Qt::AlignHCenter);
	m_fpsLabel->setMinimumSize(m_fpsLabel->sizeHint());

	m_cpuUsageLabel = new QLabel("");
	m_cpuUsageLabel->setAlignment(Qt::AlignHCenter);
	m_cpuUsageLabel->setMinimumSize(m_cpuUsageLabel->sizeHint());

	m_msgLabel = new ElidedLabel();
	m_msgLabel->setAlignment(Qt::AlignLeft);
	QFontMetrics fm(m_msgLabel->font());
	m_msgLabel->setMinimumSize(fm.boundingRect("...").size());
	m_msgLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);

	statusBar()->addWidget(m_msgLabel, 1);
	statusBar()->addWidget(m_fpsLabel);
	statusBar()->addWidget(m_cpuUsageLabel);
#ifdef HAS_GSH_VULKAN
	if(GSH_Vulkan::CDeviceInfo::GetInstance().HasAvailableDevices())
	{
		m_gsLabel = new QLabel("");
		auto gs_index = CAppConfig::GetInstance().GetPreferenceInteger(PREF_VIDEO_GS_HANDLER);
		UpdateGSHandlerLabel(gs_index);

		m_gsLabel->setAlignment(Qt::AlignHCenter);
		m_gsLabel->setMinimumSize(m_gsLabel->sizeHint());
		//QLabel have no click event, so we're using ContextMenu event aka rightClick to toggle GS
		m_gsLabel->setContextMenuPolicy(Qt::CustomContextMenu);
		connect(m_gsLabel, &QLabel::customContextMenuRequested, [&]() {
			auto gs_index = CAppConfig::GetInstance().GetPreferenceInteger(PREF_VIDEO_GS_HANDLER);
			gs_index = (gs_index + 1) % SettingsDialog::GS_HANDLERS::MAX_HANDLER;
			CAppConfig::GetInstance().SetPreferenceInteger(PREF_VIDEO_GS_HANDLER, gs_index);
			SetupGsHandler();
			UpdateGSHandlerLabel(gs_index);
		});
		statusBar()->addWidget(m_gsLabel);
	}
#endif
	m_msgLabel->setText(QString("Play! v%1 - %2").arg(PLAY_VERSION).arg(__DATE__));

	m_fpsTimer = new QTimer(this);
	connect(m_fpsTimer, SIGNAL(timeout()), this, SLOT(updateStats()));
	m_fpsTimer->start(1000);

	UpdateCpuUsageLabel();
}

void MainWindow::updateStats()
{
	uint32 frames = CStatsManager::GetInstance().GetFrames();
	uint32 drawCalls = CStatsManager::GetInstance().GetDrawCalls();
	auto cpuUtilisation = CStatsManager::GetInstance().GetCpuUtilisationInfo();
	uint32 dcpf = (frames != 0) ? (drawCalls / frames) : 0;
#ifdef PROFILE
	m_profileStatsLabel->setText(QString::fromStdString(CStatsManager::GetInstance().GetProfilingInfo()));
#endif
	m_fpsLabel->setText(QString("%1 f/s, %2 dc/f").arg(frames).arg(dcpf));

	auto eeUsageRatio = CStatsManager::ComputeCpuUsageRatio(cpuUtilisation.eeIdleTicks, cpuUtilisation.eeTotalTicks);
	m_cpuUsageLabel->setText(QString("EE CPU: %1%").arg(static_cast<int>(eeUsageRatio)));

	CStatsManager::GetInstance().ClearStats();
}

void MainWindow::on_actionSettings_triggered()
{
	auto gs_index = CAppConfig::GetInstance().GetPreferenceInteger(PREF_VIDEO_GS_HANDLER);
	SettingsDialog sd;
	sd.exec();
	SetupSoundHandler();
	if(m_virtualMachine != nullptr)
	{
		m_virtualMachine->ReloadSpuBlockCount();
		m_virtualMachine->ReloadFrameRateLimit();
		UpdateCpuUsageLabel();
		auto new_gs_index = CAppConfig::GetInstance().GetPreferenceInteger(PREF_VIDEO_GS_HANDLER);
		if(gs_index != new_gs_index)
		{
			UpdateGSHandlerLabel(new_gs_index);
			SetupGsHandler();
		}
		else
		{
			outputWindow_resized();
			auto gsHandler = m_virtualMachine->GetGSHandler();
			if(gsHandler)
			{
				gsHandler->NotifyPreferencesChanged();
			}
		}
	}
}

void MainWindow::SetupSaveLoadStateSlots()
{
	bool enable = IsExecutableLoaded();
	ui->menuSave_States->clear();
	ui->menuLoad_States->clear();
	for(int i = 0; i < 10; i++)
	{
		QString info = enable ? GetSaveStateInfo(i) : "Empty";

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
	auto stateFilePath = m_virtualMachine->GenerateStatePath(stateSlot);
	auto future = m_virtualMachine->SaveState(stateFilePath);
	m_continuationChecker->GetContinuationManager().Register(std::move(future),
	                                                         [this, stateSlot = stateSlot](const bool& succeeded) {
		                                                         if(succeeded)
		                                                         {
			                                                         m_msgLabel->setText(QString("Saved state to slot %1.").arg(stateSlot));
			                                                         QString datetime = GetSaveStateInfo(stateSlot);
			                                                         ui->menuSave_States->actions().at(stateSlot)->setText(QString("Save Slot %1 - %2").arg(stateSlot).arg(datetime));
			                                                         ui->menuLoad_States->actions().at(stateSlot)->setText(QString("Load Slot %1 - %2").arg(stateSlot).arg(datetime));
		                                                         }
		                                                         else
		                                                         {
			                                                         m_msgLabel->setText(QString("Error saving state to slot %1.").arg(stateSlot));
		                                                         }
	                                                         });
}

void MainWindow::loadState(int stateSlot)
{
	auto stateFilePath = m_virtualMachine->GenerateStatePath(stateSlot);
	auto future = m_virtualMachine->LoadState(stateFilePath);
	m_continuationChecker->GetContinuationManager().Register(std::move(future),
	                                                         [this, stateSlot = stateSlot](const bool& succeeded) {
		                                                         if(succeeded)
		                                                         {
			                                                         m_msgLabel->setText(QString("Loaded state from slot %1.").arg(stateSlot));
#ifndef DEBUGGER_INCLUDED
			                                                         m_virtualMachine->Resume();
#endif
		                                                         }
		                                                         else
		                                                         {
			                                                         m_msgLabel->setText(QString("Error loading state from slot %1.").arg(stateSlot));
		                                                         }
	                                                         });
}

QString MainWindow::GetSaveStateInfo(int stateSlot)
{
	auto stateFilePath = m_virtualMachine->GenerateStatePath(stateSlot);
	QFileInfo file(PathToQString(stateFilePath));
	if(file.exists() && file.isFile())
	{
		return file.lastModified().toString("hh:mm dd.MM.yyyy");
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
	//btnRes is the answer to the "Are you sure you want to exit?" question.
	auto resBtn = QMessageBox::Yes;

	if(CAppConfig::GetInstance().GetPreferenceBoolean(PREF_UI_SHOWEXITCONFIRMATION))
	{
		auto message =
		    [this]() {
			    if(ui->bootablesView->IsProcessing())
			    {
				    return tr("Are you sure you want to exit?\nBootables are currently getting processed.\n");
			    }
			    else
			    {
				    return tr("Are you sure you want to exit?\nAny progress will be lost if not saved previously.\n");
			    }
		    }();

		QMessageBox messageBox;

		QCheckBox* showAgainCheckBox = new QCheckBox("Show this message next time", &messageBox);
		showAgainCheckBox->setChecked(true);

		messageBox.setWindowTitle("Close Confirmation");
		messageBox.setText(message);
		messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
		messageBox.setDefaultButton(QMessageBox::Yes);
		messageBox.setIcon(QMessageBox::Question);
		messageBox.setCheckBox(showAgainCheckBox);
		resBtn = static_cast<QMessageBox::StandardButton>(messageBox.exec());

		CAppConfig::GetInstance().SetPreferenceBoolean(PREF_UI_SHOWEXITCONFIRMATION, showAgainCheckBox->isChecked());
	}

	if(resBtn != QMessageBox::Yes)
	{
		event->ignore();
	}
	else
	{
		event->accept();
#ifdef DEBUGGER_INCLUDED
		m_debugger->close();
		m_frameDebugger->close();
#endif
	}
}

void MainWindow::changeEvent(QEvent* event)
{
	if(event->type() == QEvent::WindowStateChange)
	{
		if(isFullScreen())
		{
			statusBar()->hide();
			menuBar()->hide();
			m_outputwindow->ShowFullScreenCursor();
			ui->actionToggleFullscreen->setChecked(true);
		}
		else
		{
			statusBar()->show();
			menuBar()->show();
			m_outputwindow->DismissFullScreenCursor();
			ui->actionToggleFullscreen->setChecked(false);
		}
	}
}

void MainWindow::on_actionAbout_triggered()
{
	auto about = QString("Version %1 (%2)\nQt v%3 - zlib v%4")
	                 .arg(QString(PLAY_VERSION), __DATE__, QT_VERSION_STR, ZLIB_VERSION);
	QMessageBox::about(this, this->windowTitle(), about);
}

void MainWindow::EmitOnExecutableChange()
{
	emit onExecutableChange();
}

void MainWindow::HandleOnExecutableChange()
{
	UpdateUI();
	auto titleString = QString("Play! - [ %1 ] - %2").arg(m_virtualMachine->m_ee->m_os->GetExecutableName(), QString(PLAY_VERSION));
	setWindowTitle(titleString);
	ui->bootablesView->AsyncResetModel(true);
}

bool MainWindow::IsExecutableLoaded() const
{
	return (m_virtualMachine != nullptr ? (m_virtualMachine->m_ee->m_os->GetELF() != nullptr) : false);
}

void MainWindow::UpdateUI()
{
	ui->actionPause_when_focus_is_lost->setChecked(m_pauseFocusLost);
	ui->actionReset->setEnabled(!m_lastOpenCommand.path.empty());
	ui->actionPause_Resume->setEnabled(IsExecutableLoaded());
	SetOutputWindowSize();
	SetupSaveLoadStateSlots();
}

void MainWindow::UpdateCpuUsageLabel()
{
	bool visible = CAppConfig::GetInstance().GetPreferenceBoolean(PREF_UI_SHOWEECPUUSAGE);
	m_cpuUsageLabel->setVisible(visible);
}

void MainWindow::RegisterPreferences()
{
	CAppConfig::GetInstance().RegisterPreferenceBoolean(PREFERENCE_AUDIO_ENABLEOUTPUT, true);
	CAppConfig::GetInstance().RegisterPreferenceBoolean(PREF_UI_PAUSEWHENFOCUSLOST, true);
	CAppConfig::GetInstance().RegisterPreferenceBoolean(PREF_UI_SHOWEECPUUSAGE, false);
	CAppConfig::GetInstance().RegisterPreferenceBoolean(PREF_UI_SHOWEXITCONFIRMATION, true);
	CAppConfig::GetInstance().RegisterPreferenceInteger(PREF_VIDEO_GS_HANDLER, SettingsDialog::GS_HANDLERS::OPENGL);
	CAppConfig::GetInstance().RegisterPreferenceString(PREF_INPUT_PAD1_PROFILE, "default");
}

void MainWindow::focusOutEvent(QFocusEvent* event)
{
	if(m_pauseFocusLost && m_virtualMachine->GetStatus() == CVirtualMachine::RUNNING)
	{
		if(!isActiveWindow() && !m_outputwindow->isActive())
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
		if(m_deactivatePause && (isActiveWindow() || m_outputwindow->isActive()))
		{
			if(m_virtualMachine != nullptr)
			{
				m_virtualMachine->Resume();
				m_deactivatePause = false;
			}
		}
	}
}

void MainWindow::outputWindow_doubleClickEvent(QMouseEvent* ev)
{
	if(!m_virtualMachine->HasGunListener() && (ev->button() == Qt::LeftButton))
	{
		on_actionToggleFullscreen_triggered();
	}
}

void MainWindow::outputWindow_mouseMoveEvent(QMouseEvent* ev)
{
	if(m_virtualMachine->HasGunListener())
	{
		auto gsHandler = m_virtualMachine->GetGSHandler();
		if(!gsHandler) return;
		qreal scale = 1.0;
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
		scale = devicePixelRatioF();
#endif
		auto presentationViewport = gsHandler->GetPresentationViewport();
		float vpOfsX = static_cast<float>(presentationViewport.offsetX) / scale;
		float vpOfsY = static_cast<float>(presentationViewport.offsetY) / scale;
		float vpWidth = static_cast<float>(presentationViewport.width) / scale;
		float vpHeight = static_cast<float>(presentationViewport.height) / scale;
		float mouseX = ev->x();
		float mouseY = ev->y();
		mouseX -= vpOfsX;
		mouseY -= vpOfsY;
		mouseX = std::clamp<float>(mouseX, 0, vpWidth);
		mouseY = std::clamp<float>(mouseY, 0, vpHeight);
		m_virtualMachine->ReportGunPosition(
		    static_cast<float>(mouseX) / static_cast<float>(vpWidth),
		    static_cast<float>(mouseY) / static_cast<float>(vpHeight));
	}
}

void MainWindow::outputWindow_mousePressEvent(QMouseEvent* ev)
{
	m_qtMouseInputProvider->OnMousePress(ev->button());
}

void MainWindow::outputWindow_mouseReleaseEvent(QMouseEvent* ev)
{
	m_qtMouseInputProvider->OnMouseRelease(ev->button());
}

void MainWindow::on_actionToggleFullscreen_triggered()
{
	if(isFullScreen())
	{
		showNormal();
	}
	else
	{
		showFullScreen();
	}
}

void MainWindow::buildResizeWindowMenu()
{
	struct VIDEO_MODE
	{
		const char* name;
		unsigned int width;
		unsigned int height;
	};
	// clang-format off
	static const VIDEO_MODE videoModes[] =
	{
		{ "NTSC", 640, 448 },
		{ "PAL", 640, 512 },
		{ "HDTV (1080)", 1920, 1080 }
	};
	static const float scaleRatios[] =
	{
		0.5f,
		1.0f,
		2.0f
	};
	// clang-format on
	for(const auto& videoMode : videoModes)
	{
		auto videoModeDesc = QString("%1 - %2x%3").arg(videoMode.name).arg(videoMode.width).arg(videoMode.height);
		QMenu* videoModeMenu = ui->menuResizeWindow->addMenu(videoModeDesc);

		for(const auto& scaleRatio : scaleRatios)
		{
			QAction* scaleAction = new QAction(this);
			scaleAction->setText(QString("%1x").arg(scaleRatio));
			videoModeMenu->addAction(scaleAction);

			auto width = static_cast<uint32>(static_cast<float>(videoMode.width) * scaleRatio);
			auto height = static_cast<uint32>(static_cast<float>(videoMode.height) * scaleRatio);
			connect(scaleAction, &QAction::triggered, std::bind(&MainWindow::resizeWindow, this, width, height));
		}
	}
}

void MainWindow::resizeWindow(unsigned int width, unsigned int height)
{
	ui->centralWidget->resize(width, height);
	ui->centralWidget->setMinimumSize(width, height);
	adjustSize();
	ui->centralWidget->setMinimumSize(0, 0);
}

#ifdef DEBUGGER_INCLUDED

void MainWindow::ShowMainWindow()
{
	show();
	raise();
	activateWindow();
}

void MainWindow::ShowDebugger()
{
	m_debugger->showMaximized();
	m_debugger->raise();
	m_debugger->activateWindow();
}

void MainWindow::ShowFrameDebugger()
{
	m_frameDebugger->show();
	m_frameDebugger->raise();
	m_frameDebugger->activateWindow();
}

fs::path MainWindow::GetFrameDumpDirectoryPath()
{
	return CAppConfig::GetInstance().GetBasePath() / fs::path("framedumps/");
}

void MainWindow::DumpNextFrame()
{
	m_virtualMachine->m_ee->m_gs->TriggerFrameDump(
	    [&](const CFrameDump& frameDump) {
		    try
		    {
			    auto frameDumpDirectoryPath = GetFrameDumpDirectoryPath();
			    Framework::PathUtils::EnsurePathExists(frameDumpDirectoryPath);
			    for(unsigned int i = 0; i < UINT_MAX; i++)
			    {
				    auto frameDumpFileName = string_format("framedump_%08d.dmp.zip", i);
				    auto frameDumpPath = frameDumpDirectoryPath / fs::path(frameDumpFileName);
				    if(!fs::exists(frameDumpPath))
				    {
					    auto dumpStream = Framework::CreateOutputStdStream(frameDumpPath.native());
					    frameDump.Write(dumpStream);
					    m_msgLabel->setText(QString("Dumped frame to '%1'.").arg(frameDumpFileName.c_str()));
					    return;
				    }
			    }
		    }
		    catch(...)
		    {
		    }
		    m_msgLabel->setText(QString("Failed to dump frame."));
	    });
}

void MainWindow::ToggleGsDraw()
{
	auto gs = m_virtualMachine->GetGSHandler();
	if(gs == nullptr) return;
	bool newState = !gs->GetDrawEnabled();
	gs->SetDrawEnabled(newState);
	debugMenuUi->actionGsDrawEnabled->setChecked(newState);
	m_msgLabel->setText(newState ? QString("GS Draw Enabled") : QString("GS Draw Disabled"));
}

#endif

void MainWindow::on_actionPause_when_focus_is_lost_triggered(bool checked)
{
	m_pauseFocusLost = checked;
	CAppConfig::GetInstance().SetPreferenceBoolean(PREF_UI_PAUSEWHENFOCUSLOST, m_pauseFocusLost);
}

void MainWindow::on_actionReset_triggered()
{
	try
	{
		if(!m_lastOpenCommand.path.empty())
		{
			switch(m_lastOpenCommand.type)
			{
			case BootType::CD:
				BootCDROM();
				break;
			case BootType::ELF:
				BootElf(m_lastOpenCommand.path);
				break;
			case BootType::ARCADE:
				BootArcadeMachine(m_lastOpenCommand.path);
				break;
			default:
				assert(false);
				break;
			}
		}
	}
	catch(const std::exception& e)
	{
		QMessageBox messageBox;
		messageBox.critical(nullptr, "Error", e.what());
		messageBox.show();
	}
}

void MainWindow::on_actionMemory_Card_Manager_triggered()
{
	MemoryCardManagerDialog mcm(this);
	mcm.exec();
}

void MainWindow::on_actionVFS_Manager_triggered()
{
	VFSManagerDialog vfsmgr;
	vfsmgr.exec();
}

void MainWindow::on_actionController_Manager_triggered()
{
	auto padHandler = static_cast<CPH_GenericInput*>(m_virtualMachine->GetPadHandler());
	if(!padHandler) return;

	ControllerConfigDialog ccd(&padHandler->GetBindingManager(), m_qtKeyInputProvider.get(), m_qtMouseInputProvider.get(), this);
	ccd.exec();
}

void MainWindow::on_actionCapture_Screen_triggered()
{
	m_screenShotCompleteConnection = CScreenShotUtils::TriggerGetScreenshot(m_virtualMachine,
	                                                                        [&](int res, const char* msg) -> void {
		                                                                        m_msgLabel->setText(msg);
	                                                                        });
}

void MainWindow::on_actionList_Bootables_triggered()
{
	ui->stackedWidget->setCurrentIndex(1 - ui->stackedWidget->currentIndex());
}

void MainWindow::UpdateGSHandlerLabel(int gs_index)
{
#if HAS_GSH_VULKAN
	switch(gs_index)
	{
	default:
	case SettingsDialog::GS_HANDLERS::OPENGL:
		m_gsLabel->setText("OpenGL");
		break;
	case SettingsDialog::GS_HANDLERS::VULKAN:
		m_gsLabel->setText("Vulkan");
		break;
	}
#endif
}

void MainWindow::SetupBootableView()
{
	auto showEmu = std::bind(&QStackedWidget::setCurrentIndex, ui->stackedWidget, 1);
	QBootablesView* bootablesView = ui->bootablesView;

	bootablesView->AddMsgLabel(m_msgLabel);

	QBootablesView::BootCallback bootGameCallback = [&, showEmu](fs::path filePath) {
		try
		{
			if(IsBootableDiscImagePath(filePath))
			{
				LoadCDROM(filePath);
				BootCDROM();
			}
			else if(IsBootableExecutablePath(filePath))
			{
				BootElf(filePath);
			}
			else if(IsBootableArcadeDefPath(filePath))
			{
				BootArcadeMachine(filePath);
			}
			else
			{
				QMessageBox messageBox;
				QString invalid("Invalid File Format.");
				messageBox.critical(this, this->windowTitle(), invalid);
				messageBox.show();
			}
			showEmu();
		}
		catch(const std::exception& e)
		{
			QMessageBox messageBox;
			messageBox.critical(nullptr, "Error", e.what());
			messageBox.show();
		}
	};
	bootablesView->SetupActions(bootGameCallback);
}

void MainWindow::SetupDebugger()
{
#ifdef DEBUGGER_INCLUDED
	CDebugSupportSettings::GetInstance().Initialize(&CAppConfig::GetInstance());

	m_debugger = std::make_unique<QtDebugger>(*m_virtualMachine);
	m_frameDebugger = std::make_unique<QtFramedebugger>();

	{
		auto debugMenu = new QMenu(this);
		debugMenuUi = new Ui::DebugMenu();
		debugMenuUi->setupUi(debugMenu);
		ui->menuBar->insertMenu(ui->menuHelp->menuAction(), debugMenu);

		connect(debugMenuUi->actionShowDebugger, &QAction::triggered, this, std::bind(&MainWindow::ShowDebugger, this));
		connect(debugMenuUi->actionShowFrameDebugger, &QAction::triggered, this, std::bind(&MainWindow::ShowFrameDebugger, this));
		connect(debugMenuUi->actionDumpNextFrame, &QAction::triggered, this, std::bind(&MainWindow::DumpNextFrame, this));
		connect(debugMenuUi->actionGsDrawEnabled, &QAction::triggered, this, std::bind(&MainWindow::ToggleGsDraw, this));
	}

#if defined(__APPLE__)
	{
		auto debugDockMenu = new QMenu(this);
		debugDockMenuUi = new Ui::DebugDockMenu();
		debugDockMenuUi->setupUi(debugDockMenu);

		connect(debugDockMenuUi->actionShowMainWindow, &QAction::triggered, this, std::bind(&MainWindow::ShowMainWindow, this));
		connect(debugDockMenuUi->actionShowDebugger, &QAction::triggered, this, std::bind(&MainWindow::ShowDebugger, this));
		connect(debugDockMenuUi->actionShowFrameDebugger, &QAction::triggered, this, std::bind(&MainWindow::ShowFrameDebugger, this));

		debugDockMenu->setAsDockMenu();
	}
#endif //defined(__APPLE__)

#endif //DEBUGGER_INCLUDED
}
