#pragma once

#include <QDialog>
#include <QDir>
#include <boost/filesystem.hpp>
#include <thread>
#include <atomic>
#include "BootableModel.h"

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

private:
	Ui::BootableListDialog* ui;

	BootablesDb::Bootable bootable;
	std::vector<BootablesDb::Bootable> m_bootables;
	BootableModel* model = nullptr;
	int m_sortingMethod = 2;
	std::thread cover_loader;
	std::atomic<bool> m_thread_running;

	void resetModel();

Q_SIGNALS:
	void AsyncUpdateCoverDisplay();

protected:
	void showEvent(QShowEvent*) Q_DECL_OVERRIDE;
};
