#pragma once

#include <QDialog>
#include <QKeyEvent>

#include "ui_unix/PH_HidUnix.h"

namespace Ui {
class InputEventSelectionDialog;
}

class InputEventSelectionDialog : public QDialog
{
	Q_OBJECT

public:
    explicit InputEventSelectionDialog(QWidget *parent = 0);
	~InputEventSelectionDialog();

    void Setup(const char*, CInputBindingManager*, PS2::CControllerInfo::BUTTON);

    int click_count = 0;

protected:
	void keyPressEvent(QKeyEvent*) Q_DECL_OVERRIDE;

private:
    QString labeltext = QString("Select new binding for\n%1");
    PS2::CControllerInfo::BUTTON m_button;
    CInputBindingManager::BINDINGINFO m_key1;
    CInputBindingManager *m_inputManager;
	Ui::InputEventSelectionDialog *ui;
};
