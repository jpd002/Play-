#include "Iop_Usbd.h"
#include "IopBios.h"
#include "../Log.h"
#include "UsbBuzzerDevice.h"

using namespace Iop;

#define LOG_NAME "iop_usbd"

#define FUNCTION_REGISTERLLD "RegisterLld"
#define FUNCTION_SCANSTATICDESCRIPTOR "ScanStaticDescriptor"
#define FUNCTION_OPENPIPE "OpenPipe"
#define FUNCTION_TRANSFERPIPE "TransferPipe"
#define FUNCTION_GETDEVICELOCATION "GetDeviceLocation"

CUsbd::CUsbd(CIopBios& bios, uint8* ram)
    : m_bios(bios)
    , m_ram(ram)
{
	RegisterDevice(std::make_unique<CBuzzerUsbDevice>(bios, ram));
}

std::string CUsbd::GetId() const
{
	return "usbd";
}

std::string CUsbd::GetFunctionName(unsigned int functionId) const
{
	switch(functionId)
	{
	case 4:
		return FUNCTION_REGISTERLLD;
	case 6:
		return FUNCTION_SCANSTATICDESCRIPTOR;
	case 9:
		return FUNCTION_OPENPIPE;
	case 11:
		return FUNCTION_TRANSFERPIPE;
	case 13:
		return FUNCTION_GETDEVICELOCATION;
	default:
		return "unknown";
	}
}

void CUsbd::Invoke(CMIPS& context, unsigned int functionId)
{
	switch(functionId)
	{
	case 4:
		context.m_State.nGPR[CMIPS::V0].nD0 = RegisterLld(
		    context.m_State.nGPR[CMIPS::A0].nV0);
		break;
	case 6:
		context.m_State.nGPR[CMIPS::V0].nD0 = ScanStaticDescriptor(
		    context.m_State.nGPR[CMIPS::A0].nV0,
		    context.m_State.nGPR[CMIPS::A1].nV0,
		    context.m_State.nGPR[CMIPS::A2].nV0);
		break;
	case 9:
		context.m_State.nGPR[CMIPS::V0].nD0 = OpenPipe(
		    context.m_State.nGPR[CMIPS::A0].nV0,
		    context.m_State.nGPR[CMIPS::A1].nV0);
		break;
	case 11:
		context.m_State.nGPR[CMIPS::V0].nD0 = TransferPipe(
		    context.m_State.nGPR[CMIPS::A0].nV0,
		    context.m_State.nGPR[CMIPS::A1].nV0,
		    context.m_State.nGPR[CMIPS::A2].nV0,
		    context.m_State.nGPR[CMIPS::A3].nV0,
		    context.m_pMemoryMap->GetWord(context.m_State.nGPR[CMIPS::SP].nV0 + 0x10),
		    context.m_pMemoryMap->GetWord(context.m_State.nGPR[CMIPS::SP].nV0 + 0x14));
		break;
	case 13:
		context.m_State.nGPR[CMIPS::V0].nD0 = GetDeviceLocation(
		    context.m_State.nGPR[CMIPS::A0].nV0,
		    context.m_State.nGPR[CMIPS::A1].nV0);
		break;
	default:
		CLog::GetInstance().Warn(LOG_NAME, "Unknown function called (%d).\r\n", functionId);
		break;
	}
}

void CUsbd::CountTicks(uint32 ticks)
{
	for(auto activeDeviceId : m_activeDeviceIds)
	{
		assert(m_devices.find(activeDeviceId) != std::end(m_devices));
		auto& device = m_devices[activeDeviceId];
		device->CountTicks(ticks);
	}
}

void CUsbd::RegisterDevice(UsbDevicePtr device)
{
	auto result = m_devices.insert(std::make_pair(device->GetId(), std::move(device)));
	assert(result.second);
}

int32 CUsbd::RegisterLld(uint32 lldOpsPtr)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_REGISTERLLD "(lldOpsPtr = 0x%08X);\r\n",
	                          lldOpsPtr);

	auto lldOps = reinterpret_cast<const LLDOPS*>(m_ram + lldOpsPtr);
	const char* name = reinterpret_cast<const char*>(m_ram + lldOps->namePtr);
	for(const auto& devicePair : m_devices)
	{
		auto& device = devicePair.second;
		if(!strcmp(name, device->GetLldName()))
		{
			device->OnLldRegistered();
			m_activeDeviceIds.push_back(device->GetId());
			m_bios.TriggerCallback(lldOps->connectFctPtr, device->GetId());
		}
	}
	return 0;
}

int32 CUsbd::ScanStaticDescriptor(uint32 deviceId, uint32 descriptorPtr, uint32 descriptorType)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_SCANSTATICDESCRIPTOR "(deviceId = 0x%08X, descriptorPtr = 0x%08X, descriptorType = %d);\r\n",
	                          deviceId, descriptorPtr, descriptorType);

	auto deviceIteratorPair = m_devices.find(deviceId);
	if(deviceIteratorPair != std::end(m_devices))
	{
		auto& device = deviceIteratorPair->second;
		return device->ScanStaticDescriptor(deviceId, descriptorPtr, descriptorType);
	}
	else
	{
		CLog::GetInstance().Warn(LOG_NAME, FUNCTION_SCANSTATICDESCRIPTOR " called on unknown device id 0x%08X.\r\n", deviceId);
		return 0;
	}
}

int32 CUsbd::OpenPipe(uint32 deviceId, uint32 descriptorPtr)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_OPENPIPE "(deviceId = 0x%08X, descriptorPtr = 0x%08X);\r\n",
	                          deviceId, descriptorPtr);

	auto deviceIteratorPair = m_devices.find(deviceId);
	if(deviceIteratorPair != std::end(m_devices))
	{
		auto& device = deviceIteratorPair->second;
		uint16 pipeId = device->OpenPipe(deviceId, descriptorPtr);
		assert(pipeId < 0x8000);
		return (static_cast<uint32>(pipeId) << 16) | deviceId;
	}
	else
	{
		CLog::GetInstance().Warn(LOG_NAME, FUNCTION_OPENPIPE " called on unknown device id 0x%08X.\r\n", deviceId);
		return -1;
	}
}

int32 CUsbd::TransferPipe(uint32 pipeId, uint32 bufferPtr, uint32 size, uint32 optionPtr, uint32 doneCb, uint32 arg)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_TRANSFERPIPE "(pipeId = 0x%08X, bufferPtr = 0x%08X, length = %d, optionPtr = 0x%08X, doneCb = 0x%08X, arg = 0x%08X);\r\n",
	                          pipeId, bufferPtr, size, optionPtr, doneCb, arg);

	uint16 deviceId = (pipeId & 0xFFFF);
	auto deviceIteratorPair = m_devices.find(deviceId);
	if(deviceIteratorPair != std::end(m_devices))
	{
		auto& device = deviceIteratorPair->second;
		return device->TransferPipe(pipeId, bufferPtr, size, optionPtr, doneCb, arg);
	}
	else
	{
		CLog::GetInstance().Warn(LOG_NAME, FUNCTION_TRANSFERPIPE " called on unknown device id 0x%08X.\r\n", deviceId);
		return -1;
	}
}

int32 CUsbd::GetDeviceLocation(uint32 deviceId, uint32 locationPtr)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_GETDEVICELOCATION "(deviceId = 0x%08X, locationPtr = 0x%08X);\r\n",
	                          deviceId, locationPtr);

	auto location = reinterpret_cast<uint8*>(m_ram + locationPtr);
	memset(location, 0, 7);
	location[0] = 1;

	return 0;
}
