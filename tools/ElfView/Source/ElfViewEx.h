#ifndef _ELFVIEWEX_H_
#define _ELFVIEWEX_H_

#include "ELFFile.h"
#include "ui_win32\ELFView.h"

class CElfViewEx : public CELFView
{
public:
	CElfViewEx(HWND);
	virtual ~CElfViewEx();

	void LoadElf(Framework::CStream&);

private:
	CElfFile* m_elf;
};

#endif
