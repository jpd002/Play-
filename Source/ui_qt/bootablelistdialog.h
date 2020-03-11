#pragma once

#include <QResizeEvent>
#include <QDialog>
#include <QDir>
#include <QStatusBar>
#include <thread>
#include <atomic>
#include "BootableModel.h"
#include "ContinuationChecker.h"
#include "ElidedLabel.h"

namespace Ui
{
	class BootableListDialog;
}

class
    BootableListDialog : public QDialog
{
	Q_OBJECT

public:
	explicit BootableListDialog(QWidget* parent = 0);
	~BootableListDialog();

	BootablesDb::Bootable getResult();

private slots:
	void on_add_games_button_clicked();
	void on_listView_doubleClicked(const QModelIndex& index);
	void on_refresh_button_clicked();

	void on_comboBox_currentIndexChanged(int index);
	void UpdateCoverDisplay();

	void on_awsS3Button_clicked();
	void UpdateStatus(std::string);

	void resetModel(bool = true);

private:
	Ui::BootableListDialog* ui;

	QStatusBar* m_statusBar = nullptr;
	ElidedLabel* m_msgLabel = nullptr;

	BootablesDb::Bootable bootable;
	std::vector<BootablesDb::Bootable> m_bootables;
	BootableModel* model = nullptr;
	int m_sortingMethod = 2;
	std::thread cover_loader;
	std::atomic<bool> m_thread_running;
	std::atomic<bool> m_s3_processing;
	CContinuationChecker* m_continuationChecker = nullptr;

	void SelectionChange(const QModelIndex&);
	void SetupStatusBar();

Q_SIGNALS:
	void AsyncResetModel(bool);
	void AsyncUpdateCoverDisplay();
	void AsyncUpdateStatus(std::string);

protected:
	void showEvent(QShowEvent*) Q_DECL_OVERRIDE;
	void resizeEvent(QResizeEvent*) Q_DECL_OVERRIDE;
	void closeEvent(QCloseEvent*) Q_DECL_OVERRIDE;
};
