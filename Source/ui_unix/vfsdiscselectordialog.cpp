#include "vfsdiscselectordialog.h"
#include "ui_vfsdiscselectordialog.h"

#include <QFileDialog>
#include <QMessageBox>

VFSDiscSelectorDialog::VFSDiscSelectorDialog(std::string m_sImagePath, CDevice::BINDINGTYPE m_nBindingType, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::VFSDiscSelectorDialog)
{
    ui->setupUi(this);
    res = QString(m_sImagePath.c_str());

    refresh_disc_drive();
    if (m_nBindingType == CDevice::BINDING_IMAGE)
    {
        ui->lineEdit->setText(res);
        on_disc_image_radioButton_clicked();
        ui->disc_image_radioButton->setChecked(true);
    } else {
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
    if (dialog.exec())
    {
        res = dialog.selectedFiles().first();
        ui->lineEdit->setText(res);
    }
}

void VFSDiscSelectorDialog::on_refresh_disc_drive_button_clicked()
{
    refresh_disc_drive();
}

void VFSDiscSelectorDialog::refresh_disc_drive()
{
    discInfo.clear();
    ui->comboBox->clear();
    foreach (const QStorageInfo &storage, QStorageInfo::mountedVolumes())
    {
        if (storage.isValid() && storage.isReady())
        {
            QString FS(storage.fileSystemType());
            if (QString::compare(FS, "UDF", Qt::CaseInsensitive) == 0 || QString::compare(FS, "ISO9660", Qt::CaseInsensitive) == 0)
            {
                QString device(storage.device());
                QString name;
                if (!storage.name().isNull())
                {
                    name = storage.name();
                } else {
                    name = "Unknown";
                }
                QString item("%1 (%2)");
                ui->comboBox->addItem(item.arg(device).arg(name));
                discInfo.append(storage);
            }
        }
    }
    ui->comboBox->setEnabled(!discInfo.isEmpty());
    if (res.startsWith("////", Qt::CaseInsensitive) || res.startsWith("/dev/", Qt::CaseInsensitive))
    {
        for (int i = 0; i < discInfo.size(); i++)
        {
            QString device(discInfo.at(i).device());
            if (QString::compare(device, res, Qt::CaseInsensitive) == 0)
            {
                ui->comboBox->setCurrentIndex(i);
                break;
            }
        }
    }
}

void VFSDiscSelectorDialog::accept()
{
    if (ui->disc_image_radioButton->isChecked())
    {
        if (!ui->lineEdit->text().isEmpty())
        {
            emit onFinish(ui->lineEdit->text(), CDevice::BINDING_IMAGE);
            return QDialog::accept();
        }
    } else {
        if (ui->comboBox->currentIndex() > -1)
        {
            emit onFinish(QString(discInfo.at(ui->comboBox->currentIndex()).device()), CDevice::BINDING_PHYSICAL);
            return QDialog::accept();
        }
    }
    QMessageBox messageBox;
    messageBox.critical(this,"Error","Invalid selection, please try again.");
    messageBox.show();
}
void VFSDiscSelectorDialog::on_comboBox_currentIndexChanged(int index)
{
    if (discInfo.size() > 0){
        res = QString(discInfo.at(index).device());
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
    ui->comboBox->setEnabled(!discInfo.isEmpty());
    ui->refresh_disc_drive_button->setEnabled(true);

    ui->lineEdit->setEnabled(false);
    ui->iso_browse_button->setEnabled(false);
}
