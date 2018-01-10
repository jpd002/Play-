#include "inputeventselectiondialog.h"
#include "ui_inputeventselectiondialog.h"


InputEventSelectionDialog::InputEventSelectionDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::InputEventSelectionDialog)
{
	ui->setupUi(this);
	setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint);
	setFixedSize(size());

	// workaround to avoid direct thread ui access
	connect(this, SIGNAL(setSelectedButtonLabelText(QString)), ui->selectedbuttonlabel, SLOT(setText(QString)));
	connect(this, SIGNAL(setCountDownLabelText(QString)), ui->countdownlabel, SLOT(setText(QString)));

	m_isCounting = false;
	m_running = true;
	m_thread = std::thread([this]{ CountDownThreadLoop(); });
}

InputEventSelectionDialog::~InputEventSelectionDialog()
{
	m_running = false;
	if(m_thread.joinable()) m_thread.join();
	delete ui;
}

void InputEventSelectionDialog::Setup(const char* text, CInputBindingManager *inputManager,
									  PS2::CControllerInfo::BUTTON button, std::unique_ptr<CGamePadDeviceListener>const &GPDL)
{
	m_inputManager = inputManager;
	ui->bindinglabel->setText(m_bindingtext.arg(text));
	m_button = button;

	auto onInput = [=](std::array<unsigned int, 6> device, int code, int value, int type, const input_absinfo *abs)->void
	{
		if(type == 4) return;
		if(!(type == 3 && abs->flat))
		{
			if(!setCounter(value)) return;
		}
		QString key = QString(libevdev_event_code_get_name(type, code));
		if(type == 3)
		{
			if(abs->flat)
			{
				int triggerRange = abs->maximum/100*20;
				int triggerVal1 = abs->maximum - triggerRange;
				int triggerVal2 = abs->minimum + triggerRange;
				if(value < triggerVal1 && triggerVal2 < value)
				{
					setCounter(0);
					return;
				}
				setCounter(1);
				setSelectedButtonLabelText("Selected Key: " + key);
				m_key1.id = code;
				m_key1.device = device;
				m_key1.type = type;
				m_key1.bindtype = CInputBindingManager::BINDINGTYPE::BINDING_SIMPLE;
			}
			else
			{
				m_key1.id = code;
				m_key1.device = device;
				m_key1.type = type;
				m_key1.value = value;
				m_key1.bindtype = CInputBindingManager::BINDINGTYPE::BINDING_POVHAT;
				setSelectedButtonLabelText("Selected Key: " + key);
			}
		}
		else
		{
			if(PS2::CControllerInfo::IsAxis(m_button))
			{
				if(click_count == 0)
				{
					setSelectedButtonLabelText("(-) Key Selected: " + key);
					m_key1.id = code;
					m_key1.device = device;
					m_key1.type = type;
					m_key1.bindtype = CInputBindingManager::BINDINGTYPE::BINDING_SIMULATEDAXIS;
				}
				else
				{
					m_key2.id = code;
					m_key2.device = device;
					m_key2.type = type;
					setSelectedButtonLabelText("(+) Key Selected: " + key);
				}
			}
			else
			{
				setSelectedButtonLabelText("Selected Key: " + key);
				m_key1.id = code;
				m_key1.device = device;
				m_key1.type = type;
				m_key1.bindtype = CInputBindingManager::BINDINGTYPE::BINDING_SIMPLE;
			}
		}
	};

	GPDL.get()->UpdateOnInputEventCallback(onInput);

}
void InputEventSelectionDialog::keyPressEvent(QKeyEvent* ev)
{
	if(ev->key() == Qt::Key_Escape)
	{
		m_running = false;
		reject();
		return;
	}
	setCounter(1);

	QString key = QKeySequence(ev->key()).toString();
	if(PS2::CControllerInfo::IsAxis(m_button))
	{
		if(click_count == 0)
		{
			m_key1.id = ev->key();
			m_key1.device = {0};
			m_key1.bindtype = CInputBindingManager::BINDINGTYPE::BINDING_SIMULATEDAXIS;
			setSelectedButtonLabelText("(-) Key Selected: " + key);
		}
		else
		{
			m_key2.id = ev->key();
			m_key2.device = {0};
			setSelectedButtonLabelText("(+) Key Selected: " + key);
		}
	}
	else
	{
		m_key1.id = ev->key();
		m_key1.device = {0};
		m_key1.bindtype = CInputBindingManager::BINDINGTYPE::BINDING_SIMPLE;
		setSelectedButtonLabelText("Key Selected: " + key);
	}
}

void InputEventSelectionDialog::keyReleaseEvent(QKeyEvent* ev)
{
	if(!ev->isAutoRepeat())
	{
		setCounter(0);
	}
}

void InputEventSelectionDialog::CountDownThreadLoop()
{
	while(m_running)
	{
		auto time_now = std::chrono::system_clock::now();
		std::chrono::duration<double> diff = time_now - m_countStart;
		if(m_isCounting)
		{
			if(diff.count() < 3)
			{
				setCountDownLabelText(m_countingtext.arg(static_cast<int>(3 - diff.count())));
			}
			else
			{
				switch(m_key1.bindtype)
				{
					case CInputBindingManager::BINDINGTYPE::BINDING_SIMPLE:
						m_inputManager->SetSimpleBinding(m_button, m_key1);
						break;
					case CInputBindingManager::BINDINGTYPE::BINDING_POVHAT:
						m_inputManager->SetPovHatBinding(m_button,m_key1, m_key1.value);
						break;
					case CInputBindingManager::BINDINGTYPE::BINDING_SIMULATEDAXIS:
						if(click_count == 0)
						{
							click_count++;
							m_isCounting = 0;
							setCountDownLabelText(m_countingtext.arg(3));
							continue;
						}
						else if(m_key2.id > 0)
						{
							m_inputManager->SetSimulatedAxisBinding(m_button, m_key1, m_key2);
						}
						break;
				}

				m_running = false;
				accept();
			}
		}
		else if(diff.count() < 3)
		{
			setCountDownLabelText(m_countingtext.arg(3));
		}
	}
}

bool InputEventSelectionDialog::setCounter(int value)
{
	if(value == 0)
	{
		if(click_count == 0)
		{
			m_key1.id = -1;
			setSelectedButtonLabelText("Selected Key: None");
		}
		else
		{
			m_key2.id = -1;
			setSelectedButtonLabelText("(+) Selected Key: None");
		}
		m_isCounting = false;
		return false;
	}
	else if(m_isCounting == false)
	{
		m_countStart = std::chrono::system_clock::now();
		m_isCounting = true;
	}
	return true;
}
