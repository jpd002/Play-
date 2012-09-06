#include <boost/filesystem/operations.hpp>
#include <exception>
#include "string_cast_sjis.h"
#include "Save.h"
#include "StdStream.h"
#include "StdStreamUtils.h"

namespace filesystem = boost::filesystem;

CSave::CSave(const filesystem::path& basePath) 
: m_basePath(basePath)
{
	filesystem::path iconSysPath = m_basePath / "icon.sys";

	auto iconStream(Framework::CreateInputStdStream(iconSysPath.native()));

	uint32 nMagic = iconStream.Read32();
	if(nMagic != 0x44325350)
	{
		throw std::runtime_error("Invalid 'icon.sys' file.");
	}

	iconStream.Seek(6, Framework::STREAM_SEEK_SET);
	m_nSecondLineStartPosition = iconStream.Read16();

	iconStream.Seek(192, Framework::STREAM_SEEK_SET);
	ReadName(iconStream);

	char sBuffer[64];

	iconStream.Read(sBuffer, 64);
	m_sNormalIconFileName = sBuffer;

	iconStream.Read(sBuffer, 64);
	m_sCopyingIconFileName = sBuffer;

	iconStream.Read(sBuffer, 64);
	m_sDeletingIconFileName = sBuffer;

	m_sId = m_basePath.filename().string();

	m_nLastModificationTime = filesystem::last_write_time(iconSysPath);
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

	for(filesystem::directory_iterator itElement(m_basePath);
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
	return m_basePath;
}

filesystem::path CSave::GetIconPath(const ICONTYPE& iconType) const
{
	filesystem::path iconPath;
	switch(iconType)
	{
	case ICON_NORMAL:
		iconPath = GetNormalIconPath();
		break;
	case ICON_DELETING:
		iconPath = GetDeletingIconPath();
		break;
	case ICON_COPYING:
		iconPath = GetCopyingIconPath();
		break;
	}
	return iconPath;
}

filesystem::path CSave::GetNormalIconPath() const
{
	return m_basePath / m_sNormalIconFileName;
}

filesystem::path CSave::GetDeletingIconPath() const
{
	return m_basePath / m_sDeletingIconFileName;
}

filesystem::path CSave::GetCopyingIconPath() const
{
	return m_basePath / m_sCopyingIconFileName;
}

size_t CSave::GetSecondLineStartPosition() const
{
	return std::min<size_t>(m_nSecondLineStartPosition / 2, m_sName.length());
}

time_t CSave::GetLastModificationTime() const
{
	return m_nLastModificationTime;
}

void CSave::ReadName(Framework::CStream& inputStream)
{
	char sNameSJIS[68];
	inputStream.Read(sNameSJIS, 68);
	m_sName = string_cast_sjis(sNameSJIS);
}
