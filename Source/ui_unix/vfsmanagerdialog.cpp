#include "vfsmanagerdialog.h"
#include "ui_vfsmanagerdialog.h"
#include "vfsmodel.h"

#include <QStandardItemModel>
#include "AppConfig.h"
#include "PS2VM_Preferences.h"

VFSManagerDialog::VFSManagerDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::VFSManagerDialog)
{
    ui->setupUi(this);
    VFSModel* model= new VFSModel(this);
    model->setHeaderData(1, Qt::Orientation::Horizontal, QVariant("Device"), Qt::DisplayRole);
    model->setHeaderData(1, Qt::Orientation::Horizontal, QVariant("Binding Type"), Qt::DisplayRole);
    model->setHeaderData(1, Qt::Orientation::Horizontal, QVariant("Binding Value"), Qt::DisplayRole);
    ui->tableView->setModel(model);
    ui->tableView->horizontalHeader()->setStretchLastSection(true);
    ui->tableView->resizeColumnsToContents();

}

VFSManagerDialog::~VFSManagerDialog()
{
    delete ui;
}

void VFSManagerDialog::on_tableView_doubleClicked(const QModelIndex &index)
{
    VFSModel* model = static_cast<VFSModel*>(ui->tableView->model());
    model->DoubleClicked(index, this);
}


void VFSManagerDialog::accept()
{
    VFSModel* model = static_cast<VFSModel*>(ui->tableView->model());
    model->Save();
    CAppConfig::GetInstance().Save();
    QDialog::accept();
}
