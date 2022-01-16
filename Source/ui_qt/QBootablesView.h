#pragma once

#include <thread>
#include <QWidget>
#include <QCloseEvent>
#include <QStorageInfo>
#include "BootableModel.h"
#include "BootableModelProxy.h"
#include "ContinuationChecker.h"
#include "ElidedLabel.h"
#include "ui_shared/BootablesDbClient.h"

namespace Ui
{
	class QBootablesView;
}

class QBootablesView : public QWidget
{
	Q_OBJECT

public:
	explicit QBootablesView(QWidget* parent = 0);
	~QBootablesView();

	using Callback = std::function<void(QListView*, bool)>;

	void AddAction(std::string, Callback);
	void AddBootAction(Callback);
	void AddMsgLabel(ElidedLabel*);

Q_SIGNALS:
	void AsyncResetModel(bool);
	void AsyncUpdateCoverDisplay();
	void AsyncUpdateStatus(std::string);

protected:
	void resizeEvent(QResizeEvent*) Q_DECL_OVERRIDE;

private slots:
	void on_add_games_button_clicked();
	void on_listView_doubleClicked(const QModelIndex& index);
	void on_refresh_button_clicked();
	void on_comboBox_currentIndexChanged(int index);
	void on_reset_filter_button_clicked();
	void on_awsS3Button_clicked();

	void UpdateStatus(std::string);
	void resetModel(bool = true);

private:
	void AsyncPopulateCache();
	void DoubleClicked(const QModelIndex&);

	void SelectionChange(const QModelIndex&);
	void DisplayWarningMessage();
	void BootBootables(const QModelIndex&);

	Ui::QBootablesView* ui;
	std::vector<BootablesDb::Bootable> m_bootables;

	ElidedLabel* m_msgLabel = nullptr;
	int m_sortingMethod = 2;
	std::atomic<bool> m_s3Processing;
	BootableModelProxy* m_proxyModel = nullptr;
	CContinuationChecker* m_continuationChecker = nullptr;

	std::atomic<bool> m_threadRunning;
	std::thread m_coverLoader;
	Callback m_bootCallback;
};
