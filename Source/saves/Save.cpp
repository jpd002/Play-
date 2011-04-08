#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/filesystem/operations.hpp>
#include <exception>
#include "string_cast_sjis.h"
#include "Save.h"

using namespace std;
namespace iostreams = boost::iostreams;
namespace filesystem = boost::filesystem;

CSave::CSave(filesystem::path& BasePath) :
m_BasePath(BasePath)
{
	uint32 nMagic;
	char sBuffer[64];
	filesystem::path IconSysPath;

	IconSysPath = m_BasePath / "icon.sys";

	iostreams::stream_buffer<iostreams::file_source> IconFileBuffer(IconSysPath.string(), ios::in | ios::binary);
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

	IconFile.read(sBuffer, 64);
	m_sCopyingIconFileName = sBuffer;

	IconFile.read(sBuffer, 64);
	m_sDeletingIconFileName = sBuffer;

	m_sId = m_BasePath.filename().string().c_str();

	m_nLastModificationTime = filesystem::last_write_time(IconSysPath);
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
	unsigned int nSize = 0;

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

filesystem::path CSave::GetDeletingIconPath() const
{
	return m_BasePath / m_sDeletingIconFileName;
}

filesystem::path CSave::GetCopyingIconPath() const
{
	return m_BasePath / m_sCopyingIconFileName;
}

size_t CSave::GetSecondLineStartPosition() const
{
	return min<size_t>(m_nSecondLineStartPosition / 2, m_sName.length());
}

time_t CSave::GetLastModificationTime() const
{
	return m_nLastModificationTime;
}

void CSave::ReadName(istream& Input)
{
	char sNameSJIS[68];
	Input.read(sNameSJIS, 68);
	m_sName = string_cast_sjis(sNameSJIS);
}
