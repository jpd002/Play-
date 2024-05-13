#include "Iop_Usbd.h"
#include "IopBios.h"
#include "../Log.h"

using namespace Iop;

#define LOG_NAME "iop_usbd"

#define FUNCTION_REGISTERLLD "RegisterLld"
#define FUNCTION_SCANSTATICDESCRIPTOR "ScanStaticDescriptor"
#define FUNCTION_OPENPIPE "OpenPipe"
#define FUNCTION_TRANSFERPIPE "TransferPipe"
#define FUNCTION_GETDEVICELOCATION "GetDeviceLocation"

static constexpr uint32 g_deviceId = 0xBEEF;
static constexpr uint32 g_controlPipeId = 0xCAFE;
static constexpr uint32 g_pipeId = 0xDEAD;

CUsbd::CUsbd(CIopBios& bios, uint8* ram)
    : m_bios(bios)
    , m_ram(ram)
{
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
	if(m_nextTransferTicks != 0)
	{
		m_nextTransferTicks -= ticks;
		if(m_nextTransferTicks <= 0)
		{
			uint8* buffer = m_ram + m_transferBufferPtr;
			buffer[0] = 0x7F;
			buffer[1] = 0x7F;
			buffer[2] = m_buttonState;
			buffer[3] = 0x00;
			buffer[4] = 0xF0;
			m_bios.TriggerCallback(m_transferCb, 0, m_transferSize, m_transferCbArg);
			m_nextTransferTicks = 0;
			m_transferCb = 0;
		}
	}
}

void CUsbd::SetButtonState(unsigned int padNumber, PS2::CControllerInfo::BUTTON button, bool pressed, uint8* ram)
{
	if(padNumber == 0)
	{
		uint8 mask = 0;
		switch(button)
		{
		case PS2::CControllerInfo::CROSS:
			mask = 1;
			break;
		case PS2::CControllerInfo::CIRCLE:
			mask = 2;
			break;
		case PS2::CControllerInfo::SQUARE:
			mask = 4;
			break;
		case PS2::CControllerInfo::TRIANGLE:
			mask = 8;
			break;
		case PS2::CControllerInfo::DPAD_UP:
			mask = 0x10;
			break;
		}
		if(mask != 0)
		{
			m_buttonState &= ~mask;
			if(pressed) m_buttonState |= mask;
		}
	}
}

int32 CUsbd::RegisterLld(uint32 lldOpsPtr)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_REGISTERLLD "(lldOpsPtr = 0x%08X);\r\n",
	                          lldOpsPtr);

	auto lldOps = reinterpret_cast<const LLDOPS*>(m_ram + lldOpsPtr);
	const char* name = reinterpret_cast<const char*>(m_ram + lldOps->namePtr);
	if(!strcmp(name, "buzzer"))
	{
		m_descriptorMemPtr = m_bios.GetSysmem()->AllocateMemory(0x80, 0, 0);
		m_bios.TriggerCallback(lldOps->connectFctPtr, g_deviceId);
	}
	return 0;
}

int32 CUsbd::ScanStaticDescriptor(uint32 deviceId, uint32 descriptorPtr, uint32 descriptorType)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_SCANSTATICDESCRIPTOR "(deviceId = 0x%08X, descriptorPtr = 0x%08X, descriptorType = %d);\r\n",
	                          deviceId, descriptorPtr, descriptorType);

	assert(deviceId == g_deviceId);

	//Details about descriptors here:
	//https://www.beyondlogic.org/usbnutshell/usb5.shtml

	uint32 result = 0;
	switch(descriptorType)
	{
	case 1:
	{
		//Device descriptor
		uint8* descriptorMem = reinterpret_cast<uint8*>(m_ram + m_descriptorMemPtr);
		descriptorMem[1] = descriptorType;
		result = m_descriptorMemPtr;
	}
	break;
	case 2:
	{
		//Configuration descriptor
		uint8* descriptorMem = reinterpret_cast<uint8*>(m_ram + m_descriptorMemPtr);
		descriptorMem[1] = descriptorType;
		descriptorMem[4] = 1;
		result = m_descriptorMemPtr;
	}
	break;
	case 4:
	{
		//Interface descriptor
		uint8* descriptorMem = reinterpret_cast<uint8*>(m_ram + m_descriptorMemPtr);
		descriptorMem[1] = descriptorType;
		descriptorMem[4] = 1;
		result = m_descriptorMemPtr;
	}
	break;
	case 5:
	{
		//Endpoint descriptor
		uint8* descriptorMem = reinterpret_cast<uint8*>(m_ram + m_descriptorMemPtr);
		if(descriptorMem[1] != descriptorType)
		{
			descriptorMem[1] = descriptorType;
			descriptorMem[2] = 0x80;
			descriptorMem[3] = 0x3;
			result = m_descriptorMemPtr;
		}
	}
	break;
	}

	return result;
}

int32 CUsbd::OpenPipe(uint32 deviceId, uint32 descriptorPtr)
{
	CLog::GetInstance().Warn(LOG_NAME, FUNCTION_OPENPIPE "(deviceId = 0x%08X, descriptorPtr = 0x%08X);\r\n",
	                         deviceId, descriptorPtr);
	if(descriptorPtr != 0)
	{
		return g_pipeId;
	}
	else
	{
		return g_controlPipeId;
	}
}

int32 CUsbd::TransferPipe(uint32 pipeId, uint32 bufferPtr, uint32 size, uint32 optionPtr, uint32 doneCb, uint32 arg)
{
	CLog::GetInstance().Warn(LOG_NAME, FUNCTION_TRANSFERPIPE "(pipeId = 0x%08X, bufferPtr = 0x%08X, length = %d, optionPtr = 0x%08X, doneCb = 0x%08X, arg = 0x%08X);\r\n",
	                         pipeId, bufferPtr, size, optionPtr, doneCb, arg);
	if(pipeId == g_controlPipeId)
	{
		m_bios.TriggerCallback(doneCb, 0, size, arg);
	}
	else
	{
		m_transferBufferPtr = bufferPtr;
		m_transferSize = size;
		m_transferCb = doneCb;
		m_transferCbArg = arg;
		m_nextTransferTicks = 33000000;
	}
	return 0;
}

int32 CUsbd::GetDeviceLocation(uint32 deviceId, uint32 locationPtr)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_GETDEVICELOCATION "(deviceId = 0x%08X, locationPtr = 0x%08X);\r\n",
	                          deviceId, locationPtr);

	assert(deviceId == g_deviceId);
	uint8* location = reinterpret_cast<uint8*>(m_ram + locationPtr);
	memset(location, 0, 7);
	location[0] = 1;

	return 0;
}
