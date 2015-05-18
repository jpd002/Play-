#include "IsoDevice.h"
#include <algorithm>
#include <string>

using namespace Iop::Ioman;

CIsoDevice::CIsoDevice(Iso9660Ptr& iso)
: m_iso(iso)
{

}

CIsoDevice::~CIsoDevice()
{

}

char CIsoDevice::FixSlashes(char input)
{
	if(input == '\\') return '/';
	return input;
}

Framework::CStream* CIsoDevice::GetFile(uint32 mode, const char* devicePath)
{
	if(mode != OPEN_FLAG_RDONLY) return nullptr;
	if(!m_iso) return nullptr;
	std::string fixedString(devicePath);
	transform(fixedString.begin(), fixedString.end(), fixedString.begin(), &CIsoDevice::FixSlashes);
	return m_iso->Open(fixedString.c_str());
}
