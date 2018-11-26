#include "InputProviderEvDev.h"
#include "string_format.h"

#define PROVIDER_ID 'evdv'

CInputProviderEvDev::CInputProviderEvDev()
{

}

CInputProviderEvDev::~CInputProviderEvDev()
{

}

uint32 CInputProviderEvDev::GetId() const
{
	return PROVIDER_ID;
}

std::string CInputProviderEvDev::GetTargetDescription(const BINDINGTARGET& target) const
{
	return string_format("EvDev: btn-%d", target.keyId);
}
