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
#include "controllerconfigdialog.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_openglpanel = new OpenGLWindow;
    QWidget * container = QWidget::createWindowContainer(m_openglpanel);
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

    CreateStatusBar();
    UpdateUI();

}

MainWindow::~MainWindow()
{
    CAppConfig::GetInstance().Save();
	m_GPDL.reset();
    if (g_virtualMachine != nullptr)
    {
        g_virtualMachine->Pause();
        g_virtualMachine->DestroyPadHandler();
        g_virtualMachine->DestroyGSHandler();
        g_virtualMachine->DestroySoundHandler();
        g_virtualMachine->Destroy();
        delete g_virtualMachine;
        g_virtualMachine = nullptr;
    }
    delete m_InputBindingManager;
    delete ui;
}

void MainWindow::showEvent(QShowEvent* event)
{
    QMainWindow::showEvent(event);
    InitEmu();
}

void MainWindow::InitEmu()
{
    g_virtualMachine = new CPS2VM();
    g_virtualMachine->Initialize();

    g_virtualMachine->CreateGSHandler(CGSH_OpenGLQt::GetFactoryFunction(m_openglpanel));
    SetupSoundHandler();

    m_InputBindingManager = new CInputBindingManager(CAppConfig::GetInstance());
    g_virtualMachine->CreatePadHandler(CPH_HidUnix::GetFactoryFunction(m_InputBindingManager));

    auto onInput = [=](std::array<uint32, 6> device, int code, int value, int type, const input_absinfo *abs)->void
    {
        if(m_InputBindingManager != nullptr)
        {
            m_InputBindingManager->OnInputEventReceived(device, code, value);
        }
    };
    m_GPDL = std::make_unique<CGamePadDeviceListener>(onInput);

    StatsManager = new CStatsManager();
    g_virtualMachine->m_ee->m_gs->OnNewFrame.connect(std::bind(&CStatsManager::OnNewFrame, StatsManager, std::placeholders::_1));

    g_virtualMachine->OnRunningStateChange.connect(std::bind(&MainWindow::OnRunningStateChange, this));
    g_virtualMachine->m_ee->m_os->OnExecutableChange.connect(std::bind(&MainWindow::OnExecutableChange, this));
}

void MainWindow::SetOpenGlPanelSize()
{
    openGLWindow_resized();
}

void MainWindow::SetupSoundHandler()
{
    if(g_virtualMachine != nullptr)
    {
        bool audioEnabled = CAppConfig::GetInstance().GetPreferenceBoolean(PREFERENCE_AUDIO_ENABLEOUTPUT);
        if(audioEnabled)
        {
            g_virtualMachine->CreateSoundHandler(&CSH_OpenAL::HandlerFactory);
        }
        else
        {
            g_virtualMachine->DestroySoundHandler();
        }
    }
}

void MainWindow::openGLWindow_resized()
{
    if (g_virtualMachine != nullptr && g_virtualMachine->m_ee != nullptr  && g_virtualMachine->m_ee->m_gs != nullptr )
        {
            GLint w = m_openglpanel->size().width(), h = m_openglpanel->size().height();

            CGSHandler::PRESENTATION_PARAMS presentationParams;
            presentationParams.mode 			= (CGSHandler::PRESENTATION_MODE)CAppConfig::GetInstance().GetPreferenceInteger(PREF_CGSHANDLER_PRESENTATION_MODE);
            presentationParams.windowWidth 		= w;
            presentationParams.windowHeight 	= h;
            g_virtualMachine->m_ee->m_gs->SetPresentationParams(presentationParams);
            g_virtualMachine->m_ee->m_gs->Flip();
        }
}

