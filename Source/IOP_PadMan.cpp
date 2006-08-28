#include <assert.h>
#include <stdio.h>
#include <boost/bind.hpp>
#include "IOP_PadMan.h"
#include "PS2VM.h"

using namespace IOP;
using namespace Framework;
using namespace boost;

#define PADNUM			(1)
#define MODE			(0x4)

CPadMan::CPadMan()
{
	m_nPadDataAddress = 0;
	m_nPadDataType = 0;
	m_pPad = NULL;
}

void CPadMan::Invoke(uint32 nMethod, void* pArgs, uint32 nArgsSize, void* pRet, uint32 nRetSize)
{
	assert(nMethod == 1);
	nMethod = ((uint32*)pArgs)[0];
	switch(nMethod)
	{
	case 0x00000001:
	case 0x80000100:
		Open(pArgs, nArgsSize, pRet, nRetSize);
		break;
	case 0x00000008:
		SetActuatorAlign(pArgs, nArgsSize, pRet, nRetSize);
		break;
	case 0x00000010:
		Init(pArgs, nArgsSize, pRet, nRetSize);
		break;
	case 0x00000012:
		GetModuleVersion(pArgs, nArgsSize, pRet, nRetSize);
		break;
	default:
		Log("Unknown method invoked (0x%0.8X).\r\n", nMethod);
		break;
	}
}

void CPadMan::SaveState(CStream* pStream)
{	
	pStream->Write(&m_nPadDataAddress,	4);
	pStream->Write(&m_nPadDataType,		4);
}

void CPadMan::LoadState(CStream* pStream)
{
	pStream->Read(&m_nPadDataAddress,	4);
	pStream->Read(&m_nPadDataType,		4);
}

void CPadMan::SetButtonState(unsigned int nPadNumber, CPadListener::BUTTON nButton, bool nPressed)
{
	if(m_nPadDataAddress == 0) return;

	ExecutePadDataFunction(bind(&CPadMan::PDF_SetButtonState, _1, nButton, nPressed),
		CPS2VM::m_pRAM + m_nPadDataAddress, PADNUM);
}

void CPadMan::Open(void* pArgs, uint32 nArgsSize, void* pRet, uint32 nRetSize)
{
	uint32 nPort, nSlot, nAddress;

	nPort		= ((uint32*)pArgs)[1];
	nSlot		= ((uint32*)pArgs)[2];
	nAddress	= ((uint32*)pArgs)[4];

	if(nPort == 0)
	{
		m_nPadDataAddress = nAddress;
		m_pPad = (PADDATA*)(CPS2VM::m_pRAM + nAddress);
	}

	Log("Opening device on port %i and slot %i.\r\n", nPort, nSlot);

	ExecutePadDataFunction(&CPadMan::PDF_InitializeStruct0, CPS2VM::m_pRAM + m_nPadDataAddress, 0);
	ExecutePadDataFunction(&CPadMan::PDF_InitializeStruct1, CPS2VM::m_pRAM + m_nPadDataAddress, 1);

	//Returns 0 on error
	((uint32*)pRet)[3] = 0x00000001;
}

void CPadMan::SetActuatorAlign(void* pArgs, uint32 nArgsSize, void* pRet, uint32 nRetSize)
{
	assert(nRetSize >= 24);

	((uint32*)pRet)[5] = 1;
}

void CPadMan::Init(void* pArgs, uint32 nArgsSize, void* pRet, uint32 nRetSize)
{
	assert(nRetSize >= 0x10);

	m_nPadDataType = 1;

	Log("Init();\r\n");

	((uint32*)pRet)[3] = 1;
}

void CPadMan::GetModuleVersion(void* pArgs, uint32 nArgsSize, void* pRet, uint32 nRetSize)
{
	assert(nRetSize >= 0x10);

	Log("GetModuleVersion();\r\n");

	((uint32*)pRet)[3] = 0x00000400;
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

//	m_pPad[0].nFrame			= 0;
//	m_pPad[0].nState			= 6;
//	m_pPad[0].nReqState			= 0;
//	m_pPad[0].nLength			= 32;
//	m_pPad[0].nOk				= 1;
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
/*
	m_pPad[1].nFrame			= 1;
	m_pPad[1].nState			= 6;
	m_pPad[1].nReqState			= 0;
	m_pPad[1].nLength			= 32;
	m_pPad[1].nOk				= 1;
#ifdef USE_EX
	m_pPad[1].nModeCurId		= MODE << 4;
	m_pPad[1].nModeCurOffset	= 0;
	m_pPad[1].nModeTable[0]		= MODE;
#endif
*/
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

/*
	uint16 nStatus;
	if(m_pPad == NULL) return;
	nStatus = (m_pPad[PADNUM].nData[2] << 8) | (m_pPad[PADNUM].nData[3]);

	nStatus &= ~nButton;
	if(!nPressed)
	{
		nStatus |= nButton;
	}

	m_pPad[PADNUM].nReqState = 0;

	m_pPad[PADNUM].nData[2] = (uint8)(nStatus >> 8);
	m_pPad[PADNUM].nData[3] = (uint8)(nStatus >> 0);

	m_pPad[PADNUM].nData[0] = 0;
	m_pPad[PADNUM].nData[1] = MODE << 4;
*/
}

void CPadMan::Log(const char* sFormat, ...)
{
#ifdef _DEBUG

	if(!CPS2VM::m_Logging.GetIOPLoggingStatus()) return;

	va_list Args;
	printf("IOP_PadMan: ");
	va_start(Args, sFormat);
	vprintf(sFormat, Args);
	va_end(Args);

#endif
}
