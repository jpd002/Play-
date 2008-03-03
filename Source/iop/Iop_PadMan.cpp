#include <assert.h>
#include <stdio.h>
#include <boost/bind.hpp>
#include "Iop_PadMan.h"
#include "../Log.h"
#include "../RegisterStateFile.h"

using namespace Iop;
using namespace Framework;
using namespace boost;
using namespace std;

#define PADNUM			(1)
#define MODE			(0x4)
#define LOG_NAME        "iop_padman"

#define STATE_PADDATA           ("iop_padman/paddata.xml")
#define STATE_PADDATA_ADDRESS   ("address")
#define STATE_PADDATA_TYPE      ("type")

CPadMan::CPadMan(CSIF& sif)
{
	m_nPadDataAddress = 0;
	m_nPadDataType = 0;
	m_pPad = NULL;

    sif.RegisterModule(MODULE_ID_1, this);
    sif.RegisterModule(MODULE_ID_2, this);
    sif.RegisterModule(MODULE_ID_3, this);
    sif.RegisterModule(MODULE_ID_4, this);
}

string CPadMan::GetId() const
{
    return "padman";
}

void CPadMan::Invoke(CMIPS& context, unsigned int functionId)
{
    throw runtime_error("Not implemented.");
}

void CPadMan::Invoke(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	assert(method == 1);
	method = args[0];
	switch(method)
	{
	case 0x00000001:
	case 0x80000100:
		Open(args, argsSize, ret, retSize, ram);
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
		CLog::GetInstance().Print(LOG_NAME, "Unknown method invoked (0x%0.8X).\r\n", method);
		break;
	}
}

void CPadMan::SaveState(CZipArchiveWriter& archive)
{	
    CRegisterStateFile* registerFile = new CRegisterStateFile(STATE_PADDATA);

    registerFile->SetRegister32(STATE_PADDATA_ADDRESS,  m_nPadDataAddress);
    registerFile->SetRegister32(STATE_PADDATA_TYPE,     m_nPadDataType);

    archive.InsertFile(registerFile);
}

void CPadMan::LoadState(CZipArchiveReader& archive)
{
    CRegisterStateFile registerFile(*archive.BeginReadFile(STATE_PADDATA));
    m_nPadDataAddress   = registerFile.GetRegister32(STATE_PADDATA_ADDRESS);
    m_nPadDataType      = registerFile.GetRegister32(STATE_PADDATA_TYPE);
}

void CPadMan::SetButtonState(unsigned int nPadNumber, CPadListener::BUTTON nButton, bool nPressed, uint8* ram)
{
	if(m_nPadDataAddress == 0) return;

	ExecutePadDataFunction(bind(&CPadMan::PDF_SetButtonState, _1, nButton, nPressed),
		ram + m_nPadDataAddress, PADNUM);
}

void CPadMan::Open(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	uint32 nPort        = args[1];
	uint32 nSlot        = args[2];
	uint32 nAddress     = args[4];

	if(nPort == 0)
	{
		m_nPadDataAddress = nAddress;
		m_pPad = reinterpret_cast<PADDATA*>(ram + nAddress);
	}

	CLog::GetInstance().Print(LOG_NAME, "Opening device on port %i and slot %i.\r\n", nPort, nSlot);

	ExecutePadDataFunction(&CPadMan::PDF_InitializeStruct0, ram + m_nPadDataAddress, 0);
	ExecutePadDataFunction(&CPadMan::PDF_InitializeStruct1, ram + m_nPadDataAddress, 1);

	//Returns 0 on error
	ret[3] = 0x00000001;
}

void CPadMan::SetActuatorAlign(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	assert(retSize >= 24);

	ret[5] = 1;
}

void CPadMan::Init(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	assert(retSize >= 0x10);

	m_nPadDataType = 1;

	CLog::GetInstance().Print(LOG_NAME, "Init();\r\n");

	ret[3] = 1;
}

void CPadMan::GetModuleVersion(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	assert(retSize >= 0x10);

	CLog::GetInstance().Print(LOG_NAME, "GetModuleVersion();\r\n");

	ret[3] = 0x00000400;
}

void CPadMan::ExecutePadDataFunction(PadDataFunction Function, void* pBase, size_t nOffset)
{
	switch(m_nPadDataType)
	{
	case 0:
		Function(&CPadDataHandler<PADDATA>(reinterpret_cast<PADDATA*>(pBase) + nOffset));
		break;
	case 1:
		Function(&CPadDataHandler<PADDATAEX>(reinterpret_cast<PADDATAEX*>(pBase) + nOffset));
		break;
	}
}

void CPadMan::PDF_InitializeStruct0(CPadDataInterface* pPadData)
{
	pPadData->SetFrame(0);
	pPadData->SetState(6);
	pPadData->SetReqState(0);
	pPadData->SetLength(32);
	pPadData->SetOk(1);
}

void CPadMan::PDF_InitializeStruct1(CPadDataInterface* pPadData)
{
	pPadData->SetFrame(1);
	pPadData->SetState(6);
	pPadData->SetReqState(0);
	pPadData->SetLength(32);
	pPadData->SetOk(1);

	//EX struct initialization
	pPadData->SetModeCurId(MODE << 4);
	pPadData->SetModeCurOffset(0);
	pPadData->SetModeTable(0, MODE);
}

void CPadMan::PDF_SetButtonState(CPadDataInterface* pPadData, BUTTON nButton, bool nPressed)
{
	uint16 nStatus;

	nStatus = (pPadData->GetData(2) << 8) | (pPadData->GetData(3));

	nStatus &= ~nButton;
	if(!nPressed)
	{
		nStatus |= nButton;
	}

	pPadData->SetReqState(0);

	pPadData->SetData(2, static_cast<uint8>(nStatus >> 8));
	pPadData->SetData(3, static_cast<uint8>(nStatus >> 0));

	pPadData->SetData(0, 0);
	pPadData->SetData(1, MODE << 4);
}
