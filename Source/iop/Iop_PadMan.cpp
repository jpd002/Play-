#include "Iop_PadMan.h"
#include <cstdio>
#include <cassert>
#include "../Log.h"
#include "../states/RegisterStateFile.h"
#include "placeholder_def.h"

using namespace Iop;
using namespace PS2;

#define PADNUM (1)

#define PAD_MODE_DIGITAL 4
#define PAD_MODE_DUALSHOCK 7

#define LOG_NAME "iop_padman"

#define STATE_PADDATA ("iop_padman/paddata.xml")
#define STATE_PADDATA_PAD0_ADDRESS ("pad_address0")
#define STATE_PADDATA_PAD1_ADDRESS ("pad_address1")
#define STATE_PADDATA_TYPE ("type")

std::string CPadMan::GetId() const
{
	return "padman";
}

std::string CPadMan::GetFunctionName(unsigned int) const
{
	return "unknown";
}

void CPadMan::RegisterSifModules(CSifMan& sif)
{
	sif.RegisterModule(MODULE_ID_1, this);
	sif.RegisterModule(MODULE_ID_2, this);
	sif.RegisterModule(MODULE_ID_3, this);
	sif.RegisterModule(MODULE_ID_4, this);
}

void CPadMan::Invoke(CMIPS& context, unsigned int functionId)
{
	throw std::runtime_error("Not implemented.");
}

bool CPadMan::Invoke(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	assert(method == 1);
	method = args[0];
	switch(method)
	{
	case 0x00000001:
	case 0x80000100:
		Open(args, argsSize, ret, retSize, ram);
		break;
	case 0x80000105:
		SetMainMode(args, argsSize, ret, retSize, ram);
		break;
	case 0x8000010D:
		Close(args, argsSize, ret, retSize, ram);
		break;
	case 0x00000008:
		SetActuatorAlign(args, argsSize, ret, retSize, ram);
		break;
	case 0x00000010:
		Init(args, argsSize, ret, retSize, ram);
		break;
	case 0x00000012:
		GetModuleVersion(args, argsSize, ret, retSize, ram);
		break;
	default:
		CLog::GetInstance().Warn(LOG_NAME, "Unknown method invoked (0x%08X).\r\n", method);
		break;
	}
	return true;
}

void CPadMan::SaveState(Framework::CZipArchiveWriter& archive) const
{
	auto registerFile = std::make_unique<CRegisterStateFile>(STATE_PADDATA);

	registerFile->SetRegister32(STATE_PADDATA_PAD0_ADDRESS, m_padDataAddress[0]);
	registerFile->SetRegister32(STATE_PADDATA_PAD1_ADDRESS, m_padDataAddress[1]);
	registerFile->SetRegister32(STATE_PADDATA_TYPE, m_padDataType);

	archive.InsertFile(std::move(registerFile));
}

void CPadMan::LoadState(Framework::CZipArchiveReader& archive)
{
	CRegisterStateFile registerFile(*archive.BeginReadFile(STATE_PADDATA));
	m_padDataAddress[0] = registerFile.GetRegister32(STATE_PADDATA_PAD0_ADDRESS);
	m_padDataAddress[1] = registerFile.GetRegister32(STATE_PADDATA_PAD1_ADDRESS);
	m_padDataType = static_cast<PAD_DATA_TYPE>(registerFile.GetRegister32(STATE_PADDATA_TYPE));
}

void CPadMan::SetButtonState(unsigned int padNumber, CControllerInfo::BUTTON button, bool pressed, uint8* ram)
{
	if(padNumber >= MAX_PADS) return;

	uint32 padDataAddress = m_padDataAddress[padNumber];
	if(padDataAddress == 0) return;

	ExecutePadDataFunction(std::bind(&CPadMan::PDF_SetButtonState, PLACEHOLDER_1, button, pressed),
	                       ram + padDataAddress, PADNUM);
}

void CPadMan::SetAxisState(unsigned int padNumber, CControllerInfo::BUTTON button, uint8 axisValue, uint8* ram)
{
	if(padNumber >= MAX_PADS) return;

	uint32 padDataAddress = m_padDataAddress[padNumber];
	if(padDataAddress == 0) return;

	ExecutePadDataFunction(std::bind(&CPadMan::PDF_SetAxisState, std::placeholders::_1, button, axisValue),
	                       ram + padDataAddress, PADNUM);
}

void CPadMan::Open(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	uint32 port = args[1];
	uint32 slot = args[2];
	uint32 address = args[4];

	CLog::GetInstance().Print(LOG_NAME, "Open(port = %d, slot = %d, padAreaAddr = 0x%08x);\r\n",
	                          port, slot, address);

	if(port < MAX_PADS)
	{
		m_padDataAddress[port] = address;
		m_padDataType = GetDataType(ram + address);

		CLog::GetInstance().Print(LOG_NAME, "Detected data type %d.\r\n", m_padDataType);

		ExecutePadDataFunction(&CPadMan::PDF_InitializeStruct0, ram + address, 0);
		ExecutePadDataFunction(&CPadMan::PDF_InitializeStruct1, ram + address, 1);
	}

	//Returns 0 on error
	ret[3] = 0x00000001;
}

