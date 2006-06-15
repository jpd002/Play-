#include <string.h>
#include "Utils.h"

using namespace Framework;

void Utils::GetLine(CStream* pS, CStrA* pStr, bool nIgnoreCR)
{
	unsigned char nChar;

	(*pStr) = "";

	pS->Read(&nChar, 1);
	while(!pS->IsEOF())
	{
		if(!(nIgnoreCR && (nChar == '\r')))
		{
			(*pStr) += nChar;
		}
		pS->Read(&nChar, 1);
		if(nChar == '\n') break;
	}
}

const char* Utils::GetFilenameFromPath(const char* sPath, char nSeparator)
{
	const char* sTemp;
	sTemp = strchr(sPath, nSeparator);
	while(sTemp != NULL)
	{
		sPath = sTemp + 1;
		sTemp = strchr(sPath, nSeparator);
	}
	return sPath;
}
