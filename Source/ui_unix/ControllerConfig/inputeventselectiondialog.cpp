#include <QTimer>
#include "inputeventselectiondialog.h"
#include "ui_inputeventselectiondialog.h"

#define COUNTDOWN_SECS 3

InputEventSelectionDialog::InputEventSelectionDialog(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::InputEventSelectionDialog)
{
	ui->setupUi(this);
	ui->countdownlabel->setText(m_countingtext.arg(COUNTDOWN_SECS));

	m_countdownTimer = new QTimer(this);
	connect(m_countdownTimer, SIGNAL(timeout()), this, SLOT(updateCountdown()));
	
	setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint);
	setFixedSize(size());

	// workaround to avoid direct thread ui access
	connect(this, SIGNAL(startCountdown(QString)), this, SLOT(handleStartCountdown(QString)));
	connect(this, SIGNAL(cancelCountdown()), this, SLOT(handleCancelCountdown()));
}

InputEventSelectionDialog::~InputEventSelectionDialog()
{
	m_inputManager->OverrideInputEventHandler(InputEventFunction());
	delete ui;
}

void InputEventSelectionDialog::Setup(const char* text, CInputBindingManager* inputManager, CInputProviderQtKey* qtKeyInputProvider, PS2::CControllerInfo::BUTTON button)
{
	m_inputManager = inputManager;
	m_inputManager->OverrideInputEventHandler([this] (auto target, auto value) { this->onInputEvent(target, value); } );
	connect(this, SIGNAL(countdownComplete()), this, SLOT(completeSimpleBinding()));
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
	auto setSelection =
		[this] (CInputBindingManager::BINDINGTYPE bindingType, const auto& target)
		{
			m_selectedTarget = target;
			m_bindingType = bindingType;
			m_state = STATE::SELECTED;
			auto targetDescription = m_inputManager->GetTargetDescription(target);
			startCountdown(QString::fromStdString(targetDescription));
		};
	
	auto resetSelection =
		[this] ()
		{
			m_selectedTarget = BINDINGTARGET();
			m_state = STATE::NONE;
			m_bindingType = CInputBindingManager::BINDING_UNBOUND;
			cancelCountdown();
		};
	
	switch(m_state)
	{
	case STATE::NONE:
		//Check if we've pressed something
		switch(target.keyType)
		{
		case BINDINGTARGET::KEYTYPE::BUTTON:
			if(value != 0)
			{
				setSelection(CInputBindingManager::BINDING_SIMPLE, target);
			}
			break;
		case BINDINGTARGET::KEYTYPE::AXIS:
			if(!IsAxisIdle(value))
			{
				setSelection(CInputBindingManager::BINDING_SIMPLE, target);
			}
			break;
		case BINDINGTARGET::KEYTYPE::POVHAT:
			if(value < BINDINGTARGET::POVHAT_MAX)
			{
				setSelection(CInputBindingManager::BINDING_POVHAT, target);
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
				resetSelection();
			}
			break;
		case BINDINGTARGET::KEYTYPE::AXIS:
			if(IsAxisIdle(value))
			{
				resetSelection();
			}
			break;
		case BINDINGTARGET::KEYTYPE::POVHAT:
			if(m_bindingValue != value)
			{
				resetSelection();
			}
		}
		break;
	}
}

void InputEventSelectionDialog::keyPressEvent(QKeyEvent* ev)
{
	if(ev->key() == Qt::Key_Escape)
	{
		reject();
		return;
	}
	m_qtKeyInputProvider->OnKeyPress(ev->key());
}

void InputEventSelectionDialog::keyReleaseEvent(QKeyEvent* ev)
{
	m_qtKeyInputProvider->OnKeyRelease(ev->key());
}

void InputEventSelectionDialog::handleStartCountdown(QString bindingDesc)
{
	m_countdownRemain = COUNTDOWN_SECS - 1;
	static_assert(COUNTDOWN_SECS >= 1, "COUNTDOWN_SECS must be at least 1");
	ui->countdownlabel->setText(m_countingtext.arg(m_countdownRemain));
	ui->selectedbuttonlabel->setText("Selected Key: " + bindingDesc);
	m_countdownTimer->start(1000);
}

void InputEventSelectionDialog::handleCancelCountdown()
{
	m_countdownTimer->stop();
	ui->countdownlabel->setText(m_countingtext.arg(COUNTDOWN_SECS));
	ui->selectedbuttonlabel->setText("Selected Key: None");
}

void InputEventSelectionDialog::updateCountdown()
{
	if(m_countdownRemain == 0)
	{
		m_countdownTimer->stop();
		countdownComplete();
	}
	else
	{
		m_countdownRemain--;
		ui->countdownlabel->setText(m_countingtext.arg(m_countdownRemain));
	}
}

void InputEventSelectionDialog::completeSimpleBinding()
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
	accept();
}
