#include <stdio.h>
#include "Ps2Const.h"
#include "SIF.h"
#include "PtrMacro.h"
#include "Profiler.h"
#include "Log.h"
#include "RegisterStateFile.h"
#include "iop/IopBios.h"

#define		CMD_RECVADDR		(CIopBios::CONTROL_BLOCK_END)
#define		RPC_RECVADDR		0xDEADBEEF

#define LOG_NAME        ("sif")

#define STATE_REGS_XML  ("sif/regs.xml")

#ifdef	PROFILE
#define	PROFILE_SIFZONE "SIF"
#endif

using namespace Framework;
using namespace std;

CSIF::CSIF(CDMAC& dmac, uint8* eeRam, uint8* iopRam) :
m_nMAINADDR(0),
m_nSUBADDR(0),
m_nMSFLAG(0),
m_nSMFLAG(0),
m_nEERecvAddr(0),
m_nDataAddr(0),
m_dmac(dmac),
m_eeRam(eeRam),
m_iopRam(iopRam),
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

    m_packetQueue.clear();
    m_callReplies.clear();
    m_bindReplies.clear();

	DeleteModules();
}

void CSIF::SetDmaBuffer(uint32 bufferAddress, uint32 size)
{
    m_dmaBuffer = m_iopRam + bufferAddress;
    m_dmaBufferSize = size;
}

void CSIF::RegisterModule(uint32 moduleId, CSifModule* module)
{
    m_modules[moduleId] = module;

    BindReplyMap::iterator replyIterator(m_bindReplies.find(moduleId));
    if(replyIterator != m_bindReplies.end())
    {
        SendPacket(&(replyIterator->second), sizeof(RPCREQUESTEND));
        m_bindReplies.erase(replyIterator);
    }
}

void CSIF::UnregisterModule(uint32 moduleId)
{
    m_modules.erase(moduleId);
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

    assert(!isTagIncluded);

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
    else if(nDstAddr == CMD_RECVADDR)
    {
	    PACKETHDR* pHDR = (PACKETHDR*)(m_eeRam + nSrcAddr);

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
    else
    {
        assert(nDstAddr < PS2::IOP_RAM_SIZE);
        if(nDstAddr >= 0 && nDstAddr <= CMD_RECVADDR)
        {
            CLog::GetInstance().Print(LOG_NAME, "Warning: Trying to DMA in Bios Control Area.\r\n");
        }
        else
        {
            memcpy(m_iopRam + nDstAddr, m_eeRam + nSrcAddr, nSize);
        }
        return nSize;
    }
}

void CSIF::SendPacket(void* packet, uint32 size)
{
    m_packetQueue.insert(m_packetQueue.begin(), 
        reinterpret_cast<uint8*>(packet), 
        reinterpret_cast<uint8*>(packet) + size);
    m_packetQueue.insert(m_packetQueue.begin(),
        reinterpret_cast<uint8*>(&size),
        reinterpret_cast<uint8*>(&size) + 4);
}

void CSIF::ProcessPackets()
{
    if(m_packetQueue.size() != 0)
    {
        assert(m_packetQueue.size() > 4);
        uint32 size = *reinterpret_cast<uint32*>(&m_packetQueue[0]);
        SendDMA(&m_packetQueue[4], size);
        m_packetQueue.erase(m_packetQueue.begin(), m_packetQueue.begin() + 4 + size);
    }
}

void CSIF::SendDMA(void* pData, uint32 nSize)
{
	//Humm, the DMAC doesn't know about our addresses on this side...

    if(nSize > m_dmaBufferSize)
    {
        throw runtime_error("Packet too big.");
    }

	memcpy(m_dmaBuffer, pData, nSize);
	uint32 nQuads = (nSize + 0x0F) / 0x10;

	m_dmac.SetRegister(CDMAC::D5_MADR, m_nEERecvAddr);
	m_dmac.SetRegister(CDMAC::D5_QWC,  nQuads);
	m_dmac.SetRegister(CDMAC::D5_CHCR, 0x00000100);
}

void CSIF::LoadState(CZipArchiveReader& archive)
{
    CRegisterStateFile registerFile(*archive.BeginReadFile(STATE_REGS_XML));
    m_nMAINADDR     = registerFile.GetRegister32("MAINADDR");
    m_nSUBADDR      = registerFile.GetRegister32("SUBADDR");
    m_nMSFLAG       = registerFile.GetRegister32("MSFLAG");
    m_nSMFLAG       = registerFile.GetRegister32("SMFLAG");
    m_nEERecvAddr   = registerFile.GetRegister32("EERecvAddr");
    m_nDataAddr     = registerFile.GetRegister32("DataAddr");
}

void CSIF::SaveState(CZipArchiveWriter& archive)
{
    CRegisterStateFile* registerFile = new CRegisterStateFile(STATE_REGS_XML);
    registerFile->SetRegister32("MAINADDR",     m_nMAINADDR);
    registerFile->SetRegister32("SUBADDR",      m_nSUBADDR);
    registerFile->SetRegister32("MSFLAG",       m_nMSFLAG);
    registerFile->SetRegister32("SMFLAG",       m_nSMFLAG);
    registerFile->SetRegister32("EERecvAddr",   m_nEERecvAddr);
    registerFile->SetRegister32("DataAddr",     m_nDataAddr);
    archive.InsertFile(registerFile);
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

	INIT* pInit = reinterpret_cast<INIT*>(pHDR);

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

        SendPacket(&SReg, sizeof(SETSREG));
		//SendDMA(&SReg, sizeof(SETSREG));
	}
	else
	{
		assert(0);
	}
}

