#include "QBootablesView.h"
#include "ui_bootableview.h"

#include <QAction>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>

#include "AppConfig.h"
#include "CoverUtils.h"
#include "http/HttpClientFactory.h"
#include "string_format.h"
#include "QStringUtils.h"
#include "QtUtils.h"
#include "ui_shared/BootablesProcesses.h"

#ifdef HAS_AMAZON_S3
#include "S3FileBrowser.h"
#include "amazon/AmazonS3Client.h"
#include "../s3stream/S3ObjectStream.h"
#include "ui_shared/AmazonS3Utils.h"
#endif

QBootablesView::QBootablesView(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::QBootablesView)
    , m_continuationChecker(new CContinuationChecker(this))
{
	ui->setupUi(this);
	ui->listView->setStyleSheet("QListView{ background:QLinearGradient( x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #4baaf3, stop: 1 #0A0A0A); }");
	m_proxyModel = new BootableModelProxy(this);
	ui->listView->setModel(m_proxyModel);

	CAppConfig::GetInstance().RegisterPreferenceInteger("ui.sortmethod", 2);
	m_sortingMethod = CAppConfig::GetInstance().GetPreferenceInteger("ui.sortmethod");
	ui->comboBox->setCurrentIndex(m_sortingMethod);

	connect(ui->filterLineEdit, &QLineEdit::textChanged, m_proxyModel, &QSortFilterProxyModel::setFilterFixedString);
	connect(ui->stateFilterComboBox, &QComboBox::currentTextChanged, m_proxyModel, &BootableModelProxy::setFilterState);
	connect(ui->listView->selectionModel(), &QItemSelectionModel::currentChanged, this, &QBootablesView::SelectionChange);
	connect(this, &QBootablesView::AsyncUpdateStatus, this, &QBootablesView::UpdateStatus);

	// used as workaround to avoid direct ui access from a thread
	connect(this, &QBootablesView::AsyncUpdateCoverDisplay, this, &QBootablesView::UpdateCoverDisplay, Qt::QueuedConnection);
	connect(this, &QBootablesView::AsyncResetModel, this, &QBootablesView::resetModel, Qt::QueuedConnection);

	//if m_sortingMethod == currentIndex == 0, setting index wont trigger on_comboBox_currentIndexChanged() thus resetModel()
	if(m_sortingMethod == 0)
	{
		resetModel();
	}

	ui->listView->setContextMenuPolicy(Qt::ActionsContextMenu);
	ui->listView->setItemDelegate(new BootImageItemDelegate);

	CoverUtils::PopulatePlaceholderCover();

#ifdef HAS_AMAZON_S3
	ui->awsS3Button->setVisible(S3FileBrowser::IsAvailable());
#endif

	for(auto state : BootablesDb::CClient::GetInstance().GetStates())
	{
		ui->stateFilterComboBox->insertItem(ui->stateFilterComboBox->count(), QString::fromStdString(state.name));
	}
	ui->stateFilterComboBox->setVisible(ui->stateFilterComboBox->count() > 1);
}

void QBootablesView::AddMsgLabel(ElidedLabel* msgLabel)
{
	m_msgLabel = msgLabel;
}

void QBootablesView::SetupActions(BootCallback bootCallback)
{
	auto bootAction = new QAction("Boot", this);
	auto listViewBoundCallback = [this, listView = ui->listView]() {
		auto index = listView->selectionModel()->selectedIndexes().at(0);
		BootBootables(index);
	};
	connect(bootAction, &QAction::triggered, listViewBoundCallback);
	ui->listView->addAction(bootAction);
	m_bootCallback = bootCallback;

	auto removeAction = new QAction("Remove", this);
	auto removeGameCallback = [listView = ui->listView, proxyModel = m_proxyModel](bool) {
		auto index = listView->selectionModel()->selectedIndexes().at(0);
		auto src_index = proxyModel->mapToSource(index);
		auto model = static_cast<BootableModel*>(proxyModel->sourceModel());
		auto bootable = model->GetBootable(src_index);

		BootablesDb::CClient::GetInstance().UnregisterBootable(bootable.path);
		model->removeItem(index);
	};
	connect(removeAction, &QAction::triggered, removeGameCallback);
	ui->listView->addAction(removeAction);
}

void QBootablesView::AsyncPopulateCache()
{
	auto populateCoverCacheFuture = std::async(std::launch::async, [&]() {
		m_coverProcessing = true;
		CoverUtils::PopulateCache(m_bootables);
		AsyncUpdateCoverDisplay();
		return true;
	});

	m_continuationChecker->GetContinuationManager().Register(std::move(populateCoverCacheFuture), [&](bool) {
		m_coverProcessing = false;
	});
}

void QBootablesView::resizeEvent(QResizeEvent* ev)
{
	static_cast<BootableModel*>(m_proxyModel->sourceModel())->SetWidth(size().width() - style()->pixelMetric(QStyle::PM_ScrollBarExtent) - 5);
	QWidget::resizeEvent(ev);
}

