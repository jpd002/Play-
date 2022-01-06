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
#include "QtUtils.h"
#include "ui_shared/BootablesProcesses.h"
#include "ui_shared/BootablesDbClient.h"

#ifdef HAS_AMAZON_S3
#include "S3FileBrowser.h"
#include "amazon/AmazonS3Client.h"
#include "../s3stream/S3ObjectStream.h"
#include "ui_shared/AmazonS3Utils.h"
#endif

BootableListDialog::BootableListDialog(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::BootableListDialog)
    , m_thread_running(false)
    , m_s3_processing(false)
{
	ui->setupUi(this);
	m_proxy_model = new BootableModelProxy(this);
	ui->listView->setModel(m_proxy_model);

	CAppConfig::GetInstance().RegisterPreferenceInteger("ui.sortmethod", 2);
	m_sortingMethod = CAppConfig::GetInstance().GetPreferenceInteger("ui.sortmethod");
	ui->comboBox->setCurrentIndex(m_sortingMethod);

	connect(ui->filterLineEdit, &QLineEdit::textChanged, m_proxy_model, &QSortFilterProxyModel::setFilterFixedString);
	connect(ui->listView->selectionModel(), &QItemSelectionModel::currentChanged, this, &BootableListDialog::SelectionChange);

	// used as workaround to avoid direct ui access from a thread
	connect(this, SIGNAL(AsyncUpdateCoverDisplay()), this, SLOT(UpdateCoverDisplay()));
	connect(this, SIGNAL(AsyncResetModel(bool)), this, SLOT(resetModel(bool)));

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
		        BootBootables(index);
	        });
	connect(removegame, &QAction::triggered,
	        [&](bool) {
		        QModelIndex index = ui->listView->selectionModel()->selectedIndexes().at(0);
		        auto src_index = m_proxy_model->mapToSource(index);
		        auto bootable = model->GetBootable(src_index);
		        BootablesDb::CClient::GetInstance().UnregisterBootable(bootable.path);
		        model->removeItem(src_index);
	        });
	m_continuationChecker = new CContinuationChecker(this);
#ifdef HAS_AMAZON_S3
	ui->awsS3Button->setVisible(S3FileBrowser::IsAvailable());
#endif

	SetupStatusBar();
}

BootableListDialog::~BootableListDialog()
{
	if(cover_loader.joinable())
		cover_loader.join();
	delete ui;
}

void BootableListDialog::resetModel(bool repopulateBootables)
{
	auto old_model = model;

	if(repopulateBootables)
		m_bootables = BootablesDb::CClient::GetInstance().GetBootables(m_sortingMethod);

	model = new BootableModel(this, m_bootables);
	m_proxy_model->setSourceModel(model);

	if(old_model)
		delete old_model;

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
	QStringList filters;
	filters.push_back(QtUtils::GetDiscImageFormatsFilter());
	filters.push_back("ELF files (*.elf)");
	filters.push_back("All files (*)");

	QFileDialog dialog(this);
	dialog.setFileMode(QFileDialog::ExistingFile);
	dialog.setNameFilters(filters);
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

void BootableListDialog::BootBootables(const QModelIndex& index)
{
	auto src_index = m_proxy_model->mapToSource(index);
	assert(src_index.isValid());
	bootable = model->GetBootable(src_index);
	if(!m_s3_processing)
	{
		accept();
	}
	else
	{
		DisplayWarningMessage();
	}
}

void BootableListDialog::on_listView_doubleClicked(const QModelIndex& index)
{
	BootBootables(index);
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
#ifdef HAS_AMAZON_S3
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
		auto credentials = CS3ObjectStream::CConfig::GetInstance().GetCredentials();
		AsyncUpdateStatus("Requesting S3 Bucket Content.");
		auto result = AmazonS3Utils::GetListObjects(credentials, bucketName);
		auto size = result.objects.size();
		int i = 1;
		bool new_entry = false;
		for(const auto& item : result.objects)
		{
			auto path = string_format("//s3/%s/%s", bucketName.c_str(), item.key.c_str());
			try
			{
				std::string msg = string_format("Processing: %s (%d/%d)", path.c_str(), i, size);
				AsyncUpdateStatus(msg);
				new_entry |= TryRegisterBootable(path);
			}
			catch(const std::exception& exception)
			{
				//Failed to process a path, keep going
			}
			++i;
		}
		return new_entry;
	});

	auto updateBootableCallback = [this](bool new_entry) {
		if(new_entry)
		{
			AsyncUpdateStatus("Refreshing Model.");
			resetModel();
		}
		m_s3_processing = false;
		AsyncUpdateStatus("Complete.");
		m_statusBar->hide();
	};
	m_continuationChecker->GetContinuationManager().Register(std::move(getListFuture), updateBootableCallback);
#endif
}

void BootableListDialog::SelectionChange(const QModelIndex& index)
{
	auto src_index = static_cast<QSortFilterProxyModel*>(ui->listView->model())->mapToSource(index);
	if(src_index.isValid())
	{
		bootable = model->GetBootable(src_index);
		ui->pathLineEdit->setText(bootable.path.string().c_str());
		ui->serialLineEdit->setText(bootable.discId.c_str());
	}
	else
	{
		bootable = BootablesDb::Bootable();
		ui->pathLineEdit->clear();
		ui->serialLineEdit->clear();
	}
}

void BootableListDialog::closeEvent(QCloseEvent* event)
{
	if(m_s3_processing)
	{
		event->ignore();
		DisplayWarningMessage();
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

void BootableListDialog::DisplayWarningMessage()
{
	QMessageBox::warning(this, "Warning Message",
	                     "Can't close dialog while background operation in progress.",
	                     QMessageBox::Ok, QMessageBox::Ok);
}

void BootableListDialog::on_reset_filter_button_clicked()
{
	ui->filterLineEdit->clear();
}
