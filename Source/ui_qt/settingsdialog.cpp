#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include "PreferenceDefs.h"
#include "../gs/GSH_OpenGL/GSH_OpenGL.h"

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::SettingsDialog)
{
	ui->setupUi(this);

	//Not needed, as it can be set in the ui editor, but left for ease of ui edit.
	ui->stackedWidget->setCurrentIndex(0);

	LoadPreferences();
	connect(ui->listWidget, &QListWidget::currentItemChanged, this, &SettingsDialog::changePage);
}

SettingsDialog::~SettingsDialog()
{
	CAppConfig::GetInstance().Save();
	delete ui;
}

void SettingsDialog::changePage(QListWidgetItem* current, QListWidgetItem* previous)
{
	if(!current)
		current = previous;

	ui->stackedWidget->setCurrentIndex(ui->listWidget->row(current));
}

void SettingsDialog::LoadPreferences()
{
	ui->spinBox_resolution->setValue(CAppConfig::GetInstance().GetPreferenceInteger(PREF_CGSH_OPENGL_RESOLUTION_FACTOR));
	ui->checkBox_force_bilinear_filtering->setChecked(CAppConfig::GetInstance().GetPreferenceBoolean(PREF_CGSH_OPENGL_FORCEBILINEARTEXTURES));

	ui->checkBox_enable_audio->setChecked(CAppConfig::GetInstance().GetPreferenceBoolean(PREFERENCE_AUDIO_ENABLEOUTPUT));
	ui->comboBox_presentation_mode->setCurrentIndex(CAppConfig::GetInstance().GetPreferenceInteger(PREF_CGSHANDLER_PRESENTATION_MODE));
}

void SettingsDialog::on_checkBox_force_bilinear_filtering_clicked(bool checked)
{

	CAppConfig::GetInstance().SetPreferenceBoolean(PREF_CGSH_OPENGL_FORCEBILINEARTEXTURES, checked);
}

void SettingsDialog::on_checkBox_enable_audio_clicked(bool checked)
{
	CAppConfig::GetInstance().SetPreferenceBoolean(PREFERENCE_AUDIO_ENABLEOUTPUT, checked);
}

void SettingsDialog::on_comboBox_presentation_mode_currentIndexChanged(int index)
{
	CAppConfig::GetInstance().SetPreferenceInteger(PREF_CGSHANDLER_PRESENTATION_MODE, index);
}

void SettingsDialog::on_spinBox_resolution_valueChanged(int val)
{
	CAppConfig::GetInstance().SetPreferenceInteger(PREF_CGSH_OPENGL_RESOLUTION_FACTOR, val);
}