void QBootablesView::resetModel(bool repopulateBootables)
{
	if(repopulateBootables)
		m_bootables = BootablesDb::CClient::GetInstance().GetBootables(m_sortingMethod);

	auto oldModel = m_proxyModel->sourceModel();
	auto model = new BootableModel(this, m_bootables);
	m_proxyModel->setSourceModel(model);

	if(oldModel)
		delete oldModel;

	AsyncPopulateCache();
}

void QBootablesView::on_add_games_button_clicked()
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
		ToggleInterface(false);
		auto filePath = QStringToPath(dialog.selectedFiles().first()).parent_path();
		auto scanBootablesFuture = std::async(std::launch::async, [&, filePath]() {
			QString msg = QString("Scanning '%1'.").arg(PathToQString(filePath));
			AsyncUpdateStatus(msg.toStdString().c_str());
			try
			{
				ScanBootables(filePath, false);
			}
			catch(...)
			{
			}
			AsyncUpdateStatus("Retrieving Game Titles.");
			FetchGameTitles();
			AsyncUpdateStatus("Downloading Game Covers.");
			FetchGameCovers();

			return true;
		});

		auto enableAddButtonCallback = [&](bool) {
			AsyncUpdateStatus("Refreshing Model.");
			AsyncResetModel(true);
			ToggleInterface(true);
			AsyncUpdateStatus("Complete.");
		};
		m_continuationChecker->GetContinuationManager().Register(std::move(scanBootablesFuture), enableAddButtonCallback);
	}
}

void QBootablesView::BootBootables(const QModelIndex& index)
{
	auto src_index = m_proxyModel->mapToSource(index);
	assert(src_index.isValid());
	auto bootable = static_cast<BootableModel*>(m_proxyModel->sourceModel())->GetBootable(src_index);
	m_bootCallback(bootable.path);
}

void QBootablesView::on_listView_doubleClicked(const QModelIndex& index)
{
	BootBootables(index);
}

void QBootablesView::on_refresh_button_clicked()
{
	ToggleInterface(false);
	auto refreshFuture = std::async(std::launch::async, [&]() {
		auto bootables_paths = GetActiveBootableDirectories();
		for(auto path : bootables_paths)
		{
			QString msg = QString("Scanning '%1'.").arg(PathToQString(path));
			AsyncUpdateStatus(msg.toStdString().c_str());
			try
			{
				ScanBootables(path, false);
			}
			catch(...)
			{
			}
		}
		AsyncUpdateStatus("Retrieving Game Titles.");
		FetchGameTitles();
		AsyncUpdateStatus("Downloading Game Covers.");
		FetchGameCovers();

		return true;
	});

	auto enableRefreshButtonCallback = [&](bool) {
		AsyncUpdateStatus("Refreshing Model.");
		AsyncResetModel(true);
		ToggleInterface(true);
		AsyncUpdateStatus("Complete.");
	};
	m_continuationChecker->GetContinuationManager().Register(std::move(refreshFuture), enableRefreshButtonCallback);
}

void QBootablesView::on_comboBox_currentIndexChanged(int index)
{
	CAppConfig::GetInstance().SetPreferenceInteger("ui.sortmethod", index);
	m_sortingMethod = index;
	resetModel();
}

void QBootablesView::on_awsS3Button_clicked()
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

	ToggleInterface(false);
	auto getListFuture = std::async(std::launch::async, [this, bucketName]() {
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
			AsyncResetModel(true);
		}
		ToggleInterface(true);
		AsyncUpdateStatus("Complete.");
	};
	m_continuationChecker->GetContinuationManager().Register(std::move(getListFuture), updateBootableCallback);
#endif
}

void QBootablesView::SelectionChange(const QModelIndex& index)
{
	auto src_index = m_proxyModel->mapToSource(index);
	if(src_index.isValid())
	{
		auto bootable = static_cast<BootableModel*>(m_proxyModel->sourceModel())->GetBootable(src_index);
		ui->pathLineEdit->setText(bootable.path.string().c_str());
		ui->serialLineEdit->setText(bootable.discId.c_str());
	}
	else
	{
		ui->pathLineEdit->clear();
		ui->serialLineEdit->clear();
	}
}

void QBootablesView::UpdateStatus(std::string msg)
{
	m_msgLabel->setText(msg.c_str());
}

void QBootablesView::DisplayWarningMessage()
{
	QMessageBox::warning(this, "Warning Message",
	                     "Can't close dialog while background operation in progress.",
	                     QMessageBox::Ok, QMessageBox::Ok);
}

void QBootablesView::on_reset_filter_button_clicked()
{
	ui->filterLineEdit->clear();
	ui->stateFilterComboBox->setCurrentIndex(0);
}

bool QBootablesView::IsProcessing()
{
	return m_coverProcessing || m_isProcessing;
}

void QBootablesView::UpdateCoverDisplay()
{
	//Force redraw
	ui->listView->update();
}

void QBootablesView::ToggleInterface(bool enable)
{
	m_isProcessing = !enable;
	ui->add_games_button->setEnabled(enable);
	ui->refresh_button->setEnabled(enable);
	ui->comboBox->setEnabled(enable);
#ifdef HAS_AMAZON_S3
	ui->awsS3Button->setEnabled(enable);
#endif
	ui->listView->setContextMenuPolicy(enable ? Qt::ActionsContextMenu : Qt::NoContextMenu);
}
