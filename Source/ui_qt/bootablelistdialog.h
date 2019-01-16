#pragma once

#include <QDialog>
#include <QDir>
#include <boost/filesystem.hpp>
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
	void on_listView_doubleClicked(const QModelIndex &index);
	void on_refresh_button_clicked();

	void on_comboBox_currentIndexChanged(int index);

private:
	Ui::BootableListDialog* ui;

	BootablesDb::Bootable bootable;
	BootableModel* model = nullptr;
	int m_sortingMethod = 2;

	void resetModel();
protected:
	void showEvent(QShowEvent*) Q_DECL_OVERRIDE;


};
