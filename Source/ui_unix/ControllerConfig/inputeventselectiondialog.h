#pragma once

#include <QDialog>
#include <QKeyEvent>
#include <chrono>
#include <thread>
#include "ui_unix/PH_HidUnix.h"

#ifdef HAS_LIBEVDEV
#include "GamePad/GamePadInputEventListener.h"
#include "GamePad/GamePadDeviceListener.h"
#endif

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

	void Setup(const char* text, CInputBindingManager* inputManager, PS2::CControllerInfo::BUTTON button);
#ifdef HAS_LIBEVDEV
	void SetupInputDeviceManager(const std::unique_ptr<CGamePadDeviceListener>& GPDL);
#endif

protected:
	void keyPressEvent(QKeyEvent*) Q_DECL_OVERRIDE;
	void keyReleaseEvent(QKeyEvent*) Q_DECL_OVERRIDE;

private:
	struct BINDINGINFOEXTENDED : CInputBindingManager::BINDINGINFO
	{
		int value;
		CInputBindingManager::BINDINGTYPE bindtype;
	};
	void CountDownThreadLoop();
	bool setCounter(int);
	int click_count = 0;
	QString m_bindingtext = QString("Select new binding for\n%1");
	QString m_countingtext = QString("Press & Hold Button for %1 Seconds to assign key");
	PS2::CControllerInfo::BUTTON m_button;
	BINDINGINFOEXTENDED m_key1;
	CInputBindingManager::BINDINGINFO m_key2;
	CInputBindingManager* m_inputManager;
	Ui::InputEventSelectionDialog* ui;
	std::chrono::time_point<std::chrono::system_clock> m_countStart = std::chrono::system_clock::now();
	std::thread m_thread;
	std::atomic<bool> m_running;
	std::atomic<bool> m_isCounting;

Q_SIGNALS:
	void setSelectedButtonLabelText(QString);
	void setCountDownLabelText(QString);
};
