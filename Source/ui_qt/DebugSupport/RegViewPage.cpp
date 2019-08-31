#include "RegViewPage.h"

CRegViewPage::CRegViewPage(QWidget* Parent)//, const RECT& rect)
		: QTableWidget(Parent)
    //: m_font(Framework::Win32::CreateFont(_T("Courier New"), 8))
{
}

void CRegViewPage::AllocateTableEntries(unsigned int columnCount, unsigned int rowCount)
{
	this->setRowCount(rowCount);
	this->setColumnCount(columnCount);
	for(unsigned int x = 0; x < columnCount; x++)
	{
		for(unsigned int y = 0; y < rowCount; y++)
		{
			this->setItem(x, y, new QTableWidgetItem(""));
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
	if(this->item(cellToWrite, 0))
		this->item(cellToWrite, 0)->setText(sLine);
	else
		this->setItem(cellToWrite, 0, new QTableWidgetItem(sLine));
	va_end(args);
}

void CRegViewPage::WriteTableEntry(unsigned int cellToWrite, const char* format, ...)
{
	const size_t bufferSize = 256;
	va_list args;
	char sLine[bufferSize];

	va_start(args, format);
	vsnprintf(sLine, bufferSize, format, args);
	if(this->item(cellToWrite, 1))
		this->item(cellToWrite, 1)->setText(sLine);
	else	
		this->setItem(cellToWrite, 1, new QTableWidgetItem(sLine));
	va_end(args);
}
