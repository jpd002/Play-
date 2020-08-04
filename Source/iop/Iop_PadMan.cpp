#include <assert.h>
#include <stdio.h>
#include "Iop_PadMan.h"
#include "../Log.h"
#include "../states/RegisterStateFile.h"
#include "placeholder_def.h"

using namespace Iop;
using namespace PS2;

#define PADNUM (1)
//#define MODE (0x7) //DUAL SHOCK
#define MODE (0x4) //DIGITAL
#define LOG_NAME "iop_padman"

#define STATE_PADDATA ("iop_padman/paddata.xml")
#define STATE_PADDATA_ADDRESS ("address")
#define STATE_PADDATA_TYPE ("type")

CPadMan::CPadMan()
    : m_nPadDataAddress(0)
    , m_nPadDataAddress1(0)
    , m_nPadDataType(PAD_DATA_STD)
{
}

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
		CLog::GetInstance().Print(LOG_NAME, "Unknown method invoked (0x%08X).\r\n", method);
		break;
	}
	return true;
}

void CPadMan::SaveState(Framework::CZipArchiveWriter& archive)
{
	CRegisterStateFile* registerFile = new CRegisterStateFile(STATE_PADDATA);

	registerFile->SetRegister32(STATE_PADDATA_ADDRESS, m_nPadDataAddress);
	registerFile->SetRegister32(STATE_PADDATA_TYPE, m_nPadDataType);

	archive.InsertFile(registerFile);
}

void CPadMan::LoadState(Framework::CZipArchiveReader& archive)
{
	CRegisterStateFile registerFile(*archive.BeginReadFile(STATE_PADDATA));
	m_nPadDataAddress = registerFile.GetRegister32(STATE_PADDATA_ADDRESS);
	m_nPadDataType = static_cast<PAD_DATA_TYPE>(registerFile.GetRegister32(STATE_PADDATA_TYPE));
}

void CPadMan::SetButtonState(unsigned int nPadNumber, CControllerInfo::BUTTON nButton, bool nPressed, uint8* ram)
{
	if(m_nPadDataAddress1 != 0)
	{
		CPadDataHandler<PADDATA> padData(reinterpret_cast<PADDATA*>(ram + m_nPadDataAddress1) + 1);
		padData.SetReqState(0);
	}

	if(m_nPadDataAddress == 0) return;

	ExecutePadDataFunction(std::bind(&CPadMan::PDF_SetButtonState, PLACEHOLDER_1, nButton, nPressed),
	                       ram + m_nPadDataAddress, PADNUM);
}

void CPadMan::SetAxisState(unsigned int padNumber, CControllerInfo::BUTTON button, uint8 axisValue, uint8* ram)
{
	if(m_nPadDataAddress == 0) return;

	ExecutePadDataFunction(std::bind(&CPadMan::PDF_SetAxisState, std::placeholders::_1, button, axisValue),
	                       ram + m_nPadDataAddress, PADNUM);
}

void CPadMan::Open(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	uint32 nPort = args[1];
	uint32 nSlot = args[2];
	uint32 nAddress = args[4];

	CLog::GetInstance().Print(LOG_NAME, "Open(port = %d, slot = %d, padAreaAddr = 0x%08x);\r\n",
	                          nPort, nSlot, nAddress);

	if(nPort == 0)
	{
		m_nPadDataAddress = nAddress;

		m_nPadDataType = GetDataType(ram + m_nPadDataAddress);

		CLog::GetInstance().Print(LOG_NAME, "Detected data type %d.\r\n", m_nPadDataType);

		ExecutePadDataFunction(&CPadMan::PDF_InitializeStruct0, ram + m_nPadDataAddress, 0);
		ExecutePadDataFunction(&CPadMan::PDF_InitializeStruct1, ram + m_nPadDataAddress, 1);
	}
	else if(nPort == 1)
	{
		m_nPadDataAddress1 = nAddress;

		ExecutePadDataFunction(&CPadMan::PDF_InitializeStruct0, ram + nAddress, 0);
		ExecutePadDataFunction(&CPadMan::PDF_InitializeStruct1, ram + nAddress, 1);
	}

	//Returns 0 on error
	ret[3] = 0x00000001;
}

void CPadMan::Close(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	uint32 port = args[1];
	uint32 slot = args[2];
	uint32 wait = args[4];

	CLog::GetInstance().Print(LOG_NAME, "Close(port = %d, slot = %d, wait = %d);\r\n",
	                          port, slot, wait);

	if(port == 0)
	{
		m_nPadDataAddress = 0;
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
	switch(m_nPadDataType)
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

void CPadMan::PDF_InitializeStruct1(CPadDataInterface* pPadData)
{
	pPadData->SetFrame(1);
	pPadData->SetState(6);
	pPadData->SetReqState(0);
	pPadData->SetLength(32);
	pPadData->SetOk(1);

	//Reset analog sticks
	for(unsigned int i = 0; i < 4; i++)
	{
		pPadData->SetData(4 + i, 0x7F);
	}

	//EX struct initialization
	pPadData->SetModeCurId(MODE << 4);
	pPadData->SetModeCurOffset(0);
	pPadData->SetModeTable(0, MODE);
	pPadData->SetNumberOfModes(4);
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

	pPadData->SetData(0, 0);
	pPadData->SetData(1, MODE << 4);
}

void CPadMan::PDF_SetAxisState(CPadDataInterface* padData, CControllerInfo::BUTTON axis, uint8 axisValue)
{
	//rjoy_h 4;
	//rjoy_v 5;
	//ljoy_h 6;
	//ljoy_v 7;

	assert(axis < 4);

	unsigned int axisIndex[4] =
	    {
	        6,
	        7,
	        4,
	        5};

	padData->SetReqState(0);

	padData->SetData(axisIndex[axis], axisValue);

	padData->SetData(0, 0);
	padData->SetData(1, MODE << 4);
}
