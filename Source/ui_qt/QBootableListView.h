#pragma once

#include <thread>
#include <QListView>
#include <QStorageInfo>
#include "BootableModel.h"
#include "ui_shared/BootablesDbClient.h"


class QBootableListView : public QListView
{
	Q_OBJECT

public:
	explicit QBootableListView(QWidget* parent = 0);
	~QBootableListView();

	using Callback = std::function<void(bool)>;

	void AddAction(std::string, Callback);
	void AddBootAction(Callback);

Q_SIGNALS:
	void AsyncResetModel(bool);
	void AsyncUpdateCoverDisplay();

protected:
	void resizeEvent(QResizeEvent*) Q_DECL_OVERRIDE;

private:
	void AsyncPopulateCache();
	void DoubleClicked(const QModelIndex&);
	std::vector<BootablesDb::Bootable> m_bootables;

	std::atomic<bool> m_threadRunning;
	std::thread m_coverLoader;
	Callback m_bootCallback;

};
