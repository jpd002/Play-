#ifndef MEMORYCARDMANAGERDIALOG_H
#define MEMORYCARDMANAGERDIALOG_H

#include "saves/MemoryCard.h"
#include "saves/SaveExporter.h"
#include "saves/SaveImporter.h"
#include <QDialog>
#include <QDir>
#include <boost/filesystem.hpp>

namespace Ui
{
class MemoryCardManagerDialog;
}

class MemoryCardManagerDialog : public QDialog
{
	Q_OBJECT

public:
	explicit MemoryCardManagerDialog(QWidget* parent = 0);
	~MemoryCardManagerDialog();

private:
	CSaveImporterBase::OVERWRITE_PROMPT_RETURN OnImportOverwrite(const boost::filesystem::path&);
	void                                       populateSaveList();

	Ui::MemoryCardManagerDialog* ui;
	CMemoryCard                  m_MemoryCard0;
	CMemoryCard                  m_MemoryCard1;
	CMemoryCard*                 m_pMemoryCard[2];
	CMemoryCard*                 m_pCurrentMemoryCard;

	QString m_lastpath = QDir::homePath();

private slots:
	void on_import_saves_button_clicked();
	void on_comboBox_currentIndexChanged(int index);
	void on_savelistWidget_currentRowChanged(int currentRow);
	void on_delete_save_button_clicked();
	void on_export_save_button_clicked();
};

#endif // MEMORYCARDMANAGERDIALOG_H
