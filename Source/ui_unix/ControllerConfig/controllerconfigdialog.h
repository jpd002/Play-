#pragma once

#include <QDialog>
#include <QAbstractButton>
#include <QXmlStreamReader>

#ifdef HAS_LIBEVDEV
#include "GamePad/GamePadDeviceListener.h"
#elif defined(__APPLE__)
#include "GamePad/GamePadDeviceListener_OSX.h"
#endif
#include "InputBindingManager.h"

namespace Ui
{
	class ControllerConfigDialog;
}

class ControllerConfigDialog : public QDialog
{
	Q_OBJECT

public:
	explicit ControllerConfigDialog(QWidget* parent = 0);
	~ControllerConfigDialog();

	void SetInputBindingManager(CInputBindingManager*);

private slots:
	void on_buttonBox_clicked(QAbstractButton* button);
	void on_tableView_doubleClicked(const QModelIndex& index);
	void on_ConfigAllButton_clicked();

private:
	int OpenBindConfigDialog(int index);
	CInputBindingManager* m_inputManager;
#if defined(HAS_LIBEVDEV) || defined(__APPLE__)
	std::unique_ptr<CGamePadDeviceListener> m_inputDeviceManager;
#endif
	Ui::ControllerConfigDialog* ui;
};
