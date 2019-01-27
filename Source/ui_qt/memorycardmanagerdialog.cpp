#include <QFileDialog>
#include <QDateTime>
#include <QMessageBox>
#include "QStringUtils.h"
#include "memorycardmanagerdialog.h"
#include "ui_memorycardmanager.h"
#include "StdStream.h"
#include "StdStreamUtils.h"
#include "AppConfig.h"
#include "../PS2VM_Preferences.h"

MemoryCardManagerDialog::MemoryCardManagerDialog(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::MemoryCardManagerDialog)
    , m_MemoryCard0(CAppConfig::GetInstance().GetPreferencePath(PREF_PS2_MC0_DIRECTORY))
    , m_MemoryCard1(CAppConfig::GetInstance().GetPreferencePath(PREF_PS2_MC1_DIRECTORY))
{
	ui->setupUi(this);

	m_pMemoryCard[0] = &m_MemoryCard0;
	m_pMemoryCard[1] = &m_MemoryCard1;

	m_pCurrentMemoryCard = m_pMemoryCard[0];

	populateSaveList();

	ui->label_name->setWordWrap(true);
}

MemoryCardManagerDialog::~MemoryCardManagerDialog()
{
	delete ui;
}

void MemoryCardManagerDialog::on_import_saves_button_clicked()
{
	QFileDialog dialog(this);
	dialog.setDirectory(m_lastpath);
	dialog.setFileMode(QFileDialog::ExistingFiles);
	dialog.setNameFilter(tr("All Supported types (*.psu *.sps *.xps *.max);;EMS Memory Adapter Save Dumps (*.psu);;Sharkport/X-Port Save Dumps (*.sps; *.xps);;Action Replay MAX Save Dumps (*.max);;All files (*.*)"));
	if(dialog.exec())
	{
		QString fileName = dialog.selectedFiles().first();
		m_lastpath = QFileInfo(fileName).path();

		try
		{
			auto filePath = QStringToPath(fileName);
			auto input = Framework::CreateInputStdStream(filePath.native());
			CSaveImporter::ImportSave(input, m_pCurrentMemoryCard->GetBasePath(),
			                          std::bind(&MemoryCardManagerDialog::OnImportOverwrite, this, std::placeholders::_1));
		}
		catch(const std::exception& Exception)
		{
			QString msg("Couldn't import save(s):\n\n%1");
			QMessageBox messageBox;
			messageBox.critical(this, "Error", msg.arg(Exception.what()));
			messageBox.show();
			return;
		}

		m_pCurrentMemoryCard->RefreshContents();
		populateSaveList();
	}
}

CSaveImporterBase::OVERWRITE_PROMPT_RETURN MemoryCardManagerDialog::OnImportOverwrite(const boost::filesystem::path& filePath)
{
	std::string fileName = filePath.leaf().string();
	QString msg("File %1 already exists.\n\nOverwrite?");
	QMessageBox::StandardButton resBtn = QMessageBox::question(this, "Overwrite?",
	                                                           msg.arg(fileName.c_str()),
	                                                           QMessageBox::Yes | QMessageBox::No,
	                                                           QMessageBox::Yes);
	return (resBtn == QMessageBox::Yes) ? CSaveImporterBase::OVERWRITE_YES : CSaveImporterBase::OVERWRITE_NO;
}

void MemoryCardManagerDialog::on_comboBox_currentIndexChanged(int index)
{
	m_pCurrentMemoryCard = m_pMemoryCard[index];
	populateSaveList();
}

void MemoryCardManagerDialog::populateSaveList()
{
	unsigned int nItemCount = static_cast<unsigned int>(m_pCurrentMemoryCard->GetSaveCount());
	ui->savelistWidget->clear();

	for(unsigned int i = 0; i < nItemCount; i++)
	{
		QString name = QString::fromWCharArray(m_pCurrentMemoryCard->GetSaveByIndex(i)->GetName());
		ui->savelistWidget->addItem(name);
	}
}

