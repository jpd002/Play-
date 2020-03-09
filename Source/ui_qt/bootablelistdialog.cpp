#include "bootablelistdialog.h"
#include "ui_bootablelistdialog.h"

#include <QAction>
#include <QFileDialog>
#include <QGridLayout>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QPixmap>
#include <QPixmapCache>
#include <iostream>
#include <thread>

#include "AppConfig.h"
#include "CoverUtils.h"
#include "http/HttpClientFactory.h"
#include "string_format.h"
#include "QStringUtils.h"
#include "ui_shared/BootablesProcesses.h"
#include "ui_shared/BootablesDbClient.h"

#include "S3FileBrowser.h"
#include "../s3stream/AmazonS3Client.h"
#include "../s3stream/S3ObjectStream.h"
#include "ui_shared/AmazonS3Utils.h"

BootableListDialog::BootableListDialog(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::BootableListDialog)
    , m_thread_running(false)
    , m_s3_processing(false)
{
	ui->setupUi(this);
	CAppConfig::GetInstance().RegisterPreferenceInteger("ui.sortmethod", 2);
	m_sortingMethod = CAppConfig::GetInstance().GetPreferenceInteger("ui.sortmethod");
	ui->comboBox->setCurrentIndex(m_sortingMethod);

	// used as workaround to avoid direct ui access from a thread
	connect(this, SIGNAL(AsyncUpdateCoverDisplay()), this, SLOT(UpdateCoverDisplay()));

	//if m_sortingMethod == currentIndex == 0, setting index wont trigger on_comboBox_currentIndexChanged() thus resetModel()
	if(m_sortingMethod == 0)
	{
		resetModel();
	}

	CoverUtils::PopulatePlaceholderCover();
	ui->listView->setItemDelegate(new BootImageItemDelegate);

	QAction* bootgame = new QAction("Boot", ui->listView);
	QAction* removegame = new QAction("Remove", ui->listView);

	ui->listView->addAction(bootgame);
	ui->listView->addAction(removegame);
	ui->listView->setContextMenuPolicy(Qt::ActionsContextMenu);

	connect(bootgame, &QAction::triggered,
	        [&](bool) {
		        QModelIndex index = ui->listView->selectionModel()->selectedIndexes().at(0);
		        bootable = model->GetBootable(index);
		        if(!m_s3_processing)
		        {
			        accept();
		        }
		        else
		        {
			        QMessageBox::warning(this, "Warning Message",
			                             "Can't close dialog while background operation in progress.",
			                             QMessageBox::Ok, QMessageBox::Ok);
		        }
	        });
	connect(removegame, &QAction::triggered,
	        [&](bool) {
		        QModelIndex index = ui->listView->selectionModel()->selectedIndexes().at(0);
		        auto bootable = model->GetBootable(index);
		        BootablesDb::CClient::GetInstance().UnregisterBootable(bootable.path);
		        model->removeItem(index);
	        });
	m_continuationChecker = new CContinuationChecker(this);
	ui->awsS3Button->setVisible(S3FileBrowser::IsAvailable());

	SetupStatusBar();
}

BootableListDialog::~BootableListDialog()
{
	if(cover_loader.joinable())
		cover_loader.join();
	delete ui;
}

void BootableListDialog::resetModel()
{
	ui->listView->setModel(nullptr);
	if(model)
		delete model;

	m_bootables = BootablesDb::CClient::GetInstance().GetBootables(m_sortingMethod);
	model = new BootableModel(this, m_bootables);
	ui->listView->setModel(model);
	connect(ui->listView->selectionModel(), &QItemSelectionModel::currentChanged, this, &BootableListDialog::SelectionChange);

	if(!m_thread_running)
	{
		if(cover_loader.joinable())
			cover_loader.join();
		m_thread_running = true;
		cover_loader = std::thread([&] {
			CoverUtils::PopulateCache(m_bootables);

			AsyncUpdateCoverDisplay();
			m_thread_running = false;
		});
	}
}
BootablesDb::Bootable BootableListDialog::getResult()
{
	return bootable;
}

