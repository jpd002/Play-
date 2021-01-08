#pragma once

#include <string>
#include "RegViewPage.h"
#include "MIPS.h"

class CRegViewFPU : public CRegViewPage
{
public:
	CRegViewFPU(QWidget*, CMIPS*);
	virtual ~CRegViewFPU() = default;

	void Update() override;

protected:
	void ShowContextMenu(const QPoint&);

private:
	enum VIEWMODE
	{
		VIEWMODE_WORD,
		VIEWMODE_SINGLE,
		VIEWMODE_MAX,
	};

	void RenderWord();
	void RenderSingle();
	void RenderFCSR();

	void OnMachineStateChange();
	void OnRunningStateChange();

	VIEWMODE m_nViewMode;
	CMIPS* m_pCtx = nullptr;
};
