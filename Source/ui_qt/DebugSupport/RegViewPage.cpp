#include "RegViewPage.h"
#include <QHeaderView>
#include "DebugUtils.h"

CRegViewPage::CRegViewPage(QWidget* Parent)
    : QTableWidget(Parent)
{
	setFont(DebugUtils::CreateMonospaceFont());

	setEditTriggers(QAbstractItemView::NoEditTriggers);
	horizontalHeader()->setVisible(false);
	horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	verticalHeader()->setVisible(false);
}

void CRegViewPage::AllocateTableEntries(unsigned int columnCount, unsigned int rowCount)
{
	setRowCount(rowCount);
	setColumnCount(columnCount);
	for(unsigned int x = 0; x < columnCount; x++)
	{
		for(unsigned int y = 0; y < rowCount; y++)
		{
			setItem(x, y, new QTableWidgetItem(""));
		}
	}
}

void CRegViewPage::WriteTableLabel(unsigned int cellToWrite, const char* label, ...)
{
	const size_t bufferSize = 256;
	va_list args;
	char sLine[bufferSize];

	va_start(args, label);
	vsnprintf(sLine, bufferSize, label, args);
	if(item(cellToWrite, 0))
		item(cellToWrite, 0)->setText(sLine);
	else
		setItem(cellToWrite, 0, new QTableWidgetItem(sLine));
	va_end(args);
}

void CRegViewPage::WriteTableEntry(unsigned int cellToWrite, const char* format, ...)
{
	const size_t bufferSize = 256;
	va_list args;
	char sLine[bufferSize];

	va_start(args, format);
	vsnprintf(sLine, bufferSize, format, args);
	if(item(cellToWrite, 1))
		item(cellToWrite, 1)->setText(sLine);
	else
		setItem(cellToWrite, 1, new QTableWidgetItem(sLine));
	va_end(args);
}
