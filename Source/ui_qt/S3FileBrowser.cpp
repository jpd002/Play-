#include "S3FileBrowser.h"
#include "ui_s3filebrowser.h"
#include "amazon/AmazonS3Client.h"
#include "../s3stream/S3ObjectStream.h"
#include "string_format.h"
#include "AppConfig.h"
#include "ui_shared/AmazonS3Utils.h"

#define PREF_S3FILEBROWSER_BUCKETNAME "s3.filebrowser.bucketname"

S3FileBrowser::S3FileBrowser(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::S3FileBrowser())
{
	CAppConfig::GetInstance().RegisterPreferenceString(PREF_S3FILEBROWSER_BUCKETNAME, "");

	ui->setupUi(this);
	ui->bucketNameEdit->setText(CAppConfig::GetInstance().GetPreferenceString(PREF_S3FILEBROWSER_BUCKETNAME));

	m_continuationChecker = new CContinuationChecker(this);

	m_filterTimer = new QTimer(this);
	m_filterTimer->setSingleShot(true);
	connect(m_filterTimer, SIGNAL(timeout()), this, SLOT(updateFilter()));

	launchUpdate();
}

S3FileBrowser::~S3FileBrowser()
{
	delete ui;
}

bool S3FileBrowser::IsAvailable()
{
	return CS3ObjectStream::CConfig::GetInstance().GetConfigs().IsValid();
}

fs::path S3FileBrowser::GetSelectedPath() const
{
	return m_selectedPath;
}

void S3FileBrowser::on_refreshButton_clicked()
{
	launchUpdate();
}

void S3FileBrowser::on_objectList_itemSelectionChanged()
{
	updateOkButtonState();
}

void S3FileBrowser::on_searchFilterEdit_textChanged(QString)
{
	m_filterTimer->start(100);
}

void S3FileBrowser::updateFilter()
{
	ui->objectList->clear();
	auto filter = ui->searchFilterEdit->text().toStdString();
	for(const auto& item : m_bucketItems.objects)
	{
		if(!filter.empty() && (item.key.find(filter, 0) == std::string::npos)) continue;
		ui->objectList->addItem(QString::fromStdString(item.key));
	}
}

void S3FileBrowser::accept()
{
	auto bucketName = m_lastUpdateBucketName.toStdString();
	auto selectedItems = ui->objectList->selectedItems();
	if(selectedItems.size() != 0)
	{
		auto selectedItem = selectedItems[0];
		auto objectName = selectedItem->text().toStdString();
		m_selectedPath = string_format("//s3/%s/%s", bucketName.c_str(), objectName.c_str());
	}
	CAppConfig::GetInstance().SetPreferenceString(PREF_S3FILEBROWSER_BUCKETNAME, bucketName.c_str());
	CAppConfig::GetInstance().Save();
	QDialog::accept();
}

void S3FileBrowser::updateOkButtonState()
{
	bool hasSelection = ui->objectList->selectedItems().size() != 0;
	ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(hasSelection);
}

void S3FileBrowser::launchUpdate()
{
	setEnabled(false);
	m_lastUpdateBucketName = ui->bucketNameEdit->text();
	ui->objectList->clear();
	updateOkButtonState();

	auto getListFuture = std::async([bucketName = m_lastUpdateBucketName.toStdString()]() {
		auto credentials = CS3ObjectStream::CConfig::GetInstance().GetConfigs();
		return AmazonS3Utils::GetListObjects(credentials, bucketName);
	});
	m_continuationChecker->GetContinuationManager().Register(std::move(getListFuture),
	                                                         [this](auto& result) {
		                                                         m_bucketItems = result;
		                                                         this->updateFilter();
		                                                         this->setEnabled(true);
	                                                         });
}
