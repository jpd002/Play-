#include "inputeventselectiondialog.h"
#include "ui_inputeventselectiondialog.h"

InputEventSelectionDialog::InputEventSelectionDialog(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::InputEventSelectionDialog)
{
	ui->setupUi(this);
	setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint);
	setFixedSize(size());

	// workaround to avoid direct thread ui access
	connect(this, SIGNAL(setSelectedButtonLabelText(QString)), ui->selectedbuttonlabel, SLOT(setText(QString)));
	connect(this, SIGNAL(setCountDownLabelText(QString)), ui->countdownlabel, SLOT(setText(QString)));

	m_isCounting = false;
	m_running = true;
	m_thread = std::thread([this] { CountDownThreadLoop(); });
}

InputEventSelectionDialog::~InputEventSelectionDialog()
{
	m_inputManager->OverrideInputEventHandler(InputEventFunction());
	m_running = false;
	if(m_thread.joinable()) m_thread.join();
	delete ui;
}

void InputEventSelectionDialog::Setup(const char* text, CInputBindingManager* inputManager, CInputProviderQtKey* qtKeyInputProvider, PS2::CControllerInfo::BUTTON button)
{
	m_inputManager = inputManager;
	m_inputManager->OverrideInputEventHandler([this] (auto target, auto value) { this->onInputEvent(target, value); } );
	m_qtKeyInputProvider = qtKeyInputProvider;
	ui->bindinglabel->setText(m_bindingtext.arg(text));
	m_button = button;
}

static bool IsAxisIdle(uint32 value)
{
	uint32 triggerRange = (255 * 20) / 100;
	uint32 triggerVal1 = 255 - triggerRange;
	uint32 triggerVal2 = 0 + triggerRange;
	return (value < triggerVal1) && (value > triggerVal2);
}

void InputEventSelectionDialog::onInputEvent(const BINDINGTARGET& target, uint32 value)
{
	switch(m_state)
	{
	case STATE::NONE:
		//Check if we've pressed something
		switch(target.keyType)
		{
		case BINDINGTARGET::KEYTYPE::BUTTON:
			if(value != 0)
			{
				m_selectedTarget = target;
				m_bindingType = CInputBindingManager::BINDING_SIMPLE;
				m_state = STATE::SELECTED;
				setCounter(1);
				auto targetDescription = m_inputManager->GetTargetDescription(target);
				setSelectedButtonLabelText("Selected Key: " + QString::fromStdString(targetDescription));
			}
			break;
		case BINDINGTARGET::KEYTYPE::AXIS:
			if(!IsAxisIdle(value))
			{
				m_selectedTarget = target;
				m_bindingType = CInputBindingManager::BINDING_SIMPLE;
				m_state = STATE::SELECTED;
				setCounter(1);
				auto targetDescription = m_inputManager->GetTargetDescription(target);
				setSelectedButtonLabelText("Selected Key: " + QString::fromStdString(targetDescription));
			}
			break;
		case BINDINGTARGET::KEYTYPE::POVHAT:
			if(value < BINDINGTARGET::POVHAT_MAX)
			{
				m_selectedTarget = target;
				m_bindingType = CInputBindingManager::BINDING_POVHAT;
				m_bindingValue = value;
				m_state = STATE::SELECTED;
				setCounter(1);
				auto targetDescription = m_inputManager->GetTargetDescription(target);
				setSelectedButtonLabelText("Selected Key: " + QString::fromStdString(targetDescription));
			}
		}
		break;
	case STATE::SELECTED:
		if(target != m_selectedTarget) break;
		switch(target.keyType)
		{
		case BINDINGTARGET::KEYTYPE::BUTTON:
			if(value == 0)
			{
				m_selectedTarget = BINDINGTARGET();
				m_state = STATE::NONE;
				m_bindingType = CInputBindingManager::BINDING_UNBOUND;

				setCounter(0);
				setSelectedButtonLabelText("Selected Key: None");
			}
			break;
		case BINDINGTARGET::KEYTYPE::AXIS:
			if(IsAxisIdle(value))
			{
				m_selectedTarget = BINDINGTARGET();
				m_state = STATE::NONE;
				m_bindingType = CInputBindingManager::BINDING_UNBOUND;
				setCounter(0);
				setSelectedButtonLabelText("Selected Key: None");
			}
			break;
		case BINDINGTARGET::KEYTYPE::POVHAT:
			if(m_bindingValue != value)
			{
				m_selectedTarget = BINDINGTARGET();
				m_state = STATE::NONE;
				m_bindingType = CInputBindingManager::BINDING_UNBOUND;
				setCounter(0);
				setSelectedButtonLabelText("Selected Key: None");
			}
		}
		break;
	}
}

void InputEventSelectionDialog::keyPressEvent(QKeyEvent* ev)
{
	if(ev->key() == Qt::Key_Escape)
	{
		m_running = false;
		reject();
		return;
	}
	m_qtKeyInputProvider->OnKeyPress(ev->key());
}

void InputEventSelectionDialog::keyReleaseEvent(QKeyEvent* ev)
{
	m_qtKeyInputProvider->OnKeyRelease(ev->key());
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
				switch(m_bindingType)
				{
				case CInputBindingManager::BINDING_SIMPLE:
					m_inputManager->SetSimpleBinding(0, m_button, m_selectedTarget);
					break;
				case CInputBindingManager::BINDING_POVHAT:
					m_inputManager->SetPovHatBinding(0, m_button, m_selectedTarget, m_bindingValue);
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
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

bool InputEventSelectionDialog::setCounter(int value)
{
	if(value == 0)
	{
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
