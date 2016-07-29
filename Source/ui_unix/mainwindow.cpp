#include "mainwindow.h"
#include "settingsdialog.h"

#include "openglwindow.h"

#include <QDateTime>
#include <QFileDialog>
#include <QTimer>
#include <QWindow>
#include <QMessageBox>

#include "GSH_OpenGLQt.h"
#include "tools/PsfPlayer/Source/SH_OpenAL.h"
#include "DiskUtils.h"
#include "PathUtils.h"
#include <zlib.h>
#include <boost/version.hpp>

#include "PreferenceDefs.h"

#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    openglpanel = new OpenGLWindow;
    QWidget * container = QWidget::createWindowContainer(openglpanel);
    ui->gridLayout->addWidget(container);
    connect(openglpanel, SIGNAL(heightChanged(int)), this, SLOT(openGLWindow_resized()));
    connect(openglpanel, SIGNAL(widthChanged(int)), this, SLOT(openGLWindow_resized()));

    connect(openglpanel, SIGNAL(keyUp(QKeyEvent*)), this, SLOT(keyReleaseEvent(QKeyEvent*)));
    connect(openglpanel, SIGNAL(keyDown(QKeyEvent*)), this, SLOT(keyPressEvent(QKeyEvent*)));


    connect(openglpanel, SIGNAL(focusOut(QFocusEvent*)), this, SLOT(focusOutEvent(QFocusEvent*)));
    connect(openglpanel, SIGNAL(focusIn(QFocusEvent*)), this, SLOT(focusInEvent(QFocusEvent*)));

    RegisterPreferences();

    m_pauseFocusLost = CAppConfig::GetInstance().GetPreferenceBoolean(PREF_UI_PAUSEWHENFOCUSLOST);

    UpdateUI();

}

MainWindow::~MainWindow()
{
    CAppConfig::GetInstance().Save();
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
    delete ui;
}

void MainWindow::showEvent(QShowEvent* event)
{
    QMainWindow::showEvent(event);
    initEmu();
}

void MainWindow::initEmu(){
    g_virtualMachine = new CPS2VM();
    g_virtualMachine->Initialize();

    g_virtualMachine->CreateGSHandler(CGSH_OpenGLQt::GetFactoryFunction(openglpanel));
    setupSoundHandler();

    g_virtualMachine->CreatePadHandler(CPH_HidUnix::GetFactoryFunction());
    padhandler = static_cast<CPH_HidUnix*>(g_virtualMachine->GetPadHandler());

    StatsManager = new CStatsManager();
    g_virtualMachine->m_ee->m_gs->OnNewFrame.connect(boost::bind(&CStatsManager::OnNewFrame, StatsManager, _1));

    g_virtualMachine->OnRunningStateChange.connect(boost::bind(&MainWindow::OnRunningStateChange, this));
    g_virtualMachine->m_ee->m_os->OnExecutableChange.connect(boost::bind(&MainWindow::OnExecutableChange, this));
    createStatusBar();
}


void MainWindow::setOpenGlPanelSize()
{
    openGLWindow_resized();
}

