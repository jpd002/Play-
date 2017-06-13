#pragma once

#include <QDialog>
#include <QAbstractButton>
#include <QXmlStreamReader>

#include "GamePad/GamePadDeviceListener.h"
#include "PH_HidUnix.h"
#include "Config.h"

namespace Ui {
class ControllerConfigDialog;
}

class ControllerConfigDialog : public QDialog
{
	Q_OBJECT

public:
	explicit ControllerConfigDialog(QWidget *parent = 0);
	~ControllerConfigDialog();

private slots:
	void on_buttonBox_clicked(QAbstractButton *button);
	void on_tableView_doubleClicked(const QModelIndex &index);
	void on_ConfigAllButton_clicked();

private:
	CInputBindingManager* m_inputManager;
	std::unique_ptr<CGamePadDeviceListener> m_inputDeviceManager;
	Ui::ControllerConfigDialog *ui;
};
