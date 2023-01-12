#include "OpticalMediaDevice.h"
#include <algorithm>
#include <string>
#include "OpticalMediaDirectoryIterator.h"

using namespace Iop::Ioman;

COpticalMediaDevice::COpticalMediaDevice(OpticalMediaPtr& opticalMedia)
    : m_opticalMedia(opticalMedia)
{
}

char COpticalMediaDevice::FixSlashes(char input)
{
	if(input == '\\') return '/';
	return input;
}

std::string COpticalMediaDevice::RemoveExtraVersionSpecifiers(const std::string& path)
{
	//Some games (Samurai Legend Musashi, while saving) will use a path that has two version specifiers
	//ex.: "myfile.bin;1;1". Looks like a bug in the game, assuming the driver doesn't care about that
	auto fixedPath = path;
	auto semiColonPos = fixedPath.find(';');
	if(semiColonPos != std::string::npos)
	{
		auto nextSemiColonPos = fixedPath.find(';', semiColonPos + 1);
		if(nextSemiColonPos != std::string::npos)
		{
			fixedPath = std::string(std::begin(fixedPath), std::begin(fixedPath) + nextSemiColonPos);
		}
	}
	return fixedPath;
}

Framework::CStream* COpticalMediaDevice::GetFile(uint32 mode, const char* devicePath)
{
	if(!m_opticalMedia) return nullptr;
	std::string fixedString(devicePath);
	std::transform(fixedString.begin(), fixedString.end(), fixedString.begin(), &COpticalMediaDevice::FixSlashes);
	fixedString = RemoveExtraVersionSpecifiers(fixedString);
	auto fileSystem = m_opticalMedia->GetFileSystem();
	auto fileStream = std::unique_ptr<Framework::CStream>(fileSystem->Open(fixedString.c_str()));
	if(!fileStream)
	{
		return nullptr;
	}
	return new COpticalMediaFile(std::move(fileStream));
}

DirectoryIteratorPtr COpticalMediaDevice::GetDirectory(const char* devicePath)
{
	if(!m_opticalMedia) return nullptr;
	std::string fixedString(devicePath);
	std::transform(fixedString.begin(), fixedString.end(), fixedString.begin(), &COpticalMediaDevice::FixSlashes);
	fixedString.erase(fixedString.find_last_not_of('.') + 1); // Remove trailing dot, if there is any

	auto fileSystem = m_opticalMedia->GetFileSystem();
	auto directoryStream = fileSystem->OpenDirectory(fixedString.c_str());
	if(directoryStream == nullptr)
	{
		throw std::runtime_error("Directory not found.");
	}
	return std::make_unique<COpticalMediaDirectoryIterator>(directoryStream);
}

COpticalMediaFile::COpticalMediaFile(std::unique_ptr<Framework::CStream> baseStream)
    : m_baseStream(std::move(baseStream))
{
}

void COpticalMediaFile::Seek(int64 offset, Framework::STREAM_SEEK_DIRECTION whence)
{
	if((whence == Framework::STREAM_SEEK_END) && (offset > 0))
	{
		//Some games from Phoenix Games relies on a positive offset seeking backwards when using SEEK_END:
		//- Shadow of Ganymede
		//- Guerrilla Strike
		offset = -offset;
	}
	m_baseStream->Seek(offset, whence);
}

uint64 COpticalMediaFile::Tell()
{
	return m_baseStream->Tell();
}

uint64 COpticalMediaFile::Read(void* buffer, uint64 size)
{
	return m_baseStream->Read(buffer, size);
}

uint64 COpticalMediaFile::Write(const void* buffer, uint64 size)
{
	return m_baseStream->Write(buffer, size);
}

bool COpticalMediaFile::IsEOF()
{
	return m_baseStream->IsEOF();
}
