#pragma once

#include <QDialog>
#include <QStorageInfo>

#include "VfsDevice.h"

namespace Ui
{
class VFSDiscSelectorDialog;
}

class VFSDiscSelectorDialog : public QDialog
{
	Q_OBJECT

public:
	explicit VFSDiscSelectorDialog(std::string, CCdrom0Device::BINDINGTYPE, QWidget* parent = 0);
	~VFSDiscSelectorDialog();

protected:
	void accept() Q_DECL_OVERRIDE;

private slots:
	void on_iso_browse_button_clicked();
	void on_refresh_disc_drive_button_clicked();
	void on_comboBox_currentIndexChanged(int index);
	void on_disc_image_radioButton_clicked();
	void on_cd_device_radioButton_clicked();

signals:
	void onFinish(QString, CCdrom0Device::BINDINGTYPE);

private:
	void Refresh_disc_drive();

	Ui::VFSDiscSelectorDialog* ui;
	QString                    m_path;
	QList<QStorageInfo>        m_discInfo;
};
