#include "mainwindow.h"
#include "settingsdialog.h"

#include "openglwindow.h"

#include <QFileDialog>
#include <QTimer>
#include <QWindow>

#include "global.h"
#include "GSH_OpenGLQt.h"
#include "tools/PsfPlayer/Source/SH_OpenAL.h"
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
}

MainWindow::~MainWindow()
{
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
    }
}

void MainWindow::on_actionStart_Game_triggered()
{

    if (g_virtualMachine != nullptr)
    {
        g_virtualMachine->Pause();
        g_virtualMachine->Reset();
        g_virtualMachine->m_ee->m_os->BootFromCDROM();
        g_virtualMachine->Resume();
        setOpenGlPanelSize();
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
            auto fileName = dialog.selectedFiles().first();
            g_virtualMachine->Pause();
            g_virtualMachine->Reset();
            g_virtualMachine->m_ee->m_os->BootFromFile(fileName.toStdString().c_str());
            g_virtualMachine->Resume();
            setOpenGlPanelSize();
        }
    }
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

void MainWindow::on_actionSettings_triggered()
{
    SettingsDialog sd;
    sd.exec();
    setupSoundHandler();
    CAppConfig::GetInstance().Save();
}
