#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "ElfViewEx.h"
#include "DebugSupportSettings.h"
#include "AppConfig.h"
#include "StdStreamUtils.h"

#include <QFileDialog>
#include <QMessageBox>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
	CDebugSupportSettings::GetInstance().Initialize(&CAppConfig::GetInstance());

	ui->setupUi(this);
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::on_actionOpen_triggered()
{
	QFileDialog dialog(this);
	dialog.setFileMode(QFileDialog::ExistingFile);
	dialog.setNameFilter(tr("ELF files (*.elf;*.o);;All files(*.*)"));
	if(dialog.exec())
	{
		try
		{
			auto selectedPath = dialog.selectedFiles().first();
			auto view = CreateElfViewFromPath(ui->mdiArea, fs::path(selectedPath.toStdString()));
			view->show();
		}
		catch(const std::exception& ex)
		{
			auto errorMessage = QString("Failed to open ELF: %1").arg(ex.what());
			QMessageBox messageBox;
			messageBox.critical(this, "Error", errorMessage);
		}
	}
}

void MainWindow::on_actionExit_triggered()
{
	close();
}

QMdiSubWindow* MainWindow::CreateElfViewFromPath(QMdiArea* mdiArea, const fs::path& elfPath)
{
	uint8 headerId[0x10] = {};
	auto stream = Framework::CreateInputStdStream(elfPath.native());
	stream.Read(headerId, sizeof(headerId));
	switch(headerId[ELF::EI_CLASS])
	{
	case ELF::ELFCLASS32:
		return new CElfViewEx<CELF32>(mdiArea, elfPath);
	case ELF::ELFCLASS64:
		return new CElfViewEx<CELF64>(mdiArea, elfPath);
	default:
		throw std::runtime_error("Unknown ELF class type.");
	}
}
