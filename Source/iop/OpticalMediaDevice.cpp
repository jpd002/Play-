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

Framework::CStream* COpticalMediaDevice::GetFile(uint32 mode, const char* devicePath)
{
	if((mode & OPEN_FLAG_ACCMODE) != OPEN_FLAG_RDONLY) return nullptr;
	if(!m_opticalMedia) return nullptr;
	std::string fixedString(devicePath);
	transform(fixedString.begin(), fixedString.end(), fixedString.begin(), &COpticalMediaDevice::FixSlashes);
	auto fileSystem = m_opticalMedia->GetFileSystem();
	return fileSystem->Open(fixedString.c_str());
}
