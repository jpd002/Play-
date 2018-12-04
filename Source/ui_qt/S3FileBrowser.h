#pragma once

#include <QDialog>
#include <QListWidgetItem>
#undef DELETE
#include "../s3stream/AmazonS3Client.h"
#include "boost_filesystem_def.h"
#include "ContinuationChecker.h"

namespace Ui
{
	class S3FileBrowser;
}

class S3FileBrowser : public QDialog
{
	Q_OBJECT

public:
	explicit S3FileBrowser(QWidget* parent = 0);
	~S3FileBrowser();

	boost::filesystem::path GetSelectedPath() const;

private slots:
	void refreshButton_clicked();
	void objectList_itemSelectionChanged();
	void searchFilterEdit_textChanged(QString);
	void updateFilter();

private:
	void accept() Q_DECL_OVERRIDE;
	void updateOkButtonState();
	void launchUpdate();

	Ui::S3FileBrowser* ui = nullptr;
	CContinuationChecker* m_continuationChecker = nullptr;
	QString m_lastUpdateBucketName;
	QTimer* m_filterTimer = nullptr;

	ListObjectsResult m_bucketItems;
	boost::filesystem::path m_selectedPath;
};