void MainWindow::on_actionOpen_Game_triggered()
{
    QFileDialog dialog(this);
    dialog.setDirectory(m_lastpath);
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setNameFilter(tr("All supported types(*.iso *.bin *.isz *.cso);;UltraISO Compressed Disk Images (*.isz);;CISO Compressed Disk Images (*.cso);;All files (*.*)"));
    if (dialog.exec())
    {
        auto fileName = dialog.selectedFiles().first();
        m_lastpath = QFileInfo(fileName).path();
        CAppConfig::GetInstance().SetPreferenceString(PREF_PS2_CDROM0_PATH, fileName.toStdString().c_str());

        if (g_virtualMachine != nullptr)
        {
            try
            {
                m_lastOpenCommand = new lastOpenCommand(BootType::CD, fileName.toStdString().c_str());
                BootCDROM();
            } catch( const std::exception& e) {
                QMessageBox messageBox;
                messageBox.critical(0,"Error",e.what());
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
    if (dialog.exec())
    {
        auto fileName = dialog.selectedFiles().first();
        m_lastpath = QFileInfo(fileName).path();
        if (g_virtualMachine != nullptr)
        {
            try
            {
                m_lastOpenCommand = new lastOpenCommand(BootType::ELF, fileName.toStdString().c_str());
                BootElf(fileName.toStdString().c_str());
            } catch( const std::exception& e) {
                QMessageBox messageBox;
                messageBox.critical(0,"Error",e.what());
                messageBox.show();
            }
        }
    }
}

void MainWindow::BootElf(const char* fileName)
{
    g_virtualMachine->Pause();
    g_virtualMachine->Reset();
    g_virtualMachine->m_ee->m_os->BootFromFile(fileName);
    g_virtualMachine->Resume();
}

void MainWindow::BootCDROM()
{
    g_virtualMachine->Pause();
    g_virtualMachine->Reset();
    g_virtualMachine->m_ee->m_os->BootFromCDROM();
    g_virtualMachine->Resume();
}

void MainWindow::on_actionExit_triggered()
{
    close();
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if(event->key() != Qt::Key_Escape && m_InputBindingManager != nullptr)
    {
        m_InputBindingManager->OnInputEventReceived({0}, event->key(), 1);
    }
}

void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
    if(event->key() == Qt::Key_Escape)
    {
        if(isFullScreen()) {
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
    gameIDLabel = new QLabel(" GameID ");
    gameIDLabel->setAlignment(Qt::AlignHCenter);
    gameIDLabel->setMinimumSize(gameIDLabel->sizeHint());

    fpsLabel = new QLabel(" fps:00 ");
    fpsLabel->setAlignment(Qt::AlignHCenter);
    fpsLabel->setMinimumSize(fpsLabel->sizeHint());

    m_dcLabel = new QLabel(" dc:0000 ");
    m_dcLabel->setAlignment(Qt::AlignHCenter);
    m_dcLabel->setMinimumSize(m_dcLabel->sizeHint());


    m_stateLabel = new QLabel(" Paused ");
    m_stateLabel->setAlignment(Qt::AlignHCenter);
    m_stateLabel->setMinimumSize(m_dcLabel->sizeHint());


    m_msgLabel = new ElidedLabel();
    m_msgLabel->setAlignment(Qt::AlignLeft);
    QFontMetrics fm(m_msgLabel->font());
    m_msgLabel->setMinimumSize(fm.boundingRect("...").size());
    m_msgLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);

    statusBar()->addWidget(m_stateLabel);
    statusBar()->addWidget(fpsLabel);
    statusBar()->addWidget(m_dcLabel);
    statusBar()->addWidget(m_msgLabel, 1);
    statusBar()->addWidget(gameIDLabel);

    m_fpstimer = new QTimer(this);
    connect(m_fpstimer, SIGNAL(timeout()), this, SLOT(setFPS()));
    m_fpstimer->start(1000);
}

void MainWindow::setFPS()
{
    int frames = StatsManager->GetFrames();
    int drawCalls = StatsManager->GetDrawCalls();
    int dcpf = (frames != 0) ? (drawCalls / frames) : 0;
    //fprintf(stderr, "%d f/s, %d dc/f\n", frames, dcpf);
    StatsManager->ClearStats();
    fpsLabel->setText(QString(" fps: %1 ").arg(frames));
    m_dcLabel->setText(QString(" dc: %1 ").arg(dcpf));
}

void MainWindow::OnRunningStateChange()
{
    m_stateLabel->setText(g_virtualMachine->GetStatus() == CVirtualMachine::PAUSED ? "Paused" : "Running");
}

void MainWindow::on_actionSettings_triggered()
{
    SettingsDialog sd;
    sd.exec();
    SetupSoundHandler();
}

void MainWindow::SetupSaveLoadStateSlots()
{
    bool enable = (g_virtualMachine != nullptr ? (g_virtualMachine->m_ee->m_os->GetELF() != nullptr) : false);
    ui->menuSave_States->clear();
    ui->menuLoad_States->clear();
    for (int i=1; i <= 10; i++){
        QString info = enable ? SaveStateInfo(i) : "Empty";

        QAction* saveaction = new QAction(this);
        saveaction->setText(QString("Save Slot %1 - %2").arg(i).arg(info));
        saveaction->setEnabled(enable);
        ui->menuSave_States->addAction(saveaction);

        QAction* loadaction = new QAction(this);
        loadaction->setText(QString("Load Slot %1 - %2").arg(i).arg(info));
        loadaction->setEnabled(enable);
        ui->menuLoad_States->addAction(loadaction);

        if (enable)
        {
            connect(saveaction, &QAction::triggered, std::bind(&MainWindow::saveState, this, i));
            connect(loadaction, &QAction::triggered, std::bind(&MainWindow::loadState, this, i));
        }

    }
}

void MainWindow::saveState(int stateSlot)
{
    Framework::PathUtils::EnsurePathExists(CPS2VM::GetStateDirectoryPath());

    auto stateFilePath = g_virtualMachine->GenerateStatePath(stateSlot);
    g_virtualMachine->SaveState(stateFilePath);

    QDateTime* dt = new QDateTime;
    QString datetime = dt->currentDateTime().toString("hh:mm dd.MM.yyyy");
    ui->menuSave_States->actions().at(stateSlot-1)->setText(QString("Save Slot %1 - %2").arg(stateSlot).arg(datetime));
    ui->menuLoad_States->actions().at(stateSlot-1)->setText(QString("Load Slot %1 - %2").arg(stateSlot).arg(datetime));
}

void MainWindow::loadState(int stateSlot){
    auto stateFilePath = g_virtualMachine->GenerateStatePath(stateSlot);
    g_virtualMachine->LoadState(stateFilePath);
    g_virtualMachine->Resume();
}

QString MainWindow::SaveStateInfo(int stateSlot)
{
    auto stateFilePath = g_virtualMachine->GenerateStatePath(stateSlot);
    QFileInfo file(stateFilePath.string().c_str());
    if (file.exists() && file.isFile()) {
        return file.created().toString("hh:mm dd.MM.yyyy");
    } else {
        return "Empty";
    }
}

void MainWindow::on_actionPause_Resume_triggered()
{
    if (g_virtualMachine != nullptr)
    {
        if (g_virtualMachine->GetStatus() == CVirtualMachine::PAUSED)
        {
            g_virtualMachine->Resume();
        } else {
            g_virtualMachine->Pause();
        }
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QMessageBox::StandardButton resBtn = QMessageBox::question(this, "Close Confirmation?",
                                                                tr("Are you sure you want to exit?\nHave you saved your progress?\n"),
                                                                QMessageBox::Yes | QMessageBox::No,
                                                                QMessageBox::Yes);
    if (resBtn != QMessageBox::Yes) {
        event->ignore();
    } else {
        event->accept();
    }
}

void MainWindow::on_actionAbout_triggered()
{
    QMessageBox messageBox;
    messageBox.setIconPixmap(QPixmap(":/assets/app_icon.png"));
    QString about("Version %1 (%2)\nQt v%3 - zlib v%4 - boost v%5");
    QString ver("%1.%2.%3"), boostver,qtver;
    boostver = ver.arg(BOOST_VERSION / 100000).arg(BOOST_VERSION / 100 % 1000).arg(BOOST_VERSION % 100);
    messageBox.about(this, this->windowTitle(), about.arg(QString(PLAY_VERSION), __DATE__, QT_VERSION_STR, ZLIB_VERSION, boostver));
    messageBox.show();
}

void MainWindow::OnExecutableChange()
{
    UpdateUI();
    gameIDLabel->setText(QString(" %1 ").arg(g_virtualMachine->m_ee->m_os->GetExecutableName()));
}

void MainWindow::UpdateUI()
{
    ui->actionPause_when_focus_is_lost->setChecked(m_pauseFocusLost);
    ui->actionReset->setEnabled(m_lastOpenCommand != nullptr);
    SetOpenGlPanelSize();
    SetupSaveLoadStateSlots();
}

void MainWindow::RegisterPreferences()
{
    CAppConfig::GetInstance().RegisterPreferenceBoolean(PREFERENCE_AUDIO_ENABLEOUTPUT, true);
    CAppConfig::GetInstance().RegisterPreferenceBoolean(PREF_UI_PAUSEWHENFOCUSLOST, true);
}

void MainWindow::focusOutEvent(QFocusEvent * event)
{
    if (m_pauseFocusLost && g_virtualMachine->GetStatus() == CVirtualMachine::RUNNING)
    {
        if (!isActiveWindow() && !m_openglpanel->isActive())
        {
            if (g_virtualMachine != nullptr) {
                g_virtualMachine->Pause();
                m_deactivatePause = true;
            }
        }
    }
}
void MainWindow::focusInEvent(QFocusEvent * event)
{
    if (m_pauseFocusLost && g_virtualMachine->GetStatus() == CVirtualMachine::PAUSED)
    {
        if (m_deactivatePause && (isActiveWindow() || m_openglpanel->isActive())){
            if (g_virtualMachine != nullptr)
            {
                g_virtualMachine->Resume();
                m_deactivatePause = false;
            }
        }
    }
}

void MainWindow::doubleClickEvent(QMouseEvent * ev)
{
    if (ev->button() == Qt::LeftButton)
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
    if (m_lastOpenCommand != nullptr){
        if (m_lastOpenCommand->type == BootType::CD){
            BootCDROM();
        } else if (m_lastOpenCommand->type == BootType::ELF) {
            BootElf(m_lastOpenCommand->filename);
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
    CScreenShotUtils::TriggerGetScreenshot(g_virtualMachine,
        [&](int res, const char* msg)->void
        {
            m_msgLabel->setText(msg);
        }
    );
}
