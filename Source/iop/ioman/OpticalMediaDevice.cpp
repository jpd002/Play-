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
	return fileSystem->Open(fixedString.c_str());
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
