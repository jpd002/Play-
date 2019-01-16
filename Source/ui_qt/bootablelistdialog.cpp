#include "bootablelistdialog.h"
#include "ui_bootablelistdialog.h"

#include <QAction>
#include <QFileDialog>
#include <QGridLayout>
#include <QLabel>
#include <QPixmap>
#include <QPixmapCache>
#include <iostream>

#include "AppConfig.h"
#include "http/HttpClientFactory.h"
#include "QStringUtils.h"
#include "ui_shared/BootablesProcesses.h"
#include "ui_shared/BootablesDbClient.h"


BootableListDialog::BootableListDialog(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::BootableListDialog)
{
	ui->setupUi(this);
	CAppConfig::GetInstance().RegisterPreferenceInteger("ui.sortmethod", 2);
	m_sortingMethod = CAppConfig::GetInstance().GetPreferenceInteger("ui.sortmethod");
	ui->comboBox->setCurrentIndex(m_sortingMethod);

	ui->listView->setItemDelegate(new BootImageItemDelegate);
	resetModel();

	QAction *bootgame = new QAction("Boot", ui->listView);
	QAction *removegame = new QAction("Remove", ui->listView);

	ui->listView->addAction(bootgame);
	ui->listView->addAction(removegame);
	ui->listView->setContextMenuPolicy(Qt::ActionsContextMenu);

	connect(bootgame, &QAction::triggered,
	        [&](bool)
	{
		QModelIndex index= ui->listView->selectionModel()->selectedIndexes().at(0);
		bootable = model->GetBootable(index);
		accept();
	});
	connect(removegame, &QAction::triggered,
	        [&](bool)
	{
		QModelIndex index = ui->listView->selectionModel()->selectedIndexes().at(0);
		auto bootable = model->GetBootable(index);
		BootablesDb::CClient::GetInstance().UnregisterBootable(bootable.path);
		model->removeItem(index);
	});
}

BootableListDialog::~BootableListDialog()
{
	delete ui;
}

void BootableListDialog::resetModel()
{
	ui->listView->setModel(nullptr);
	if(model)
		delete model;
	model = new BootableModel(this, m_sortingMethod);
	ui->listView->setModel(model);
}
BootablesDb::Bootable BootableListDialog::getResult()
{
	return bootable;
}

void BootableListDialog::showEvent(QShowEvent* ev)
{
	ui->gridLayout->invalidate();
	QDialog::showEvent(ev);
}


void BootableListDialog::on_add_games_button_clicked()
{
	QFileDialog dialog(this);
	dialog.setFileMode(QFileDialog::ExistingFile);
	dialog.setNameFilter(tr("All supported types(*.iso *.bin *.isz *.cso *.elf);;UltraISO Compressed Disk Images (*.isz);;CISO Compressed Disk Images (*.cso);;ELF files (*.elf);;All files (*.*)"));
	if(dialog.exec())
	{
		auto filePath = QStringToPath(dialog.selectedFiles().first()).parent_path();
		try
		{
			ScanBootables(filePath, false);
		}
		catch(...)
		{

		}
		FetchGameTitles();
		FetchGameCovers();
		resetModel();
	}
}


void BootableListDialog::on_listView_doubleClicked(const QModelIndex &index)
{
	bootable = model->GetBootable(index);
	accept();
}

void BootableListDialog::on_refresh_button_clicked()
{
	auto bootables_paths = GetActiveBootableDirectories();
	for(auto path : bootables_paths)
	{
		try
		{
			ScanBootables(path);
		}
		catch(...)
		{

		}
	}
	FetchGameTitles();
	FetchGameCovers();

	resetModel();
}

void BootableListDialog::on_comboBox_currentIndexChanged(int index)
{
	CAppConfig::GetInstance().SetPreferenceInteger("ui.sortmethod", index);
	m_sortingMethod = index;
	resetModel();
}
