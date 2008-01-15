#include <stdio.h>
#include "Ps2Const.h"
#include "SIF.h"
#include "PtrMacro.h"
#include "Profiler.h"
#include "Log.h"

#define		CMD_RECVADDR		0x00001000
#define		RPC_RECVADDR		0xDEADBEEF

#define LOG_NAME "sif"

#ifdef	PROFILE
#define	PROFILE_SIFZONE "SIF"
#endif

using namespace Framework;
using namespace std;

CSIF::CSIF(CDMAC& dmac, uint8* eeRam) :
m_nMAINADDR(0),
m_nSUBADDR(0),
m_nMSFLAG(0),
m_nSMFLAG(0),
m_nEERecvAddr(0),
m_nDataAddr(0),
m_dmac(dmac),
m_eeRam(eeRam),
m_dmaBuffer(NULL),
m_dmaBufferSize(0)
{

}

CSIF::~CSIF()
{

}

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
//	m_pPadMan = new IOP::CPadMan();

//	m_Module.Insert(new IOP::CDummy,								IOP::CDummy::MODULE_ID);
//	m_Module.Insert(new IOP::CUnknown,								IOP::CUnknown::MODULE_ID);
//	m_Module.Insert(new IOP::CFileIO,								IOP::CFileIO::MODULE_ID);
//	m_Module.Insert(new IOP::CSysMem,								IOP::CSysMem::MODULE_ID);
//	m_Module.Insert(new IOP::CLoadFile,								IOP::CLoadFile::MODULE_ID);
//	m_Module.Insert(m_pPadMan,										IOP::CPadMan::MODULE_ID_1);
//	m_Module.Insert(m_pPadMan,										IOP::CPadMan::MODULE_ID_3);
//	m_Module.Insert(new IOP::CMcServ,								IOP::CMcServ::MODULE_ID);
//	m_Module.Insert(new IOP::CCdvdfsv(IOP::CCdvdfsv::MODULE_ID_1),	IOP::CCdvdfsv::MODULE_ID_1);
//	m_Module.Insert(new IOP::CCdvdfsv(IOP::CCdvdfsv::MODULE_ID_2),	IOP::CCdvdfsv::MODULE_ID_2);
//	m_Module.Insert(new IOP::CCdvdfsv(IOP::CCdvdfsv::MODULE_ID_4),	IOP::CCdvdfsv::MODULE_ID_4);
//	m_Module.Insert(new IOP::CCdvdfsv(IOP::CCdvdfsv::MODULE_ID_6),	IOP::CCdvdfsv::MODULE_ID_6);
//	m_Module.Insert(new IOP::CCdvdfsv(IOP::CCdvdfsv::MODULE_ID_7),	IOP::CCdvdfsv::MODULE_ID_7);
//	m_Module.Insert(new IOP::CLibSD,								IOP::CLibSD::MODULE_ID);
//	m_Module.Insert(new IOP::CDbcMan,								IOP::CDbcMan::MODULE_ID);
}

void CSIF::SetDmaBuffer(uint8* buffer, uint32 size)
{
    m_dmaBuffer = buffer;
    m_dmaBufferSize = size;
}

void CSIF::RegisterModule(uint32 moduleId, CSifModule* module)
{
    m_modules[moduleId] = module;
}

void CSIF::DeleteModules()
{
    m_modules.clear();
}

uint32 CSIF::ReceiveDMA5(uint32 srcAddress, uint32 size, uint32 unused, bool isTagIncluded)
{
    if(size > m_dmaBufferSize)
    {
        throw runtime_error("Packet too big.");
    }
    memcpy(m_eeRam + srcAddress, m_dmaBuffer, size);
    return size;
}

uint32 CSIF::ReceiveDMA6(uint32 nSrcAddr, uint32 nSize, uint32 nDstAddr, bool isTagIncluded)
{

#ifdef PROFILE
	CProfiler::GetInstance().BeginZone(PROFILE_SIFZONE);
#endif

	PACKETHDR* pHDR;

	//Humm, this is kinda odd, but it ors the address with 0x20000000
	nSrcAddr &= (PS2::EERAMSIZE - 1);

	if(nDstAddr == RPC_RECVADDR)
	{
		//This should be the arguments for the call command
		//Just save the source address for later use
		m_nDataAddr = nSrcAddr;

#ifdef PROFILE
	CProfiler::GetInstance().EndZone();
#endif
		return nSize;
	}

	pHDR = (PACKETHDR*)(m_eeRam + nSrcAddr);

	//This is kinda odd...
	//plasma_tunnel.elf does this
/*
	if((pHDR->nCID & 0xFF000000) == 0x08000000)
	{
		pHDR->nCID &= 0x00FFFFFF;
		pHDR->nCID |= 0x80000000;
	}
*/
    CLog::GetInstance().Print(LOG_NAME, "Received command 0x%0.8X.\r\n", pHDR->nCID);

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

#ifdef PROFILE
	CProfiler::GetInstance().EndZone();
#endif

	return nSize;
}

void CSIF::SendDMA(void* pData, uint32 nSize)
{
	//Humm, the DMAC doesn't know about our addresses on this side...
	uint32 nQuads;

    if(nSize > m_dmaBufferSize)
    {
        throw runtime_error("Packet too big.");
    }

	memcpy(m_dmaBuffer, pData, nSize);
	nQuads = (nSize + 0x0F) / 0x10;

	m_dmac.SetRegister(CDMAC::D5_MADR, m_nEERecvAddr);
	m_dmac.SetRegister(CDMAC::D5_QWC,  nQuads);
	m_dmac.SetRegister(CDMAC::D5_CHCR, 0x00000100);
}

void CSIF::LoadState(CStream* pStream)
{
/*
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
*/
}

void CSIF::SaveState(CStream* pStream)
{
/*
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
*/
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
		m_nEERecvAddr &= (PS2::EERAMSIZE - 1);
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

	CLog::GetInstance().Print(LOG_NAME, "Bound client data (0x%0.8X) with server id 0x%0.8X.\r\n", pBind->nClientDataAddr, pBind->nSID);

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

	uint32 nRecvAddr;

	pCall = (RPCCALL*)pHDR;

	CLog::GetInstance().Print(LOG_NAME, "Calling function 0x%0.8X of module 0x%0.8X.\r\n", pCall->nRPCNumber, pCall->nServerDataAddr);

	nRecvAddr = (pCall->nRecv & (PS2::EERAMSIZE - 1));

    ModuleMap::iterator moduleIterator(m_modules.find(pCall->nServerDataAddr));
	if(moduleIterator != m_modules.end())
	{
	    CSifModule* pModule(moduleIterator->second);
        pModule->Invoke(pCall->nRPCNumber, 
            reinterpret_cast<uint32*>(m_eeRam + m_nDataAddr), pCall->nSendSize, 
            reinterpret_cast<uint32*>(m_eeRam + nRecvAddr), pCall->nRecvSize,
            m_eeRam);
	}
	else
	{
		CLog::GetInstance().Print(LOG_NAME, "Called an unknown module (0x%0.8X).\r\n", pCall->nServerDataAddr);
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
			CLog::GetInstance().Print(LOG_NAME, "Warning. Trying to read an invalid system register (0x%0.8X).\r\n", nRegister);
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
			CLog::GetInstance().Print(LOG_NAME, "Warning. Trying to write to an invalid system register (0x%0.8X).\r\n", nRegister);
			break;
		}
	}
}
