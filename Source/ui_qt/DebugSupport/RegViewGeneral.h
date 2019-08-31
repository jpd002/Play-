#pragma once

#include "RegViewPage.h"
#include "MIPS.h"
#include <string>

class CRegViewGeneral : public CRegViewPage
{
public:
	CRegViewGeneral(QWidget*, CMIPS*);
	virtual ~CRegViewGeneral() = default;

private:
	void Update() override;

	CMIPS* m_pCtx = nullptr;
};
