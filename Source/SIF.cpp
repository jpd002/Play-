#include <stdio.h>
#include "PS2VM.h"
#include "SIF.h"
#include "DMAC.h"
#include "PtrMacro.h"
#include "IOP_LoadFile.h"
#include "IOP_FileIO.h"
#include "IOP_SysMem.h"
#include "IOP_McServ.h"
#include "IOP_DbcMan.h"
#include "IOP_LibSD.h"
#include "IOP_Cdvdfsv.h"

#define		CMD_RECVADDR		0x00001000
#define		RPC_RECVADDR		0xDEADBEEF

using namespace Framework;

uint32					CSIF::m_nMAINADDR	= 0;
uint32					CSIF::m_nSUBADDR	= 0;
uint32					CSIF::m_nMSFLAG		= 0;
uint32					CSIF::m_nSMFLAG		= 0;
uint32					CSIF::m_nUserReg[MAX_USERREG];

uint8					CSIF::m_pRAM[0x1000];

uint32					CSIF::m_nEERecvAddr = 0;
uint32					CSIF::m_nDataAddr = 0;

IOP::CPadMan*			CSIF::m_pPadMan = NULL;

CList<IOP::CModule>		CSIF::m_Module;

void CSIF::Reset()
{
	m_nMAINADDR		= 0;
	//This should be the address to which the IOP receives its requests from the EE
	m_nSUBADDR		= CMD_RECVADDR;
	m_nMSFLAG		= 0;
//	m_nSMFLAG		= 0x20000;
	m_nSMFLAG		= 0x60000;

	memset(m_nUserReg, 0, sizeof(uint32) * MAX_USERREG);

	DeleteModules();

	//Create modules that have multiple RPC IDs
	m_pPadMan = new IOP::CPadMan();

	m_Module.Insert(new IOP::CFileIO,								IOP::CFileIO::MODULE_ID);
	m_Module.Insert(new IOP::CSysMem,								IOP::CSysMem::MODULE_ID);
	m_Module.Insert(new IOP::CLoadFile,								IOP::CLoadFile::MODULE_ID);
	m_Module.Insert(m_pPadMan,										IOP::CPadMan::MODULE_ID_1);
	m_Module.Insert(m_pPadMan,										IOP::CPadMan::MODULE_ID_3);
	m_Module.Insert(new IOP::CMcServ,								IOP::CMcServ::MODULE_ID);
	m_Module.Insert(new IOP::CCdvdfsv(IOP::CCdvdfsv::MODULE_ID_1),	IOP::CCdvdfsv::MODULE_ID_1);
	m_Module.Insert(new IOP::CCdvdfsv(IOP::CCdvdfsv::MODULE_ID_4),	IOP::CCdvdfsv::MODULE_ID_4);
	m_Module.Insert(new IOP::CCdvdfsv(IOP::CCdvdfsv::MODULE_ID_6),	IOP::CCdvdfsv::MODULE_ID_6);
	m_Module.Insert(new IOP::CLibSD,								IOP::CLibSD::MODULE_ID);
	m_Module.Insert(new IOP::CDbcMan,								IOP::CDbcMan::MODULE_ID);
}

void CSIF::DeleteModules()
{
	m_Module.Remove(IOP::CPadMan::MODULE_ID_1);
	m_Module.Remove(IOP::CPadMan::MODULE_ID_3);

	DELETEPTR(m_pPadMan);

	while(m_Module.Count() != 0)
	{
		delete m_Module.Pull();
	}
}

uint32 CSIF::ReceiveDMA(uint32 nSrcAddr, uint32 nDstAddr, uint32 nSize)
{
	PACKETHDR* pHDR;

	//Humm, this is kinda odd, but it ors the address with 0x20000000
	nSrcAddr &= (CPS2VM::RAMSIZE - 1);

	if(nDstAddr == RPC_RECVADDR)
	{
		//This should be the arguments for the call command
		//Just save the source address for later use
		m_nDataAddr = nSrcAddr;
		return nSize;
	}

	pHDR = (PACKETHDR*)(CPS2VM::m_pRAM + nSrcAddr);

	//This is kinda odd...
	//plasma_tunnel.elf does this
/*
	if((pHDR->nCID & 0xFF000000) == 0x08000000)
	{
		pHDR->nCID &= 0x00FFFFFF;
		pHDR->nCID |= 0x80000000;
	}
*/
	printf("SIF: Received command 0x%0.8X.\r\n", pHDR->nCID);

	switch(pHDR->nCID)
	{
	case 0x80000000:
		Cmd_SetEERecvAddr(pHDR);
		break;
	case SIF_CMD_INIT:
		Cmd_Initialize(pHDR);
		break;
	case SIF_CMD_BIND:
		Cmd_Bind(pHDR);
		break;
	case SIF_CMD_CALL:
		Cmd_Call(pHDR);
		break;
	}

	return nSize;
}

