#pragma once

#include <QDialog>
#include <QListWidgetItem>
#include "amazon/AmazonS3Client.h"
#include "filesystem_def.h"
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

	static bool IsAvailable();

	fs::path GetSelectedPath() const;

private slots:
	void on_refreshButton_clicked();
	void on_objectList_itemSelectionChanged();
	void on_searchFilterEdit_textChanged(QString);
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
	fs::path m_selectedPath;
};
