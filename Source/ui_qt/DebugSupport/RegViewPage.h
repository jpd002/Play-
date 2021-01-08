#pragma once
#include <string>
#include <QTableWidget>

class CRegViewPage : public QTableWidget
{
public:
	CRegViewPage(QWidget*);
	virtual ~CRegViewPage() = default;

	virtual void Update() = 0;

protected:
	void AllocateTableEntries(unsigned int, unsigned int);
	void WriteTableLabel(unsigned int, const char*, ...);
	void WriteTableEntry(unsigned int, const char*, ...);
};