void CSIF::SendDMA(void* pData, uint32 nSize)
{
	//Humm, the DMAC doesn't know about our addresses on this side...
	uint32 nQuads;

	memcpy(m_pRAM, pData, nSize);

	nQuads = nSize / 0x10;
	if((nSize & 0x0F) != 0) nQuads++;

	CDMAC::SetRegister(CDMAC::D5_MADR, m_nEERecvAddr);
	CDMAC::SetRegister(CDMAC::D5_QWC,  nQuads);
	CDMAC::SetRegister(CDMAC::D5_CHCR, 0x00000100);
}

void CSIF::LoadState(CStream* pStream)
{
	pStream->Read(&m_nMAINADDR,		4);
	pStream->Read(&m_nSUBADDR,		4);
	pStream->Read(&m_nMSFLAG,		4);
	pStream->Read(&m_nSMFLAG,		4);
	
	pStream->Read(&m_nEERecvAddr,	4);
	pStream->Read(&m_nDataAddr,		4);

	//Load the state of individual modules
	m_Module.Find(IOP::CCdvdfsv::MODULE_ID_1)->LoadState(pStream);
	m_Module.Find(IOP::CDbcMan::MODULE_ID)->LoadState(pStream);
	m_pPadMan->LoadState(pStream);
}

void CSIF::SaveState(CStream* pStream)
{
	pStream->Write(&m_nMAINADDR,	4);
	pStream->Write(&m_nSUBADDR,		4);
	pStream->Write(&m_nMSFLAG,		4);
	pStream->Write(&m_nSMFLAG,		4);
	
	pStream->Write(&m_nEERecvAddr,	4);
	pStream->Write(&m_nDataAddr,	4);

	//Save the state of individual modules
	m_Module.Find(IOP::CCdvdfsv::MODULE_ID_1)->SaveState(pStream);
	m_Module.Find(IOP::CDbcMan::MODULE_ID)->SaveState(pStream);
	m_pPadMan->SaveState(pStream);
}

IOP::CPadMan* CSIF::GetPadMan()
{
	return m_pPadMan;
}

IOP::CDbcMan* CSIF::GetDbcMan()
{
	return (IOP::CDbcMan*)m_Module.Find(IOP::CDbcMan::MODULE_ID);
}

IOP::CFileIO* CSIF::GetFileIO()
{
	return (IOP::CFileIO*)m_Module.Find(IOP::CFileIO::MODULE_ID);
}

/////////////////////////////////////////////////////////
//SIF Commands
/////////////////////////////////////////////////////////

void CSIF::Cmd_SetEERecvAddr(PACKETHDR* pHDR)
{
	assert(0);
}

void CSIF::Cmd_Initialize(PACKETHDR* pHDR)
{
	struct INIT
	{
		PACKETHDR	Header;
		uint32		nEEAddress;
	};

	INIT* pInit;

	pInit = (INIT*)pHDR;

	if(pInit->Header.nOptional == 0)
	{
		m_nEERecvAddr =  pInit->nEEAddress;
		m_nEERecvAddr &= (CPS2VM::RAMSIZE - 1);
	}
	else if(pInit->Header.nOptional == 1)
	{
		//If this is set to 1, and we need to disregard the address received and send a command back...
		//Not sure about this though
		SETSREG SReg;
		SReg.Header.nCID		= 0x80000001;
		SReg.Header.nSize		= sizeof(SETSREG);
		SReg.Header.nDest		= NULL;
		SReg.Header.nOptional	= 0;
		SReg.nRegister			= 0;
		SReg.nValue				= 1;

		SendDMA(&SReg, sizeof(SETSREG));
	}
	else
	{
		assert(0);
	}
}

