#pragma once

#include <QMdiArea>
#include "ELFView.h"
#include "filesystem_def.h"

class CElfViewEx : public CELFView
{
public:
	CElfViewEx(QMdiArea*, const fs::path&);
	virtual ~CElfViewEx();

private:
	CELF* m_elf = nullptr;
};
