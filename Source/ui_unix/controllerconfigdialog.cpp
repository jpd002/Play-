#include "controllerconfigdialog.h"
#include "ui_controllerconfigdialog.h"
#include "ControllerInfo.h"

#include <QRegularExpression>
#include <QLabel>
#include <QFile>
#include <QAbstractButton>
#include <QPushButton>

#include "ui_unix/bindingmodel.h"

#include "AppConfig.h"

ControllerConfigDialog::ControllerConfigDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ControllerConfigDialog),
    m_inputManager(new CInputBindingManager(CAppConfig::GetInstance()))
{
    ui->setupUi(this);

    CBindingModel* model= new CBindingModel(this);
    model->Setup(m_inputManager);
    model->setHeaderData(0, Qt::Orientation::Horizontal, QVariant("Button"), Qt::DisplayRole);
    model->setHeaderData(1, Qt::Orientation::Horizontal, QVariant("Binding Type"), Qt::DisplayRole);
    model->setHeaderData(2, Qt::Orientation::Horizontal, QVariant("Binding Value"), Qt::DisplayRole);
    ui->tableView->setModel(model);
    ui->tableView->horizontalHeader()->setStretchLastSection(true);
    ui->tableView->resizeColumnsToContents();
}

ControllerConfigDialog::~ControllerConfigDialog()
{
    delete ui;
}

void ControllerConfigDialog::on_tableView_doubleClicked(const QModelIndex &index)
{
    static_cast<CBindingModel*>(ui->tableView->model())->DoubleClicked(index, this);
}

void ControllerConfigDialog::on_buttonBox_clicked(QAbstractButton *button)
{
    if(button == ui->buttonBox->button(QDialogButtonBox::Ok))
    {
        m_inputManager->Save();
    }
    else if (button == ui->buttonBox->button(QDialogButtonBox::Apply))
    {
        m_inputManager->Save();
    }
    else if (button == ui->buttonBox->button(QDialogButtonBox::RestoreDefaults))
    {
        switch (ui->tabWidget->currentIndex())
        {
        case 0:
            m_inputManager->AutoConfigureKeyboard();
            static_cast<CBindingModel*>(ui->tableView->model())->Refresh();
            break;
        case 1:
            //foreach (PadQWidgetExt* child, ui->tab_2->findChildren<PadQWidgetExt *>()) child->restoreDefault();
            break;
        }
    }
}
