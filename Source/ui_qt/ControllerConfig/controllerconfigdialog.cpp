#include "controllerconfigdialog.h"
#include "ui_controllerconfigdialog.h"

#include <QRegularExpression>
#include <QLabel>
#include <QFile>
#include <QAbstractButton>
#include <QPushButton>
#include <QInputDialog>
#include <QShortcut>
#include <QMessageBox>

#include "AppConfig.h"
#include "PreferenceDefs.h"
#include "PathUtils.h"
#include "inputbindingmodel.h"
#include "ControllerInfo.h"
#include "inputeventselectiondialog.h"
#include "QStringUtils.h"

#include "filesystem_def.h"
#include <iostream>
#include <cassert>

#define ANALOG_SENSITIVITY_MIN (0.75f)
#define ANALOG_SENSITIVITY_MAX (1.25f)
#define ANALOG_SENSITIVITY_SCALE (1000.f)

ControllerConfigDialog::ControllerConfigDialog(CInputBindingManager* inputBindingManager, CInputProviderQtKey* qtKeyInputProvider, CInputProviderQtMouse* qtMouseInputProvider, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::ControllerConfigDialog)
    , m_inputManager(inputBindingManager)
    , m_qtKeyInputProvider(qtKeyInputProvider)
    , m_qtMouseInputProvider(qtMouseInputProvider)
{
	ui->setupUi(this);

	m_padUiElements.push_back({ui->pad1TableView, ui->pad1AnalogSensitivitySlider, ui->pad1AnalogSensitivityValueLabel, ui->pad1hapticFeedbackComboBox});
	m_padUiElements.push_back({ui->pad2TableView, ui->pad2AnalogSensitivitySlider, ui->pad2AnalogSensitivityValueLabel, ui->pad2hapticFeedbackComboBox});
	m_padUiElements.push_back({ui->pad3TableView, ui->pad3AnalogSensitivitySlider, ui->pad3AnalogSensitivityValueLabel, ui->pad3hapticFeedbackComboBox});
	m_padUiElements.push_back({ui->pad4TableView, ui->pad4AnalogSensitivitySlider, ui->pad4AnalogSensitivityValueLabel, ui->pad4hapticFeedbackComboBox});

	auto devices = m_inputManager->GetDevices();
	for(uint32 padIndex = 0; padIndex < m_padUiElements.size(); padIndex++)
	{
		PrepareBindingsView(padIndex);

		const auto& uiElements = m_padUiElements[padIndex];

		uiElements.analogSensitivitySlider->setMinimum(ANALOG_SENSITIVITY_MIN * ANALOG_SENSITIVITY_SCALE);
		uiElements.analogSensitivitySlider->setMaximum(ANALOG_SENSITIVITY_MAX * ANALOG_SENSITIVITY_SCALE);
		uiElements.analogSensitivitySlider->setValue(m_inputManager->GetAnalogSensitivity(padIndex) * ANALOG_SENSITIVITY_SCALE);
		UpdateAnalogSensitivityValueLabel(padIndex);

		QShortcut* shortcut = new QShortcut(QKeySequence(Qt::Key_Delete), uiElements.bindingsView);
		connect(shortcut, SIGNAL(activated()), this, SLOT(bindingsViewDeleteItem()));

		QObject::connect(uiElements.analogSensitivitySlider, &QSlider::sliderMoved, this, [this, padIndex](int value) { analogSensitivityValueChanged(padIndex, value); });
		QObject::connect(uiElements.analogSensitivitySlider, &QSlider::valueChanged, this, [this, padIndex](int value) { analogSensitivityValueChanged(padIndex, value); });
		QObject::connect(uiElements.bindingsView, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(bindingsViewDoubleClicked(const QModelIndex&)));

		auto motorBinding = inputBindingManager->GetMotorBinding(padIndex);
		uiElements.hapticFeedbackComboBox->addItem("Disabled");

		for(auto index = 0; index < devices.size(); ++index)
		{
			auto devInfo = devices[index];
			uiElements.hapticFeedbackComboBox->addItem(devInfo.name.c_str());
			if(!motorBinding)
				continue;

			auto targetBinding = BINDINGTARGET(devInfo.providerId, devInfo.deviceId, -1, BINDINGTARGET::KEYTYPE::MOTOR);
			if(motorBinding->GetBindingTarget() == targetBinding)
			{
				uiElements.hapticFeedbackComboBox->setCurrentIndex(index + 1);
			}
		}

		QObject::connect(uiElements.hapticFeedbackComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
		                 [this, padIndex, inputBindingManager, devices](int index) {
			                 if(index == 0)
			                 {
				                 inputBindingManager->SetMotorBinding(padIndex, {});
				                 return;
			                 }
			                 auto devInfo = devices[index - 1];
			                 auto targetBinding = BINDINGTARGET(devInfo.providerId, devInfo.deviceId, -1, BINDINGTARGET::KEYTYPE::MOTOR);
			                 inputBindingManager->SetMotorBinding(padIndex, targetBinding);
			                 inputBindingManager->GetMotorBinding(padIndex)->ProcessEvent(0, 1);
		                 });
	}
	PrepareProfiles();
}

