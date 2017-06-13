#include "controllerconfigdialog.h"
#include "ui_controllerconfigdialog.h"

#include <QRegularExpression>
#include <QLabel>
#include <QFile>
#include <QAbstractButton>
#include <QPushButton>

#include "AppConfig.h"
#include "bindingmodel.h"
#include "ControllerInfo.h"
#include "inputeventselectiondialog.h"

ControllerConfigDialog::ControllerConfigDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::ControllerConfigDialog),
	m_inputManager(new CInputBindingManager(CAppConfig::GetInstance())),
	m_inputDeviceManager(std::make_unique<CGamePadDeviceListener>(true))
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
}

void ControllerConfigDialog::on_tableView_doubleClicked(const QModelIndex &index)
{
	std::string button(PS2::CControllerInfo::m_buttonName[index.row()]);
	std::transform(button.begin(), button.end(),button.begin(), ::toupper);

	InputEventSelectionDialog IESD;
	IESD.Setup(button.c_str(), m_inputManager,
			   static_cast<PS2::CControllerInfo::BUTTON>(index.row()), m_inputDeviceManager);
	IESD.exec();
	m_inputDeviceManager.get()->DisconnectInputEventCallback();
}

void ControllerConfigDialog::on_ConfigAllButton_clicked()
{
	for(int i = 0; i < PS2::CControllerInfo::MAX_BUTTONS; ++i)
	{
		std::string button(PS2::CControllerInfo::m_buttonName[i]);
		std::transform(button.begin(), button.end(),button.begin(), ::toupper);

		InputEventSelectionDialog IESD;
		IESD.Setup(button.c_str(), m_inputManager,
				   static_cast<PS2::CControllerInfo::BUTTON>(i), m_inputDeviceManager);
		if(IESD.exec())
		{
			m_inputDeviceManager.get()->DisconnectInputEventCallback();
			std::this_thread::sleep_for(std::chrono::seconds(2));
		}
		else
		{
			m_inputDeviceManager.get()->DisconnectInputEventCallback();
			break;
		}
	}
}
