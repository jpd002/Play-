#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QtWidgets/QListWidget>

#include "AppConfig.h"

namespace Ui
{
	class SettingsDialog;
}

class SettingsDialog : public QDialog
{
	Q_OBJECT

public:
	enum GS_HANDLERS
	{
		OPENGL,
#ifdef HAS_GSH_VULKAN
		VULKAN,
#endif
		MAX_HANDLER
	};

	explicit SettingsDialog(QWidget* parent = 0);
	~SettingsDialog();
	void LoadPreferences();

private slots:
	void changePage(QListWidgetItem* current, QListWidgetItem* previous);

	//General Page
	void on_comboBox_system_language_currentIndexChanged(int index);
	void on_checkBox_limitFrameRate_clicked(bool checked);
	void on_checkBox_showEECPUUsage_clicked(bool checked);

	//Video Page
	void on_checkBox_widescreenOutput_clicked(bool checked);
	void on_checkBox_enable_gs_ram_reads_clicked(bool checked);
	void on_checkBox_force_bilinear_filtering_clicked(bool checked);
	void on_comboBox_gs_selection_currentIndexChanged(int index);
	void on_comboBox_vulkan_device_currentIndexChanged(int index);
	void on_button_vulkanDeviceInfo_clicked();
	void on_comboBox_presentation_mode_currentIndexChanged(int index);
	void on_comboBox_res_multiplyer_currentIndexChanged(int index);

	//Audio Page
	void on_checkBox_enable_audio_clicked(bool checked);
	void on_spinBox_spuBlockCount_valueChanged(int value);

private:
	Ui::SettingsDialog* ui;
};

#endif // SETTINGSDIALOG_H
