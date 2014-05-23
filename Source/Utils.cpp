#include <string.h>
#include "Utils.h"

std::string Utils::GetLine(Framework::CStream* stream, bool nIgnoreCR)
{
	std::string result;
	unsigned char nChar = 0;
	stream->Read(&nChar, 1);
	while(!stream->IsEOF())
	{
		if(!(nIgnoreCR && (nChar == '\r')))
		{
			result += nChar;
		}
		stream->Read(&nChar, 1);
		if(nChar == '\n') break;
	}
	return result;
}
