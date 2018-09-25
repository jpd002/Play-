#include "vfsdiscselectordialog.h"
#include "ui_vfsdiscselectordialog.h"

#include <QFileDialog>
#include <QMessageBox>
#include "QStringUtils.h"

VFSDiscSelectorDialog::VFSDiscSelectorDialog(boost::filesystem::path path, CCdrom0Device::BINDINGTYPE m_nBindingType, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::VFSDiscSelectorDialog)
    , m_path(path)
{
	ui->setupUi(this);

	Refresh_disc_drive();
	if(m_nBindingType == CCdrom0Device::BINDING_IMAGE)
	{
		ui->lineEdit->setText(PathToQString(m_path));
		on_disc_image_radioButton_clicked();
		ui->disc_image_radioButton->setChecked(true);
	}
	else
	{
		on_cd_device_radioButton_clicked();
		ui->cd_device_radioButton->setChecked(true);
	}
}

VFSDiscSelectorDialog::~VFSDiscSelectorDialog()
{
	delete ui;
}

void VFSDiscSelectorDialog::on_iso_browse_button_clicked()
{
	QFileDialog dialog(this);
	dialog.setFileMode(QFileDialog::ExistingFile);
	dialog.setNameFilter(tr("All supported types(*.iso *.bin *.isz *.cso);;UltraISO Compressed Disk Images (*.isz);;CISO Compressed Disk Images (*.cso);;All files (*.*)"));
	if(dialog.exec())
	{
		m_path = QStringToPath(dialog.selectedFiles().first());
		ui->lineEdit->setText(PathToQString(m_path));
	}
}

void VFSDiscSelectorDialog::on_refresh_disc_drive_button_clicked()
{
	Refresh_disc_drive();
}

void VFSDiscSelectorDialog::Refresh_disc_drive()
{
	m_discInfo.clear();
	ui->comboBox->clear();
	foreach(const QStorageInfo& storage, QStorageInfo::mountedVolumes())
	{
		if(storage.isValid() && storage.isReady())
		{
			QString FS(storage.fileSystemType());
			if(QString::compare(FS, "UDF", Qt::CaseInsensitive) == 0 || QString::compare(FS, "ISO9660", Qt::CaseInsensitive) == 0)
			{
				QString device(storage.device());
				QString name;
				if(!storage.name().isNull())
				{
					name = storage.name();
				}
				else
				{
					name = "Unknown";
				}
				QString item("%1 (%2)");
				ui->comboBox->addItem(item.arg(device).arg(name));
				m_discInfo.append(storage);
			}
		}
	}
	ui->comboBox->setEnabled(!m_discInfo.isEmpty());
	{
		auto pathString = PathToQString(m_path);
		if(
		    pathString.startsWith("////", Qt::CaseInsensitive) ||
		    pathString.startsWith("/dev/", Qt::CaseInsensitive))
		{
			for(int i = 0; i < m_discInfo.size(); i++)
			{
				QString device(m_discInfo.at(i).device());
				if(QString::compare(device, pathString, Qt::CaseInsensitive) == 0)
				{
					ui->comboBox->setCurrentIndex(i);
					break;
				}
			}
		}
	}
}

void VFSDiscSelectorDialog::accept()
{
	if(ui->disc_image_radioButton->isChecked())
	{
		if(!ui->lineEdit->text().isEmpty())
		{
			emit onFinish(ui->lineEdit->text(), CCdrom0Device::BINDING_IMAGE);
			return QDialog::accept();
		}
	}
	else
	{
		if(ui->comboBox->currentIndex() > -1)
		{
			emit onFinish(QString(m_discInfo.at(ui->comboBox->currentIndex()).device()), CCdrom0Device::BINDING_PHYSICAL);
			return QDialog::accept();
		}
	}
	QMessageBox messageBox;
	messageBox.critical(this, "Error", "Invalid selection, please try again.");
	messageBox.show();
}

void VFSDiscSelectorDialog::on_comboBox_currentIndexChanged(int index)
{
	if(m_discInfo.size() > 0)
	{
		m_path = QStringToPath(m_discInfo.at(index).device());
	}
}

void VFSDiscSelectorDialog::on_disc_image_radioButton_clicked()
{
	ui->lineEdit->setEnabled(true);
	ui->iso_browse_button->setEnabled(true);

	ui->comboBox->setEnabled(false);
	ui->refresh_disc_drive_button->setEnabled(false);
}

void VFSDiscSelectorDialog::on_cd_device_radioButton_clicked()
{
	ui->comboBox->setEnabled(!m_discInfo.isEmpty());
	ui->refresh_disc_drive_button->setEnabled(true);

	ui->lineEdit->setEnabled(false);
	ui->iso_browse_button->setEnabled(false);
}