ControllerConfigDialog::~ControllerConfigDialog()
{
	delete ui;
}

void ControllerConfigDialog::PrepareBindingsView(uint32 padIndex)
{
	CInputBindingModel* model = new CInputBindingModel(this, m_inputManager, padIndex);
	model->setHeaderData(0, Qt::Orientation::Horizontal, QVariant("Button"), Qt::DisplayRole);
	model->setHeaderData(1, Qt::Orientation::Horizontal, QVariant("Binding Type"), Qt::DisplayRole);
	model->setHeaderData(2, Qt::Orientation::Horizontal, QVariant("Binding Value"), Qt::DisplayRole);
	assert(padIndex < m_padUiElements.size());
	auto bindingsView = m_padUiElements[padIndex].bindingsView;
	bindingsView->setModel(model);
	bindingsView->horizontalHeader()->setStretchLastSection(true);
	bindingsView->resizeColumnsToContents();
}

void ControllerConfigDialog::PrepareProfiles()
{
	std::string profile = CAppConfig::GetInstance().GetPreferenceString(PREF_INPUT_PAD1_PROFILE);

	auto path = CInputConfig::GetProfilePath();
	if(fs::is_directory(path))
	{
		for(auto& entry : fs::directory_iterator(path))
		{
			auto profile = Framework::PathUtils::GetNativeStringFromPath(entry.path().stem());
			ui->comboBox->addItem(profile.c_str());
		}
	}

	auto index = ui->comboBox->findText(profile.c_str());
	if(index >= 0)
		ui->comboBox->setCurrentIndex(index);
}

void ControllerConfigDialog::UpdateAnalogSensitivityValueLabel(uint32 padIndex)
{
	assert(padIndex < m_padUiElements.size());
	float sensitivityValue = m_inputManager->GetAnalogSensitivity(padIndex);
	auto label = m_padUiElements[padIndex].analogSensitivityValueLabel;
	label->setText(QString("%1").arg(sensitivityValue, 0, 'f', 3));
}

void ControllerConfigDialog::on_buttonBox_clicked(QAbstractButton* button)
{
	if(button == ui->buttonBox->button(QDialogButtonBox::Ok))
	{
		m_inputManager->Save();
		CAppConfig::GetInstance().Save();
	}
	else if(button == ui->buttonBox->button(QDialogButtonBox::Apply))
	{
		m_inputManager->Save();
		CAppConfig::GetInstance().Save();
	}
	else if(button == ui->buttonBox->button(QDialogButtonBox::RestoreDefaults))
	{
		uint32 padIndex = ui->tabWidget->currentIndex();
		assert(padIndex < CInputBindingManager::MAX_PADS);
		AutoConfigureKeyboard(padIndex, m_inputManager);
		for(const auto& uiElements : m_padUiElements)
		{
			static_cast<CInputBindingModel*>(uiElements.bindingsView->model())->Refresh();
		}
	}
	else if(button == ui->buttonBox->button(QDialogButtonBox::Cancel))
	{
		m_inputManager->Reload();
	}
}

void ControllerConfigDialog::bindingsViewDoubleClicked(const QModelIndex& index)
{
	uint32 padIndex = ui->tabWidget->currentIndex();
	assert(padIndex < CInputBindingManager::MAX_PADS);
	OpenBindConfigDialog(padIndex, index.row());
	auto& bindingsView = m_padUiElements[padIndex].bindingsView;
	static_cast<CInputBindingModel*>(bindingsView->model())->RefreshRow(index);
}