void MemoryCardManagerDialog::on_savelistWidget_currentRowChanged(int currentRow)
{
	int nItemCount = static_cast<int>(m_pCurrentMemoryCard->GetSaveCount() - 1);
	if(currentRow >= 0 && currentRow <= nItemCount)
	{
		const CSave* save = m_pCurrentMemoryCard->GetSaveByIndex(currentRow);
		QDateTime* dt = new QDateTime;
		dt->setTime_t(save->GetLastModificationTime());
		QString datetime = dt->toUTC().toString("hh:mm - dd.MM.yyyy");

		ui->label_name->setText(QString::fromWCharArray(save->GetName()));
		ui->label_name->setMinimumSize(ui->label_name->sizeHint());
		ui->label_id->setText(QString(save->GetId()));
		QString size("%1kb");
		ui->label_size->setText(size.arg(QString::number(save->GetSize() / 1000)));
		ui->label_last_mod->setText(datetime);
		ui->delete_save_button->setEnabled(true);
		ui->export_save_button->setEnabled(true);
	}
	else
	{
		ui->label_name->setText("--");
		ui->label_id->setText("--");
		ui->label_size->setText("--");
		ui->label_last_mod->setText("--");
		ui->delete_save_button->setEnabled(false);
		ui->export_save_button->setEnabled(false);
	}
}

void MemoryCardManagerDialog::on_delete_save_button_clicked()
{
	QMessageBox::StandardButton resBtn = QMessageBox::question(this, "Overwrite?",
	                                                           "Are you sure you want to delete the currently selected entry",
	                                                           QMessageBox::Yes | QMessageBox::No,
	                                                           QMessageBox::Yes);
	if(resBtn == QMessageBox::Yes)
	{
		int currentRow = ui->savelistWidget->currentRow();
		int nItemCount = static_cast<int>(m_pCurrentMemoryCard->GetSaveCount() - 1);
		if(currentRow >= 0 && currentRow <= nItemCount)
		{
			const CSave* save = m_pCurrentMemoryCard->GetSaveByIndex(currentRow);
			boost::filesystem::remove_all(save->GetPath());
			m_pCurrentMemoryCard->RefreshContents();
			populateSaveList();
		}
	}
}

void MemoryCardManagerDialog::on_export_save_button_clicked()
{
	int currentRow = ui->savelistWidget->currentRow();
	int nItemCount = static_cast<int>(m_pCurrentMemoryCard->GetSaveCount() - 1);
	if(currentRow >= 0 && currentRow <= nItemCount)
	{
		const CSave* save = m_pCurrentMemoryCard->GetSaveByIndex(currentRow);
		if(save != NULL)
		{
			QFileDialog dialog(this);
			dialog.setAcceptMode(QFileDialog::AcceptSave);
			dialog.setNameFilter(tr("EMS Memory Adapter Save Dumps (*.psu)"));
			dialog.setDefaultSuffix("psu");
			if(dialog.exec())
			{
				QString fileName = dialog.selectedFiles().first();

				try
				{
					auto filePath = QStringToPath(fileName);
					auto output = Framework::CreateOutputStdStream(filePath.native());
					CSaveExporter::ExportPSU(output, save->GetPath());
				}
				catch(const std::exception& Exception)
				{
					QString msg("Couldn't export save(s):\n\n%1");
					QMessageBox messageBox;
					messageBox.critical(this, "Error", msg.arg(Exception.what()));
					messageBox.show();
					return;
				}

				QString msg("Save exported successfully.");
				QMessageBox messageBox;
				messageBox.information(this, "Success", msg);
				messageBox.show();
				return;
			}
			else
			{
				QString msg("Save export Cancelled.");
				QMessageBox messageBox;
				messageBox.warning(this, "Cancelled", msg);
				messageBox.show();
				return;
			}
		}
		else
		{
			QString msg("Save not found,\nPlease try again.");
			QMessageBox messageBox;
			messageBox.critical(this, "Error", msg);
			messageBox.show();
		}
	}
	else
	{
		QString msg("Invalid selection,\nPlease try again.");
		QMessageBox messageBox;
		messageBox.critical(this, "Error", msg);
		messageBox.show();
	}
}
