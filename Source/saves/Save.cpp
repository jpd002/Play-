#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/filesystem/operations.hpp>
#include <unicode/ucnv.h>
#include <exception>
#include "Save.h"

using namespace boost;
using namespace std;

CSave::CSave(filesystem::path& BasePath) :
m_BasePath(BasePath)
{
	uint32 nMagic;
	char sBuffer[64];
	iostreams::stream_buffer<iostreams::file_source> IconFileBuffer((m_BasePath / "icon.sys").string(), ios::in | ios::binary);
	istream IconFile(&IconFileBuffer);

	IconFile.read(reinterpret_cast<char*>(&nMagic), 4);
	if(nMagic != 0x44325350)
	{
		throw exception("Invalid 'icon.sys' file.");
	}

	IconFile.seekg(6);
	IconFile.read(reinterpret_cast<char*>(&m_nSecondLineStartPosition), 2);

	IconFile.seekg(192);
	ReadName(IconFile);

	IconFile.read(sBuffer, 64);
	m_sNormalIconFileName = sBuffer;

	m_sId = m_BasePath.leaf().c_str();
}

CSave::~CSave()
{

}

const wchar_t* CSave::GetName() const
{
	return m_sName.c_str();
}

const char* CSave::GetId() const
{
	return m_sId.c_str();
}

unsigned int CSave::GetSize() const
{
	filesystem::directory_iterator itEnd;
	unsigned int nSize;
	
	nSize = 0;

	for(filesystem::directory_iterator itElement(m_BasePath);
		itElement != itEnd;
		itElement++)
	{
		if(!filesystem::is_directory(*itElement))
		{
			nSize += filesystem::file_size(*itElement);
		}
	}

	return nSize;
}

filesystem::path CSave::GetPath() const
{
	return m_BasePath;
}

filesystem::path CSave::GetNormalIconPath() const
{
	return m_BasePath / m_sNormalIconFileName;
}

size_t CSave::GetSecondLineStartPosition() const
{
	return min<size_t>(m_nSecondLineStartPosition / 2, m_sName.length());
}

void CSave::ReadName(istream& Input)
{
	char sNameSJIS[68];
	UChar sName[68];
	UErrorCode nStatus;
	UConverter* pConverter;

	Input.read(sNameSJIS, 68);

	nStatus = U_ZERO_ERROR;

	pConverter = ucnv_open("shift_jis", &nStatus);
	ucnv_toUChars(pConverter, sName, 68, sNameSJIS, 68, &nStatus);
	ucnv_close(pConverter);

	m_sName = sName;
}
