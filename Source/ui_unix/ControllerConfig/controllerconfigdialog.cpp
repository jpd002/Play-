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

ControllerConfigDialog::ControllerConfigDialog(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::ControllerConfigDialog)
{
	ui->setupUi(this);
}

ControllerConfigDialog::~ControllerConfigDialog()
{
	delete ui;
}

#if defined(HAS_LIBEVDEV) || defined(__APPLE__)
void ControllerConfigDialog::SetInputBindingManager(CInputBindingManager* inputBindingManager, CGamePadDeviceListener* inputDeviceManager)
{
	m_inputDeviceManager = inputDeviceManager;
#else
void ControllerConfigDialog::SetInputBindingManager(CInputBindingManager* inputBindingManager)
{
#endif
	m_inputManager = inputBindingManager;
	CBindingModel* model = new CBindingModel(this);
	model->Setup(m_inputManager);
	model->setHeaderData(0, Qt::Orientation::Horizontal, QVariant("Button"), Qt::DisplayRole);
	model->setHeaderData(1, Qt::Orientation::Horizontal, QVariant("Binding Type"), Qt::DisplayRole);
	model->setHeaderData(2, Qt::Orientation::Horizontal, QVariant("Binding Value"), Qt::DisplayRole);
	ui->tableView->setModel(model);
	ui->tableView->horizontalHeader()->setStretchLastSection(true);
	ui->tableView->resizeColumnsToContents();
}

void ControllerConfigDialog::on_buttonBox_clicked(QAbstractButton* button)
{
	if(button == ui->buttonBox->button(QDialogButtonBox::Ok))
	{
		m_inputManager->Save();
	}
	else if(button == ui->buttonBox->button(QDialogButtonBox::Apply))
	{
		m_inputManager->Save();
	}
	else if(button == ui->buttonBox->button(QDialogButtonBox::RestoreDefaults))
	{
		switch(ui->tabWidget->currentIndex())
		{
		case 0:
			m_inputManager->AutoConfigureKeyboard();
			static_cast<CBindingModel*>(ui->tableView->model())->Refresh();
			break;
		}
	}
	else if(button == ui->buttonBox->button(QDialogButtonBox::Cancel))
	{
		m_inputManager->Load();
	}
}

void ControllerConfigDialog::on_tableView_doubleClicked(const QModelIndex& index)
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
	std::transform(button.begin(), button.end(), button.begin(), ::toupper);

	InputEventSelectionDialog IESD;
	IESD.Setup(button.c_str(), m_inputManager, static_cast<PS2::CControllerInfo::BUTTON>(index));
#if defined(HAS_LIBEVDEV) || defined(__APPLE__)
	IESD.SetupInputDeviceManager(m_inputDeviceManager);
#endif
	auto res = IESD.exec();
#if defined(HAS_LIBEVDEV) || defined(__APPLE__)
	m_inputDeviceManager->DisconnectInputEventCallback();
#endif
	return res;
}