void ControllerConfigDialog::bindingsViewDeleteItem()
{
	uint32 padIndex = ui->tabWidget->currentIndex();
	assert(padIndex < CInputBindingManager::MAX_PADS);
	auto& bindingsView = m_padUiElements[padIndex].bindingsView;
	QItemSelectionModel* selModel = bindingsView->selectionModel();
	auto selRows = selModel->selectedRows();
	if(selRows.empty())
	{
		return;
	}
	auto message = QString("Clear %1 binding(s)?").arg(selRows.size());
	auto result = QMessageBox::question(this, tr("Clear bindings"), message, QMessageBox::Ok, QMessageBox::Cancel);
	if(result == QMessageBox::Ok)
	{
		for(const auto& selRow : selRows)
		{
			m_inputManager->ResetBinding(padIndex, static_cast<PS2::CControllerInfo::BUTTON>(selRow.row()));
			static_cast<CInputBindingModel*>(bindingsView->model())->RefreshRow(selRow);
		}
	}
}

void ControllerConfigDialog::analogSensitivityValueChanged(uint32 padIndex, int value)
{
	float sensitivityValue = static_cast<float>(value) / ANALOG_SENSITIVITY_SCALE;
	assert(padIndex < CInputBindingManager::MAX_PADS);
	m_inputManager->SetAnalogSensitivity(padIndex, sensitivityValue);
	UpdateAnalogSensitivityValueLabel(padIndex);
}

void ControllerConfigDialog::on_ConfigAllButton_clicked()
{
	uint32 padIndex = ui->tabWidget->currentIndex();
	assert(padIndex < CInputBindingManager::MAX_PADS);
	for(uint32 buttonIndex = 0; buttonIndex < PS2::CControllerInfo::MAX_BUTTONS; ++buttonIndex)
	{
		if(!OpenBindConfigDialog(padIndex, buttonIndex))
		{
			break;
		}
	}
}

void ControllerConfigDialog::AutoConfigureKeyboard(uint32 padIndex, CInputBindingManager* bindingManager)
{
	bindingManager->SetSimpleBinding(padIndex, PS2::CControllerInfo::START, CInputProviderQtKey::MakeBindingTarget(Qt::Key_Return));
	bindingManager->SetSimpleBinding(padIndex, PS2::CControllerInfo::SELECT, CInputProviderQtKey::MakeBindingTarget(Qt::Key_Backspace));
	bindingManager->SetSimpleBinding(padIndex, PS2::CControllerInfo::DPAD_LEFT, CInputProviderQtKey::MakeBindingTarget(Qt::Key_Left));
	bindingManager->SetSimpleBinding(padIndex, PS2::CControllerInfo::DPAD_RIGHT, CInputProviderQtKey::MakeBindingTarget(Qt::Key_Right));
	bindingManager->SetSimpleBinding(padIndex, PS2::CControllerInfo::DPAD_UP, CInputProviderQtKey::MakeBindingTarget(Qt::Key_Up));
	bindingManager->SetSimpleBinding(padIndex, PS2::CControllerInfo::DPAD_DOWN, CInputProviderQtKey::MakeBindingTarget(Qt::Key_Down));
	bindingManager->SetSimpleBinding(padIndex, PS2::CControllerInfo::SQUARE, CInputProviderQtKey::MakeBindingTarget(Qt::Key_A));
	bindingManager->SetSimpleBinding(padIndex, PS2::CControllerInfo::CROSS, CInputProviderQtKey::MakeBindingTarget(Qt::Key_Z));
	bindingManager->SetSimpleBinding(padIndex, PS2::CControllerInfo::TRIANGLE, CInputProviderQtKey::MakeBindingTarget(Qt::Key_S));
	bindingManager->SetSimpleBinding(padIndex, PS2::CControllerInfo::CIRCLE, CInputProviderQtKey::MakeBindingTarget(Qt::Key_X));
	bindingManager->SetSimpleBinding(padIndex, PS2::CControllerInfo::L1, CInputProviderQtKey::MakeBindingTarget(Qt::Key_1));
	bindingManager->SetSimpleBinding(padIndex, PS2::CControllerInfo::L2, CInputProviderQtKey::MakeBindingTarget(Qt::Key_2));
	bindingManager->SetSimpleBinding(padIndex, PS2::CControllerInfo::L3, CInputProviderQtKey::MakeBindingTarget(Qt::Key_3));
	bindingManager->SetSimpleBinding(padIndex, PS2::CControllerInfo::R1, CInputProviderQtKey::MakeBindingTarget(Qt::Key_8));
	bindingManager->SetSimpleBinding(padIndex, PS2::CControllerInfo::R2, CInputProviderQtKey::MakeBindingTarget(Qt::Key_9));
	bindingManager->SetSimpleBinding(padIndex, PS2::CControllerInfo::R3, CInputProviderQtKey::MakeBindingTarget(Qt::Key_0));

	bindingManager->SetSimulatedAxisBinding(padIndex, PS2::CControllerInfo::ANALOG_LEFT_X,
	                                        CInputProviderQtKey::MakeBindingTarget(Qt::Key_F),
	                                        CInputProviderQtKey::MakeBindingTarget(Qt::Key_H));
	bindingManager->SetSimulatedAxisBinding(padIndex, PS2::CControllerInfo::ANALOG_LEFT_Y,
	                                        CInputProviderQtKey::MakeBindingTarget(Qt::Key_T),
	                                        CInputProviderQtKey::MakeBindingTarget(Qt::Key_G));

	bindingManager->SetSimulatedAxisBinding(padIndex, PS2::CControllerInfo::ANALOG_RIGHT_X,
	                                        CInputProviderQtKey::MakeBindingTarget(Qt::Key_J),
	                                        CInputProviderQtKey::MakeBindingTarget(Qt::Key_L));
	bindingManager->SetSimulatedAxisBinding(padIndex, PS2::CControllerInfo::ANALOG_RIGHT_Y,
	                                        CInputProviderQtKey::MakeBindingTarget(Qt::Key_I),
	                                        CInputProviderQtKey::MakeBindingTarget(Qt::Key_K));
}

