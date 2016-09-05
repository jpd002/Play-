#include "mainwindow.h"
#include <QApplication>

#include "AppConfig.h"
#include "PS2VM.h"
#include "PS2VM_Preferences.h"
#include "GSH_OpenGLUnix.h"

CPS2VM* g_virtualMachine;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    g_virtualMachine = new CPS2VM();
    g_virtualMachine->Initialize();

    g_virtualMachine->CreateGSHandler(CGSH_OpenGLUnix::GetFactoryFunction(w.getOpenGLPanel()));

    CAppConfig::GetInstance().SetPreferenceString(PS2VM_CDROM0PATH, "/home/zer0/Documents/FFXII.iso");

    g_virtualMachine->Pause();
    g_virtualMachine->Reset();
    g_virtualMachine->m_ee->m_os->BootFromCDROM();
    g_virtualMachine->Resume();

    return a.exec();
}
