#include "PsfStreamProvider.h"
#include "StdStream.h"
#include "MemStream.h"
#include "StdStreamUtils.h"

std::unique_ptr<CPsfStreamProvider> CreatePsfStreamProvider(const boost::filesystem::path& archivePath)
{
	if(archivePath.empty())
	{
		return std::unique_ptr<CPsfStreamProvider>(new CPhysicalPsfStreamProvider());
	}
	else
	{
		return std::unique_ptr<CPsfStreamProvider>(new CArchivePsfStreamProvider(archivePath));
	}
}

//CPhysicalPsfStreamProvider
//----------------------------------------------------------
Framework::CStream*	CPhysicalPsfStreamProvider::GetStreamForPath(const CPsfPathToken& pathToken)
{
	auto path = GetFilePathFromPathToken(pathToken);
	return new Framework::CStdStream(Framework::CreateInputStdStream(path.native()));
}

CPsfPathToken CPhysicalPsfStreamProvider::GetPathTokenFromFilePath(const boost::filesystem::path& filePath)
{
	return filePath.wstring();
}

boost::filesystem::path CPhysicalPsfStreamProvider::GetFilePathFromPathToken(const CPsfPathToken& pathToken)
{
	return boost::filesystem::path(pathToken.GetWidePath());
}

CPsfPathToken CPhysicalPsfStreamProvider::GetSiblingPath(const CPsfPathToken& pathToken, const std::string& siblingName)
{
	auto path = GetFilePathFromPathToken(pathToken);
	auto siblingPath = path.remove_leaf() / siblingName;
	return GetPathTokenFromFilePath(siblingPath);
}

//CArchivePsfStreamProvider
//----------------------------------------------------------
CArchivePsfStreamProvider::CArchivePsfStreamProvider(const boost::filesystem::path& path)
{
	m_archive = CPsfArchive::CreateFromPath(path);
}

CArchivePsfStreamProvider::~CArchivePsfStreamProvider()
{

}

CPsfPathToken CArchivePsfStreamProvider::GetPathTokenFromFilePath(const std::string& filePath)
{
	return CPsfPathToken::WidenString(filePath);
}

std::string CArchivePsfStreamProvider::GetFilePathFromPathToken(const CPsfPathToken& pathToken)
{
	return pathToken.GetNarrowPath();
}

CPsfPathToken CArchivePsfStreamProvider::GetSiblingPath(const CPsfPathToken& pathToken, const std::string& siblingName)
{
	auto pathString = pathToken.GetWidePath();
	auto slashPos = pathString.find_last_of(L"/");
	if(slashPos == std::wstring::npos)
	{
		slashPos = pathString.find_last_of(L"\\");
		if(slashPos == std::wstring::npos)
		{
			slashPos = -1;
		}
	}
	slashPos++;
	auto stem = std::wstring(std::begin(pathString), std::begin(pathString) + slashPos);
	auto result = stem + CPsfPathToken::WidenString(siblingName);
	return result;
}

Framework::CStream* CArchivePsfStreamProvider::GetStreamForPath(const CPsfPathToken& pathToken)
{
	auto pathString = GetFilePathFromPathToken(pathToken);
	std::replace(pathString.begin(), pathString.end(), '\\', '/');
	const auto& fileInfo(m_archive->GetFileInfo(pathString.c_str()));
	assert(fileInfo != nullptr);
	if(fileInfo == nullptr)
	{
		throw std::runtime_error("Couldn't find file in archive.");
	}
	auto result = new Framework::CMemStream();
	result->Allocate(fileInfo->length);
	m_archive->ReadFileContents(pathString.c_str(), result->GetBuffer(), fileInfo->length);
	return result;
}
