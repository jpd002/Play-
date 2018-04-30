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
	explicit SettingsDialog(QWidget* parent = 0);
	~SettingsDialog();
	void LoadPreferences();

private slots:
	void on_checkBox_enable_highres_clicked(bool checked);
	void on_checkBox_force_bilinear_filtering_clicked(bool checked);
	void on_checkBox_enable_audio_clicked(bool checked);
	void on_comboBox_presentation_mode_currentIndexChanged(int index);
	void changePage(QListWidgetItem* current, QListWidgetItem* previous);

private:
	Ui::SettingsDialog* ui;
};

#endif // SETTINGSDIALOG_H
