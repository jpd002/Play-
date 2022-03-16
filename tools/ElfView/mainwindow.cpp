#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "ElfViewEx.h"

#include <QFileDialog>
#include <QMessageBox>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
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
	dialog.setNameFilter(tr("ELF files (*.elf)"));
	if(dialog.exec())
	{
		try
		{
			auto selectedPath = dialog.selectedFiles().first();
			auto view = new CElfViewEx(ui->mdiArea, fs::path(selectedPath.toStdString()));
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
