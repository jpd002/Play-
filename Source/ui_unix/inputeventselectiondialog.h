#pragma once

#include <QDialog>
#include <QKeyEvent>

#include "ui_unix/PH_HidUnix.h"

#include "GamePad/GamePadInputEventListener.h"
#include "GamePad/GamePadDeviceListener.h"

namespace Ui {
class InputEventSelectionDialog;
}

class InputEventSelectionDialog : public QDialog
{
	Q_OBJECT

public:
	explicit InputEventSelectionDialog(QWidget *parent = 0);
	~InputEventSelectionDialog();

	void					Setup(const char *text, CInputBindingManager *inputManager, PS2::CControllerInfo::BUTTON button, const std::unique_ptr<CGamePadDeviceListener> &GPDL);

protected:
	void					keyPressEvent(QKeyEvent*) Q_DECL_OVERRIDE;

private:
	int									click_count = 0;
	QString								labeltext = QString("Select new binding for\n%1");
	PS2::CControllerInfo::BUTTON		m_button;
	CInputBindingManager::BINDINGINFO	m_key1;
	CInputBindingManager				*m_inputManager;
	Ui::InputEventSelectionDialog		*ui;
};
