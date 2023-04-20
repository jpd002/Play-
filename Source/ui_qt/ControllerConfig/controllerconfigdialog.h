#pragma once

#include <QDialog>
#include <QAbstractButton>

#include "../input/InputBindingManager.h"
#include "InputProviderQtKey.h"

namespace Ui
{
	class ControllerConfigDialog;
}

class QTableView;

class ControllerConfigDialog : public QDialog
{
	Q_OBJECT

public:
	explicit ControllerConfigDialog(CInputBindingManager*, CInputProviderQtKey*, QWidget* parent = 0);
	~ControllerConfigDialog();

	static void AutoConfigureKeyboard(uint32 padIndex, CInputBindingManager*);

private slots:
	void bindingsViewDoubleClicked(const QModelIndex& index);
	void analogSensitivityValueChanged(int);

	void on_buttonBox_clicked(QAbstractButton* button);
	void on_ConfigAllButton_clicked();

	void on_comboBox_currentIndexChanged(int index);
	void on_addProfileButton_clicked();
	void on_delProfileButton_clicked();

private:
	void PrepareBindingsView(uint32 padIndex);
	void PrepareProfiles();
	int OpenBindConfigDialog(uint32 padIndex, uint32 buttonIndex);

	Ui::ControllerConfigDialog* ui;
	CInputBindingManager* m_inputManager = nullptr;
	CInputProviderQtKey* m_qtKeyInputProvider = nullptr;
	std::vector<QTableView*> m_bindingsViews;
};
