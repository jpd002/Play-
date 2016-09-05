#ifndef VFSMANAGERDIALOG_H
#define VFSMANAGERDIALOG_H

#include <QDialog>
#include "VfsDevice.h"

namespace Ui {
class VFSManagerDialog;
}

class VFSManagerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit VFSManagerDialog(QWidget *parent = 0);
    ~VFSManagerDialog();
    Ui::VFSManagerDialog *ui;
    void UpdateList();

protected:
    void accept() Q_DECL_OVERRIDE;

private slots:
    void on_tableView_doubleClicked(const QModelIndex &index);

private:
    CDevice::DeviceList m_devices;
};

#endif // VFSMANAGERDIALOG_H
