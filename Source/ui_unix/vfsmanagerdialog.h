#pragma once

#include "VfsDevice.h"
#include <QDialog>

namespace Ui
{
	class VFSManagerDialog;
}

class VFSManagerDialog : public QDialog
{
	Q_OBJECT

public:
	explicit VFSManagerDialog(QWidget* parent = nullptr);
	~VFSManagerDialog();

	Ui::VFSManagerDialog* ui;
	void UpdateList();

protected:
	void accept() Q_DECL_OVERRIDE;

private slots:
	void on_tableView_doubleClicked(const QModelIndex& index);

private:
	DeviceList m_devices;
};
