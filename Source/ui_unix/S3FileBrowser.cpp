#include "S3FileBrowser.h"
#include "ui_S3FileBrowser.h"
#include "../s3stream/AmazonS3Client.h"
#include "../s3stream/S3ObjectStream.h"
#include "string_format.h"

S3FileBrowser::S3FileBrowser(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::S3FileBrowser())
{
	ui->setupUi(this);

	m_continuationChecker = new CContinuationChecker(this);

	launchUpdate();
}

S3FileBrowser::~S3FileBrowser()
{
	delete ui;
}

boost::filesystem::path S3FileBrowser::GetSelectedPath() const
{
	return m_selectedPath;
}

void S3FileBrowser::refreshButton_clicked()
{
	launchUpdate();
}

void S3FileBrowser::objectList_itemSelectionChanged()
{
	updateOkButtonState();
}

void S3FileBrowser::accept()
{
	auto selectedItems = ui->objectList->selectedItems();
	if(selectedItems.size() != 0)
	{
		auto selectedItem = selectedItems[0];
		auto bucketName = m_lastUpdateBucketName.toStdString();
		auto objectName = selectedItem->text().toStdString();
		m_selectedPath = string_format("s3://%s/%s", bucketName.c_str(), objectName.c_str());
	}
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

	auto getListFuture = std::async(
	    [bucketName = m_lastUpdateBucketName.toStdString()]() {
		    std::string bucketRegion;

		    //Obtain bucket region
		    try
		    {
			    {
				    CAmazonS3Client client(
						CS3ObjectStream::CConfig::GetInstance().GetAccessKeyId(),
						CS3ObjectStream::CConfig::GetInstance().GetSecretAccessKey());

				    GetBucketLocationRequest request;
				    request.bucket = bucketName;

				    auto result = client.GetBucketLocation(request);
				    bucketRegion = result.locationConstraint;
			    }

			    //List objects
			    CAmazonS3Client client(
					CS3ObjectStream::CConfig::GetInstance().GetAccessKeyId(),
					CS3ObjectStream::CConfig::GetInstance().GetSecretAccessKey(),
					bucketRegion);
			    return client.ListObjects(bucketName);
		    }
		    catch(...)
		    {
			    return ListObjectsResult();
		    }
	    });
	m_continuationChecker->GetContinuationManager().Register(std::move(getListFuture),
	                                                         [this](auto& result) {
		                                                         for(const auto& resultItem : result.objects)
		                                                         {
			                                                         ui->objectList->addItem(QString::fromStdString(resultItem.key));
		                                                         }
		                                                         setEnabled(true);
	                                                         });
}