void CSIF::Cmd_Bind(PACKETHDR* pHDR)
{
	RPCBIND* pBind = reinterpret_cast<RPCBIND*>(pHDR);

	//Maybe check what it wants to bind?

	RPCREQUESTEND rend;

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

	CLog::GetInstance().Print(LOG_NAME, "Bound client data (0x%0.8X) with server id 0x%0.8X.\r\n", pBind->nClientDataAddr, pBind->nSID);

    ModuleMap::iterator moduleIterator(m_modules.find(pBind->nSID));
    if(moduleIterator != m_modules.end() || (pBind->nSID & 0x80000000) != 0)
    {
        SendPacket(&rend, sizeof(RPCREQUESTEND));
    }
    else
    {
        assert(m_bindReplies.find(pBind->nSID) == m_bindReplies.end());
        m_bindReplies[pBind->nSID] = rend;
    }
}

void CSIF::Cmd_Call(PACKETHDR* pHDR)
{
	RPCCALL* pCall = reinterpret_cast<RPCCALL*>(pHDR);
    bool sendReply = true;

	CLog::GetInstance().Print(LOG_NAME, "Calling function 0x%0.8X of module 0x%0.8X.\r\n", pCall->nRPCNumber, pCall->nServerDataAddr);

	uint32 nRecvAddr = (pCall->nRecv & (PS2::EERAMSIZE - 1));

    ModuleMap::iterator moduleIterator(m_modules.find(pCall->nServerDataAddr));
	if(moduleIterator != m_modules.end())
	{
	    CSifModule* pModule(moduleIterator->second);
        sendReply = pModule->Invoke(pCall->nRPCNumber, 
            reinterpret_cast<uint32*>(m_eeRam + m_nDataAddr), pCall->nSendSize, 
            reinterpret_cast<uint32*>(m_eeRam + nRecvAddr), pCall->nRecvSize,
            m_eeRam);
	}
	else
	{
		CLog::GetInstance().Print(LOG_NAME, "Called an unknown module (0x%0.8X).\r\n", pCall->nServerDataAddr);
	}

    {
        RPCREQUESTEND rend;
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

        if(sendReply)
        {
            SendPacket(&rend, sizeof(RPCREQUESTEND));
        }
        else
        {
            //Hold the packet
            //We assume that there's only one call that
            assert(m_callReplies.find(pCall->nServerDataAddr) == m_callReplies.end());
            CALLREQUESTINFO requestInfo;
            requestInfo.reply = rend;
            requestInfo.call = *pCall;
            m_callReplies[pCall->nServerDataAddr] = requestInfo;
        }
    }
}

void CSIF::SendCallReply(uint32 serverId, void* returnData)
{
    CallReplyMap::iterator replyIterator(m_callReplies.find(serverId));
    assert(replyIterator != m_callReplies.end());
    if(replyIterator == m_callReplies.end()) return;

    CALLREQUESTINFO& requestInfo(replyIterator->second);
    assert(requestInfo.call.nRecv != 0);
    if(requestInfo.call.nRecv != 0)
    {
        memcpy(m_eeRam + requestInfo.call.nRecv, returnData, requestInfo.call.nRecvSize);
    }
    SendPacket(&requestInfo.reply, sizeof(RPCREQUESTEND));
    m_callReplies.erase(replyIterator);
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
