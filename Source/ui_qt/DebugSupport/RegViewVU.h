#pragma once

#include "RegViewPage.h"
#include "MIPS.h"

class CRegViewVU : public CRegViewPage
{
public:
	CRegViewVU(QWidget*, CMIPS*);
	virtual ~CRegViewVU() = default;

	void Update() override;

protected:
	void ShowContextMenu(const QPoint&);
	void DisplayWordMode();
	void DisplaySingleMode();
	void DisplayGeneral();

private:
	enum VIEWMODE
	{
		VIEWMODE_WORD,
		VIEWMODE_SINGLE,
		VIEWMODE_MAX,
	};
	static std::string PrintPipeline(const FLAG_PIPELINE&);

	CMIPS* m_ctx = nullptr;
	VIEWMODE m_viewMode = VIEWMODE_SINGLE;
};