void MainWindow::setupSoundHandler()
{
    if(g_virtualMachine != nullptr){
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

            GLint w = openglpanel->size().width(), h = openglpanel->size().height();

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
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setNameFilter(tr("ISO/CSO/ISZ files (*.iso *cso *.isz)"));
    if (dialog.exec())
    {
        auto fileName = dialog.selectedFiles().first();
        CAppConfig::GetInstance().SetPreferenceString(PS2VM_CDROM0PATH, fileName.toStdString().c_str());


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
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setNameFilter(tr("ELF files (*.elf)"));
    if (dialog.exec())
    {
        if (g_virtualMachine != nullptr)
        {
            try
            {
                auto fileName = dialog.selectedFiles().first();
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
    if (padhandler != nullptr)
            padhandler->InputValueCallbackPressed(event->key());
}

void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
    if (padhandler != nullptr)
            padhandler->InputValueCallbackReleased(event->key());
}

void MainWindow::createStatusBar()
{
    fpsLabel = new QLabel(" fps:00 ");
    fpsLabel->setAlignment(Qt::AlignHCenter);
    fpsLabel->setMinimumSize(fpsLabel->sizeHint());

    dcLabel = new QLabel(" dc:0000 ");
    dcLabel->setAlignment(Qt::AlignHCenter);
    dcLabel->setMinimumSize(dcLabel->sizeHint());


    stateLabel = new QLabel(" Paused ");
    stateLabel->setAlignment(Qt::AlignHCenter);
    stateLabel->setMinimumSize(dcLabel->sizeHint());


    statusBar()->addWidget(stateLabel);
    statusBar()->addWidget(fpsLabel);
    statusBar()->addWidget(dcLabel);


    fpstimer = new QTimer(this);
    connect(fpstimer, SIGNAL(timeout()), this, SLOT(setFPS()));
    fpstimer->start(1000);
}

void MainWindow::setFPS()
{
    int frames = StatsManager->GetFrames();
    int drawCalls = StatsManager->GetDrawCalls();
    int dcpf = (frames != 0) ? (drawCalls / frames) : 0;
    //fprintf(stderr, "%d f/s, %d dc/f\n", frames, dcpf);
    StatsManager->ClearStats();
    fpsLabel->setText(QString(" fps: %1 ").arg(frames));
    dcLabel->setText(QString(" dc: %1 ").arg(dcpf));
}

void MainWindow::OnRunningStateChange()
{
    stateLabel->setText(g_virtualMachine->GetStatus() == CVirtualMachine::PAUSED ? "Paused" : "Running");
}

void MainWindow::on_actionSettings_triggered()
{
    SettingsDialog sd;
    sd.exec();
    setupSoundHandler();
}

void MainWindow::setupSaveLoadStateSlots(){
    bool enable = (g_virtualMachine != nullptr ? (g_virtualMachine->m_ee->m_os->GetELF() != nullptr) : false);
    ui->menuSave_States->clear();
    ui->menuLoad_States->clear();
    for (int i=1; i <= 10; i++){
        QString info = enable ? saveStateInfo(i) : "Empty";

        QAction* saveaction = new QAction(this);
        saveaction->setText(QString("Save Slot %1 - %2").arg(i).arg(info));
        saveaction->setEnabled(enable);
        saveaction->setProperty("stateSlot", i);
        ui->menuSave_States->addAction(saveaction);

        QAction* loadaction = new QAction(this);
        loadaction->setText(QString("Load Slot %1 - %2").arg(i).arg(info));
        loadaction->setEnabled(enable);
        loadaction->setProperty("stateSlot", i);
        ui->menuLoad_States->addAction(loadaction);

        if (enable)
        {
            connect(saveaction, SIGNAL(triggered()), this, SLOT(saveState()));
            connect(loadaction, SIGNAL(triggered()), this, SLOT(loadState()));
        }

    }
}

void MainWindow::saveState(){
    Framework::PathUtils::EnsurePathExists(GetStateDirectoryPath());

    int m_stateSlot = sender()->property("stateSlot").toInt();
    g_virtualMachine->SaveState(GenerateStatePath(m_stateSlot).string().c_str());

    QDateTime* dt = new QDateTime;
    QString datetime = dt->currentDateTime().toString("hh:mm dd.MM.yyyy");
    ui->menuSave_States->actions().at(m_stateSlot-1)->setText(QString("Save Slot %1 - %2").arg(m_stateSlot).arg(datetime));
    ui->menuLoad_States->actions().at(m_stateSlot-1)->setText(QString("Load Slot %1 - %2").arg(m_stateSlot).arg(datetime));
}

void MainWindow::loadState(){
    int m_stateSlot = sender()->property("stateSlot").toInt();
    g_virtualMachine->LoadState(GenerateStatePath(m_stateSlot).string().c_str());
    g_virtualMachine->Resume();
}
QString MainWindow::saveStateInfo(int m_stateSlot) {
    QFileInfo file(GenerateStatePath(m_stateSlot).string().c_str());
    if (file.exists() && file.isFile()) {
        return file.created().toString("hh:mm dd.MM.yyyy");
    } else {
        return "Empty";
    }
}

boost::filesystem::path MainWindow::GetStateDirectoryPath()
{
    return CAppConfig::GetBasePath() / boost::filesystem::path("states/");
}

boost::filesystem::path MainWindow::GenerateStatePath(int m_stateSlot)
{
    std::string stateFileName = std::string(g_virtualMachine->m_ee->m_os->GetExecutableName()) + ".st" + std::to_string(m_stateSlot) + ".zip";
    return GetStateDirectoryPath() / boost::filesystem::path(stateFileName);
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
}

void MainWindow::UpdateUI()
{
    ui->actionPause_when_focus_is_lost->setChecked(m_pauseFocusLost);
    ui->actionReset->setEnabled(m_lastOpenCommand != nullptr);
    setOpenGlPanelSize();
    setupSaveLoadStateSlots();
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
        if (!isActiveWindow() && !openglpanel->isActive())
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
        if (m_deactivatePause && (isActiveWindow() || openglpanel->isActive())){
            if (g_virtualMachine != nullptr)
            {
                g_virtualMachine->Resume();
                m_deactivatePause = false;
            }
        }
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
