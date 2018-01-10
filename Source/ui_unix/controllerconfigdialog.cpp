#include "controllerconfigdialog.h"
#include "ui_controllerconfigdialog.h"

#include <QRegularExpression>
#include <QLabel>
#include <QFile>
#include <QAbstractButton>
#include <QPushButton>

#include "bindingmodel.h"
#include "ControllerInfo.h"
#include "inputeventselectiondialog.h"

ControllerConfigDialog::ControllerConfigDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::ControllerConfigDialog),
	m_inputDeviceManager(std::make_unique<CGamePadDeviceListener>(true))
{
	ui->setupUi(this);
}

ControllerConfigDialog::~ControllerConfigDialog()
{
	delete ui;
}

void ControllerConfigDialog::SetInputBindingManager(CInputBindingManager *inputBindingManager)
{
	m_inputManager = inputBindingManager;
	CBindingModel* model= new CBindingModel(this);
	model->Setup(m_inputManager);
	model->setHeaderData(0, Qt::Orientation::Horizontal, QVariant("Button"), Qt::DisplayRole);
	model->setHeaderData(1, Qt::Orientation::Horizontal, QVariant("Binding Type"), Qt::DisplayRole);
	model->setHeaderData(2, Qt::Orientation::Horizontal, QVariant("Binding Value"), Qt::DisplayRole);
	ui->tableView->setModel(model);
	ui->tableView->horizontalHeader()->setStretchLastSection(true);
	ui->tableView->resizeColumnsToContents();
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
		}
	}
	else if (button == ui->buttonBox->button(QDialogButtonBox::Cancel))
	{
		m_inputManager->Load();
	}
}

void ControllerConfigDialog::on_tableView_doubleClicked(const QModelIndex &index)
{
	OpenBindConfigDialog(index.row());
}

void ControllerConfigDialog::on_ConfigAllButton_clicked()
{
	for(int i = 0; i < PS2::CControllerInfo::MAX_BUTTONS; ++i)
	{
		if(!OpenBindConfigDialog(i))
		{
			break;
		}
	}
}

int ControllerConfigDialog::OpenBindConfigDialog(int index)
{
	std::string button(PS2::CControllerInfo::m_buttonName[index]);
	std::transform(button.begin(), button.end(),button.begin(), ::toupper);

	InputEventSelectionDialog IESD;
	IESD.Setup(button.c_str(), m_inputManager,
			   static_cast<PS2::CControllerInfo::BUTTON>(index), m_inputDeviceManager);
	auto res = IESD.exec();
	m_inputDeviceManager.get()->DisconnectInputEventCallback();
	return res;
}