void CSIF::Cmd_Bind(PACKETHDR* pHDR)
{
	RPCBIND* pBind;
	RPCREQUESTEND rend;

	pBind = (RPCBIND*)pHDR;

	//Maybe check what it wants to bind?

	printf("SIF: Bound client data (0x%0.8X) with server id 0x%0.8X.\r\n", pBind->nClientDataAddr, pBind->nSID);

	//Fill in the request end 
	rend.Header.nSize		= sizeof(RPCREQUESTEND);
	rend.Header.nDest		= pHDR->nDest;
	rend.Header.nCID		= SIF_CMD_REND;
	rend.Header.nOptional	= 0;

	rend.nRecordID			= pBind->nRecordID;
	rend.nPacketAddr		= pBind->nPacketAddr;
	rend.nRPCID				= pBind->nRPCID;
	rend.nClientDataAddr	= pBind->nClientDataAddr;
	rend.nCID				= SIF_CMD_BIND;
	rend.nServerDataAddr	= pBind->nSID;
	rend.nBuffer			= RPC_RECVADDR;
	rend.nClientBuffer		= 0xDEADCAFE;

	SendDMA(&rend, sizeof(RPCREQUESTEND));
}

void CSIF::Cmd_Call(PACKETHDR* pHDR)
{
	RPCCALL* pCall;
	RPCREQUESTEND rend;

	IOP::CModule* pModule;
	uint32 nRecvAddr;

	pCall = (RPCCALL*)pHDR;

	printf("SIF: Calling function 0x%0.8X of module 0x%0.8X.\r\n", pCall->nRPCNumber, pCall->nServerDataAddr);

	nRecvAddr = (pCall->nRecv & (CPS2VM::RAMSIZE - 1));

	pModule = m_Module.Find(pCall->nServerDataAddr);
	if(pModule != NULL)
	{
		pModule->Invoke(pCall->nRPCNumber, CPS2VM::m_pRAM + m_nDataAddr, pCall->nSendSize, CPS2VM::m_pRAM + nRecvAddr, pCall->nRecvSize);
	}
	else
	{
		printf("SIF: Called an unknown module (0x%0.8X).\r\n", pCall->nServerDataAddr);
	}

	memset(&rend, 0, sizeof(RPCREQUESTEND));

	//Fill in the request end 
	rend.Header.nSize		= sizeof(RPCREQUESTEND);
	rend.Header.nDest		= pHDR->nDest;
	rend.Header.nCID		= SIF_CMD_REND;
	rend.Header.nOptional	= 0;

	rend.nRecordID			= pCall->nRecordID;
	rend.nPacketAddr		= pCall->nPacketAddr;
	rend.nRPCID				= pCall->nRPCID;
	rend.nClientDataAddr	= pCall->nClientDataAddr;
	rend.nCID				= SIF_CMD_CALL;

	SendDMA(&rend, sizeof(RPCREQUESTEND));
}

/////////////////////////////////////////////////////////
//Get/Set Register
/////////////////////////////////////////////////////////

uint32 CSIF::GetRegister(uint32 nRegister)
{
/*
	if(nRegister & 0x80000000)
	{
		nRegister &= ~0x80000000;
		if(nRegister >= MAX_USERREG)
		{
			printf("SIF: Warning. Trying to read an unimplemented user register (0x%0.8X).\r\n", nRegister | 0x80000000);
			return 0;
		}
		return m_nUserReg[nRegister];
	}
	else
*/
	{
		switch(nRegister)
		{
		case 0x00000001:
			return m_nMAINADDR;
			break;
		case 0x00000002:
			return m_nSUBADDR;
			break;
		case 0x00000003:
			return m_nMSFLAG;
			break;
		case 0x00000004:
			return m_nSMFLAG;
			break;
		case 0x80000000:
			return 0;
			break;
		case 0x80000002:
//			return 0;
			return 1;
			break;
		default:
			printf("SIF: Warning. Trying to read an invalid system register (0x%0.8X).\r\n", nRegister);
			return 0;
			break;
		}
	}
}

void CSIF::SetRegister(uint32 nRegister, uint32 nValue)
{
/*
	if(nRegister & 0x80000000)
	{
		nRegister &= ~0x80000000;
		if(nRegister >= MAX_USERREG)
		{
			printf("SIF: Warning. Trying to write to an unimplemented user register (0x%0.8X).\r\n", nRegister | 0x80000000);
			return;
		}
		m_nUserReg[nRegister] = nValue;
	}
	else*/
	{
		switch(nRegister)
		{
		case 0x00000001:
			m_nMAINADDR = nValue;
			break;
		case 0x80000000:
			//Set by CMD library
			break;
		case 0x80000001:
			//Set by CMD library
			break;
		case 0x80000002:
			//Set by RPC library (initialized state?)
			break;
		default:
			printf("SIF: Warning. Trying to write to an invalid system register (0x%0.8X).\r\n", nRegister);
			break;
		}
	}
}
