#pragma once

#include "RegViewPage.h"
#include "MIPS.h"

class CRegViewSCU : public CRegViewPage
{
public:
	CRegViewSCU(QWidget*, CMIPS*);
	virtual ~CRegViewSCU() = default;

private:
	void Update() override;

	CMIPS* m_ctx = nullptr;
};
