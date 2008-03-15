#include "IsoDevice.h"

using namespace Framework;
using namespace Iop::Ioman;

CIsoDevice::CIsoDevice(CISO9660*& iso) :
m_iso(iso)
{

}

CIsoDevice::~CIsoDevice()
{

}

CStream* CIsoDevice::GetFile(uint32 mode, const char* devicePath)
{
	if(mode != O_RDONLY) return NULL;
    if(m_iso == NULL) return NULL;
	return m_iso->Open(devicePath);
}