void BootableListDialog::showEvent(QShowEvent* ev)
{
	ui->horizontalLayout->invalidate();
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

void BootableListDialog::on_listView_doubleClicked(const QModelIndex& index)
{
	bootable = model->GetBootable(index);
	if(!m_s3_processing)
	{
		accept();
	}
	else
	{
		QMessageBox::warning(this, "Warning Message",
		                     "Can't close dialog while background operation in progress.",
		                     QMessageBox::Ok, QMessageBox::Ok);
	}
}

void BootableListDialog::on_refresh_button_clicked()
{
	auto bootables_paths = GetActiveBootableDirectories();
	for(auto path : bootables_paths)
	{
		try
		{
			ScanBootables(path, false);
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

void BootableListDialog::UpdateCoverDisplay()
{
	//Force redraw
	ui->listView->scroll(1, 0);
	ui->listView->scroll(-1, 0);
}

void BootableListDialog::resizeEvent(QResizeEvent* ev)
{
	model->SetWidth(ui->listView->size().width() - ui->listView->style()->pixelMetric(QStyle::PM_ScrollBarExtent) - 5);
	QDialog::resizeEvent(ev);
}

void BootableListDialog::on_awsS3Button_clicked()
{
	std::string bucketName = CAppConfig::GetInstance().GetPreferenceString("s3.filebrowser.bucketname");
	{
		bool ok;
		QString res = QInputDialog::getText(this, tr("New Function"),
		                                    tr("New Function Name:"), QLineEdit::Normal,
		                                    bucketName.c_str(), &ok);
		if(!ok || res.isEmpty())
			return;

		bucketName = res.toStdString();
	}
	if(bucketName.empty())
		return;

	m_statusBar->show();
	auto getListFuture = std::async(std::launch::async, [this, bucketName]() {
		m_s3_processing = true;
		auto accessKeyId = CS3ObjectStream::CConfig::GetInstance().GetAccessKeyId();
		auto secretAccessKey = CS3ObjectStream::CConfig::GetInstance().GetSecretAccessKey();
		AsyncUpdateStatus("Requesting S3 Bucket Content.");
		auto result = AmazonS3Utils::GetListObjects(accessKeyId, secretAccessKey, bucketName);
		auto size = result.objects.size();
		int i = 1;
		for(const auto& item : result.objects)
		{
			auto path = string_format("//s3/%s/%s", bucketName.c_str(), item.key.c_str());
			try
			{
				std::string msg = string_format("Processing: %s (%d/%d)", path.c_str(), i, size);
				AsyncUpdateStatus(msg);

				TryRegisteringBootable(path);
			}
			catch(const std::exception& exception)
			{
				//Failed to process a path, keep going
			}
			++i;
		}
		return true;
	});

	auto updateBootableCallback = [this](bool) {
		AsyncUpdateStatus("Refreshing Model.");
		resetModel();
		m_s3_processing = false;
		AsyncUpdateStatus("Complete.");
		m_statusBar->hide();
	};
	m_continuationChecker->GetContinuationManager().Register(std::move(getListFuture), updateBootableCallback);
}

void BootableListDialog::SelectionChange(const QModelIndex& index)
{
	bootable = model->GetBootable(index);
	ui->pathLineEdit->setText(bootable.path.string().c_str());
	ui->serialLineEdit->setText(bootable.discId.c_str());
}

void BootableListDialog::closeEvent(QCloseEvent* event)
{
	if(m_s3_processing)
	{
		event->ignore();
		QMessageBox::warning(this, "Warning Message",
		                     "Can't close dialog while background operation in progress.",
		                     QMessageBox::Ok, QMessageBox::Ok);
	}
	else
	{
		event->accept();
	}
}

void BootableListDialog::SetupStatusBar()
{
	m_statusBar = new QStatusBar(this);
	ui->verticalLayout_3->addWidget(m_statusBar);

	m_msgLabel = new ElidedLabel();
	m_msgLabel->setAlignment(Qt::AlignLeft);
	QFontMetrics fm(m_msgLabel->font());
	m_msgLabel->setMinimumSize(fm.boundingRect("...").size());
	m_msgLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);

	qRegisterMetaType<std::string>("std::string");
	connect(this, SIGNAL(AsyncUpdateStatus(std::string)), this, SLOT(UpdateStatus(std::string)));

	m_statusBar->addWidget(m_msgLabel, 1);
	m_statusBar->hide();
}

void BootableListDialog::UpdateStatus(std::string msg)
{
	m_msgLabel->setText(msg.c_str());
}