void CPadMan::SetMainMode(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	uint32 port = args[1];
	uint32 slot = args[2];
	uint32 mode = args[3];
	uint32 lock = args[4];

	CLog::GetInstance().Print(LOG_NAME, "SetMainMode(port = %d, slot = %d, mode = %d, lock = %d);\r\n",
	                          port, slot, mode, lock);

	assert(mode <= 1);

	if(port < MAX_PADS)
	{
		uint32 padDataAddress = m_padDataAddress[port];
		if(padDataAddress != 0)
		{
			ExecutePadDataFunction(std::bind(&CPadMan::PDF_SetMode, PLACEHOLDER_1, mode ? PAD_MODE_DUALSHOCK : PAD_MODE_DIGITAL),
			                       ram + padDataAddress, PADNUM);
		}
	}

	ret[3] = 1;
}

void CPadMan::Close(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	uint32 port = args[1];
	uint32 slot = args[2];
	uint32 wait = args[4];

	CLog::GetInstance().Print(LOG_NAME, "Close(port = %d, slot = %d, wait = %d);\r\n",
	                          port, slot, wait);

	if(port < MAX_PADS)
	{
		m_padDataAddress[port] = 0;
	}

	ret[3] = 1;
}

void CPadMan::SetActuatorAlign(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	assert(retSize >= 24);

	ret[5] = 1;
}

void CPadMan::Init(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	assert(retSize >= 0x10);

	CLog::GetInstance().Print(LOG_NAME, "Init();\r\n");

	ret[3] = 1;
}

void CPadMan::GetModuleVersion(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	assert(retSize >= 0x10);

	CLog::GetInstance().Print(LOG_NAME, "GetModuleVersion();\r\n");

	ret[3] = 0x00000400;
}

void CPadMan::ExecutePadDataFunction(const PadDataFunction& func, void* pBase, size_t nOffset)
{
	switch(m_padDataType)
	{
	case PAD_DATA_STD:
	{
		CPadDataHandler<PADDATA> padData(reinterpret_cast<PADDATA*>(pBase) + nOffset);
		func(&padData);
	}
	break;
	case PAD_DATA_STD80:
	{
		CPadDataHandler<PADDATA80> padData(reinterpret_cast<PADDATA80*>(pBase) + nOffset);
		func(&padData);
	}
	break;
	case PAD_DATA_EX:
	{
		CPadDataHandler<PADDATAEX> padData(reinterpret_cast<PADDATAEX*>(pBase) + nOffset);
		func(&padData);
	}
	break;
	}
}

CPadMan::PAD_DATA_TYPE CPadMan::GetDataType(uint8* dataAddress)
{
	PAD_DATA_TYPE result = PAD_DATA_STD;
	if((dataAddress[0x08] == 0xFF) && (dataAddress[0x48] == 0xFF))
	{
		result = PAD_DATA_STD;
	}
	if((dataAddress[0x08] == 0xFF) && (dataAddress[0x88] == 0xFF))
	{
		result = PAD_DATA_STD80;
	}
	if((dataAddress[0x00] == 0xFF) && (dataAddress[0x80] == 0xFF))
	{
		result = PAD_DATA_EX;
	}
	return result;
}

void CPadMan::PDF_InitializeStruct0(CPadDataInterface* pPadData)
{
	pPadData->SetFrame(0);
	pPadData->SetState(6);
	pPadData->SetReqState(0);
	pPadData->SetLength(32);
	pPadData->SetOk(1);

	//Reset analog sticks
	for(unsigned int i = 0; i < 4; i++)
	{
		pPadData->SetData(4 + i, 0x7F);
	}
}

void CPadMan::PDF_InitializeStruct1(CPadDataInterface* padData)
{
	padData->SetFrame(1);
	padData->SetState(6);
	padData->SetReqState(0);
	padData->SetLength(32);
	padData->SetOk(1);

	//Reset analog sticks
	for(unsigned int i = 0; i < 4; i++)
	{
		padData->SetData(4 + i, 0x7F);
	}

	//Set pad mode
	padData->SetData(0, 0);
	padData->SetData(1, PAD_MODE_DUALSHOCK << 4);

	//EX struct initialization
	padData->SetModeCurId(PAD_MODE_DUALSHOCK << 4);
	padData->SetModeCurOffset(0);
	padData->SetModeTable(0, PAD_MODE_DUALSHOCK);
	padData->SetNumberOfModes(4);
}

void CPadMan::PDF_SetMode(CPadDataInterface* padData, uint8 mode)
{
	assert(mode < 0x0F);
	padData->SetData(0, 0);
	padData->SetData(1, mode << 4);
}

void CPadMan::PDF_SetButtonState(CPadDataInterface* pPadData, CControllerInfo::BUTTON nButton, bool nPressed)
{
	uint16 nStatus = (pPadData->GetData(2) << 8) | (pPadData->GetData(3));
	uint32 buttonMask = GetButtonMask(nButton);

	nStatus &= ~buttonMask;
	if(!nPressed)
	{
		nStatus |= buttonMask;
	}

	pPadData->SetReqState(0);

	pPadData->SetData(2, static_cast<uint8>(nStatus >> 8));
	pPadData->SetData(3, static_cast<uint8>(nStatus >> 0));
}

void CPadMan::PDF_SetAxisState(CPadDataInterface* padData, CControllerInfo::BUTTON axis, uint8 axisValue)
{
	assert(axis < 4);

	// clang-format off
	unsigned int axisIndex[4] =
	{
		6, //ljoy_h
		7, //ljoy_v
		4, //rjoy_h
		5  //rjoy_v
	};
	// clang-format on

	padData->SetReqState(0);

	padData->SetData(axisIndex[axis], axisValue);
}
