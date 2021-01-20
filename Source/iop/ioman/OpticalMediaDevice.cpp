#include "OpticalMediaDevice.h"
#include <algorithm>
#include <string>

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

Directory COpticalMediaDevice::GetDirectory(const char* devicePath)
{
	throw std::runtime_error("Not supported.");
}
