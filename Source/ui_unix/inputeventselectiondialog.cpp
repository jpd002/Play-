#include "inputeventselectiondialog.h"
#include "ui_inputeventselectiondialog.h"


InputEventSelectionDialog::InputEventSelectionDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::InputEventSelectionDialog)
{
	ui->setupUi(this);
	setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint);
	setFixedSize(size());
}

InputEventSelectionDialog::~InputEventSelectionDialog()
{
	delete ui;
}

void InputEventSelectionDialog::Setup(const char* text, CInputBindingManager *inputManager,
									  PS2::CControllerInfo::BUTTON button, std::unique_ptr<CGamePadDeviceListener>const &GPDL)
{
	m_inputManager = inputManager;
	ui->label->setText(labeltext.arg(text));
	m_button = button;

	auto onInput = [=](std::array<unsigned int, 6> device, int code, int value, int type, const input_absinfo *abs)->void
	{
		if(value == 0 || type == 4) return;
		QString key = QString(libevdev_event_code_get_name(type, code));
		if(type == 3)
		{
			if(abs->flat)
			{
				int midVal = abs->maximum/2;
				int triggerVal = abs->maximum/10;
				if (value < midVal + triggerVal && value > midVal - triggerVal) return;
				ui->label->setText(ui->label->text() + "\n\nSelected Key: " + key);
				m_key1.id = code;
				m_key1.device = device;
				m_key1.type = type;
				m_inputManager->SetSimpleBinding(m_button,m_key1);
			}
			else
			{
				m_key1.id = code;
				m_key1.device = device;
				m_key1.type = type;
				m_inputManager->SetPovHatBinding(m_button,m_key1, value);
				ui->label->setText(ui->label->text() + "\n\nSelected Key: " + key);
			}
			accept();
		}
		else
		{
			if(PS2::CControllerInfo::IsAxis(m_button))
			{
				if(click_count++ == 0)
				{
					m_key1.id = code;
					m_key1.device = device;
					m_key1.type = type;
					ui->label->setText("(-) Key Selected: " + key + "\nPlease Select (+) Key");
					return;
				}
				else
				{
					CInputBindingManager::BINDINGINFO m_key2;
					m_key2.id = code;
					m_key2.device = device;
					m_key2.type = type;
					m_inputManager->SetSimulatedAxisBinding(m_button, m_key1, m_key2);
				}
			}
			else
			{
				ui->label->setText(ui->label->text() + "\n\nSelected Key: " + key);
				m_key1.id = code;
				m_key1.device = device;
				m_key1.type = type;
				m_inputManager->SetSimpleBinding(m_button,m_key1);
			}
			accept();

		}
	};

	GPDL.get()->UpdateOnInputEventCallback(onInput);

}
void InputEventSelectionDialog::keyPressEvent(QKeyEvent* ev)
{
	if(ev->key() == Qt::Key_Escape)
	{
		reject();
		return;
	}

	QString key = QKeySequence(ev->key()).toString();
	ui->label->setText(ui->label->text() + "\n\nSelected Key: " + key);
	if(PS2::CControllerInfo::IsAxis(m_button))
	{
		if(click_count++ == 0)
		{
			m_key1.id = ev->key();
			m_key1.device = {0};
			ui->label->setText("(-) Key Selected: " + key + "\nPlease Select (+) Key");
			return;
		}
		else
		{
			CInputBindingManager::BINDINGINFO m_key2;
			m_key2.id = ev->key();
			m_key2.device = {0};
			m_inputManager->SetSimulatedAxisBinding(m_button, m_key1, m_key2);
		}
	}
	else
	{
		m_key1.id = ev->key();
		m_key1.device = {0};
		m_inputManager->SetSimpleBinding(m_button,m_key1);
	}
	accept();
}
