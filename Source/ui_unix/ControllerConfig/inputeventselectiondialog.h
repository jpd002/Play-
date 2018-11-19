#pragma once

#include <QDialog>
#include <QKeyEvent>
#include <chrono>
#include <thread>
#include "input/InputBindingManager.h"
#include "InputProviderQtKey.h"

namespace Ui
{
	class InputEventSelectionDialog;
}

class InputEventSelectionDialog : public QDialog
{
	Q_OBJECT

public:
	explicit InputEventSelectionDialog(QWidget* parent = nullptr);
	~InputEventSelectionDialog();

	void Setup(const char*, CInputBindingManager*, CInputProviderQtKey*, PS2::CControllerInfo::BUTTON);

protected:
	void keyPressEvent(QKeyEvent*) Q_DECL_OVERRIDE;
	void keyReleaseEvent(QKeyEvent*) Q_DECL_OVERRIDE;

private:
	enum class STATE
	{
		NONE,
		SELECTED
	};
	
	void CountDownThreadLoop();
	void onInputEvent(const BINDINGTARGET&, uint32);
	bool setCounter(int);

	Ui::InputEventSelectionDialog* ui = nullptr;

	STATE m_state = STATE::NONE;
	BINDINGTARGET m_selectedTarget;
	CInputBindingManager::BINDINGTYPE m_bindingType = CInputBindingManager::BINDING_UNBOUND;
	uint32 m_bindingValue = 0;
	
	QString m_bindingtext = QString("Select new binding for\n%1");
	QString m_countingtext = QString("Press & Hold Button for %1 Seconds to assign key");
	PS2::CControllerInfo::BUTTON m_button;
	CInputBindingManager* m_inputManager = nullptr;
	CInputProviderQtKey* m_qtKeyInputProvider = nullptr;

	std::chrono::time_point<std::chrono::system_clock> m_countStart = std::chrono::system_clock::now();
	std::thread m_thread;
	std::atomic<bool> m_running;
	std::atomic<bool> m_isCounting;

Q_SIGNALS:
	void setSelectedButtonLabelText(QString);
	void setCountDownLabelText(QString);
};