int ControllerConfigDialog::OpenBindConfigDialog(uint32 padIndex, uint32 buttonIndex)
{
	auto button = static_cast<PS2::CControllerInfo::BUTTON>(buttonIndex);
	std::string buttonName(PS2::CControllerInfo::m_buttonName[button]);
	std::transform(buttonName.begin(), buttonName.end(), buttonName.begin(), ::toupper);

	InputEventSelectionDialog IESD(this);
	IESD.Setup(buttonName.c_str(), m_inputManager, m_qtKeyInputProvider, m_qtMouseInputProvider, padIndex, button);
	auto res = IESD.exec();
	return res;
}

void ControllerConfigDialog::on_comboBox_currentIndexChanged(int index)
{
	ui->delProfileButton->setEnabled(ui->comboBox->count() > 1);

	auto profile = ui->comboBox->itemText(index).toStdString();
	m_inputManager->Save();
	m_inputManager->Load(profile.c_str());
	CAppConfig::GetInstance().SetPreferenceString(PREF_INPUT_PAD1_PROFILE, profile.c_str());

	for(int padIndex = 0; padIndex < m_padUiElements.size(); padIndex++)
	{
		const auto& uiElements = m_padUiElements[padIndex];
		static_cast<CInputBindingModel*>(uiElements.bindingsView->model())->Refresh();
		uiElements.analogSensitivitySlider->setValue(m_inputManager->GetAnalogSensitivity(padIndex) * ANALOG_SENSITIVITY_SCALE);
		UpdateAnalogSensitivityValueLabel(padIndex);
	}
}

void ControllerConfigDialog::on_addProfileButton_clicked()
{
	std::string profile_name;
	while(profile_name.empty())
	{
		bool ok_pressed = false;
		QString name = QInputDialog::getText(this, tr("Enter Profile Name"), tr("Only letters, numbers and dash characters allowed.\nProfile name:"),
		                                     QLineEdit::Normal, "", &ok_pressed);

		if(!ok_pressed)
			return;

		if(!name.isEmpty() && CInputConfig::IsValidProfileName(name.toStdString()))
			profile_name = name.toStdString();
	}

	{
		auto profile_path = CInputConfig::GetProfile(profile_name);
		if(!fs::exists(profile_path))
		{
			ui->comboBox->addItem(profile_name.c_str());
		}

		auto index = ui->comboBox->findText(profile_name.c_str());
		if(index >= 0)
			ui->comboBox->setCurrentIndex(index);

		//Set some defaults
		AutoConfigureKeyboard(0, m_inputManager);
	}
}

void ControllerConfigDialog::on_delProfileButton_clicked()
{
	auto name = ui->comboBox->currentText();
	std::string profile_name = name.toStdString();
	{
		auto profile_path = CInputConfig::GetProfile(profile_name);
		if(fs::exists(profile_path))
		{
			int index = ui->comboBox->currentIndex();
			ui->comboBox->removeItem(index);
			fs::remove(profile_path);
		}
	}
}
