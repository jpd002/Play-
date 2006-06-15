#include "FileDialog.h"

CFileDialog::CFileDialog()
{
	memset(&m_OFN, 0, sizeof(OPENFILENAME));
	m_OFN.lStructSize	= sizeof(OPENFILENAME);
	m_OFN.lpstrFile		= m_sFile;
	m_OFN.nMaxFile		= MAX_PATH;
	xstrcpy(m_sFile, _X(""));
}

CFileDialog::~CFileDialog()
{
	
}

int CFileDialog::Summon(HWND hParent)
{
	int nRet;
	xchar sPath[MAX_PATH + 1];
	GetCurrentDirectory(MAX_PATH, sPath);

	m_OFN.hwndOwner = hParent;
	nRet = GetOpenFileName(&m_OFN);

	SetCurrentDirectory(sPath);
	return nRet;
}
