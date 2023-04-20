#pragma once

#include <QDialog>
#include <QAbstractButton>

#include "../input/InputBindingManager.h"

namespace Ui
{
	class ControllerConfigDialog;
}

class QTableView;
class QSlider;
class QLabel;
class CInputProviderQtKey;
class CInputProviderQtMouse;

class ControllerConfigDialog : public QDialog
{
	Q_OBJECT

public:
	explicit ControllerConfigDialog(CInputBindingManager*, CInputProviderQtKey*, CInputProviderQtMouse*, QWidget* parent = 0);
	~ControllerConfigDialog();

	static void AutoConfigureKeyboard(uint32 padIndex, CInputBindingManager*);

private slots:
	void bindingsViewDoubleClicked(const QModelIndex& index);
	void analogSensitivityValueChanged(uint32, int);

	void on_buttonBox_clicked(QAbstractButton* button);
	void on_ConfigAllButton_clicked();

	void on_comboBox_currentIndexChanged(int index);
	void on_addProfileButton_clicked();
	void on_delProfileButton_clicked();

private:
	struct PadUiElements
	{
		QTableView* bindingsView = nullptr;
		QSlider* analogSensitivitySlider = nullptr;
		QLabel* analogSensitivityValueLabel = nullptr;
	};

	void PrepareBindingsView(uint32 padIndex);
	void PrepareProfiles();
	int OpenBindConfigDialog(uint32 padIndex, uint32 buttonIndex);
	void UpdateAnalogSensitivityValueLabel(uint32 padIndex);

	Ui::ControllerConfigDialog* ui;
	CInputBindingManager* m_inputManager = nullptr;
	CInputProviderQtKey* m_qtKeyInputProvider = nullptr;
	CInputProviderQtMouse* m_qtMouseInputProvider = nullptr;
	std::vector<PadUiElements> m_padUiElements;
};
