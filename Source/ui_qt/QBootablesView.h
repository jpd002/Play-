#pragma once

#include <thread>
#include <QWidget>
#include <QStorageInfo>
#include "BootableModel.h"
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

Q_SIGNALS:
	void AsyncResetModel(bool);
	void AsyncUpdateCoverDisplay();

protected:
	void resizeEvent(QResizeEvent*) Q_DECL_OVERRIDE;

private:
	void AsyncPopulateCache();
	void DoubleClicked(const QModelIndex&);

	Ui::QBootablesView* ui;
	std::vector<BootablesDb::Bootable> m_bootables;

	std::atomic<bool> m_threadRunning;
	std::thread m_coverLoader;
	Callback m_bootCallback;
};
