#include <assert.h>
#include <stdio.h>
#include "IOP_DbcMan.h"
#include "PS2VM.h"

using namespace IOP;
using namespace Framework;

CDbcMan::CDbcMan()
{

}

CDbcMan::~CDbcMan()
{
	DeleteAllSockets();
}

void CDbcMan::Invoke(uint32 nMethod, void* pArgs, uint32 nArgsSize, void* pRet, uint32 nRetSize)
{
	switch(nMethod)
	{
	case 0x80000901:
		CreateSocket(pArgs, nArgsSize, pRet, nRetSize);
		break;
	case 0x80000904:
		SetWorkAddr(pArgs, nArgsSize, pRet, nRetSize);
		break;
	case 0x8000091A:
		ReceiveData(pArgs, nArgsSize, pRet, nRetSize);
		break;
	case 0x80000963:
		GetVersionInformation(pArgs, nArgsSize, pRet, nRetSize);
		break;
	}
}

void CDbcMan::SaveState(CStream* pStream)
{
	uint32 nCount, nID;
	SOCKET* pSocket;
	CList<SOCKET>::ITERATOR itSocket;

	nCount = m_Socket.Count();
	pStream->Write(&nCount, 4);

	for(itSocket = m_Socket.Begin(); itSocket.HasNext(); itSocket++)
	{
		nID = itSocket.GetKey();
		pSocket = (*itSocket);

		pStream->Write(&nID,	4);
		pStream->Write(pSocket, sizeof(SOCKET));
	}
}

void CDbcMan::LoadState(CStream* pStream)
{
	uint32 nCount, nID, i;
	SOCKET* pSocket;

	DeleteAllSockets();
	
	pStream->Read(&nCount, 4);
	
	for(i = 0; i < nCount; i++)
	{
		pSocket = new SOCKET;

		pStream->Read(&nID,		4);
		pStream->Read(pSocket,	sizeof(SOCKET));

		m_Socket.Insert(pSocket, nID);
	}
}

void CDbcMan::SetButtonState(unsigned int nPadNumber, CPadListener::BUTTON nButton, bool nPressed)
{
	SOCKET* pSocket;
	CList<SOCKET>::ITERATOR itSocket;
	uint16 nStatus;

	for(itSocket = m_Socket.Begin(); itSocket.HasNext(); itSocket++)
	{
		pSocket = (*itSocket);
		if(pSocket->nPort != nPadNumber) continue;

		nStatus	= (CPS2VM::m_pRAM[pSocket->nBuf1 + 0x1C] << 8) | (CPS2VM::m_pRAM[pSocket->nBuf1 + 0x1D]);

		nStatus &= (~nButton);
		if(!nPressed)
		{
			nStatus |= nButton;
		}

		CPS2VM::m_pRAM[pSocket->nBuf1 + 0x1C] = (uint8)(nStatus >> 8);
		CPS2VM::m_pRAM[pSocket->nBuf1 + 0x1D] = (uint8)(nStatus >> 0);

//		*(uint16*)&CPS2VM::m_pRAM[pSocket->nBuf1 + 0x1C] ^= 0x2010;
	}
}

void CDbcMan::CreateSocket(void* pArgs, uint32 nArgsSize, void* pRet, uint32 nRetSize)
{
	uint32 nType1, nType2, nPort, nSlot, nBuf1, nBuf2, nID;
	SOCKET* pSocket;

	assert(nArgsSize >= 0x30);
	assert(nRetSize >= 0x04);

	nType1	= ((uint32*)pArgs)[0x00];
	nType2	= ((uint32*)pArgs)[0x01];
	nPort	= ((uint32*)pArgs)[0x02];
	nSlot	= ((uint32*)pArgs)[0x03];
	nBuf1	= ((uint32*)pArgs)[0x0A];
	nBuf2	= ((uint32*)pArgs)[0x0B];

	pSocket = new SOCKET;
	pSocket->nPort	= nPort;
	pSocket->nSlot	= nSlot;
	pSocket->nBuf1	= nBuf1;
	pSocket->nBuf2	= nBuf2;

	nID = m_Socket.MakeKey();
	m_Socket.Insert(pSocket, nID);

	((uint32*)pRet)[0x09] = nID;

	printf("IOP_DbcMan: CreateSocket(type1 = 0x%0.8X, type2 = 0x%0.8X, port = %i, slot = %i, buf1 = 0x%0.8X, buf2 = 0x%0.8X);\r\n", \
		nType1, \
		nType2, \
		nPort, \
		nSlot, \
		nBuf1, \
		nBuf2);
}

void CDbcMan::SetWorkAddr(void* pArgs, uint32 nArgsSize, void* pRet, uint32 nRetSize)
{
	uint32 nAddress;

	assert(nArgsSize == 0x400);
	assert(nRetSize == 0x400);

	//0 - Some number (0x0200) (size?)
	//1 - Address to bind with

	nAddress = ((uint32*)pArgs)[1];

	//Set Ready (?) status
	CPS2VM::m_pRAM[nAddress] = 1;

	((uint32*)pRet)[0] = 0x00000000;

	printf("IOP_DbcMan: SetWorkAddr(addr = 0x%0.8X);\r\n", nAddress);
}

void CDbcMan::ReceiveData(void* pArgs, uint32 nArgsSize, void* pRet, uint32 nRetSize)
{
	uint32 nParam, nFlags, nSocket;
	SOCKET* pSocket;

	//Param Frame
	//0 - Socket ID
	//1 - Value passed in parameter to the library
	//2 - Some parameter (0x01, or some address)

	nSocket = ((uint32*)pArgs)[0];
	nFlags	= ((uint32*)pArgs)[1];
	nParam	= ((uint32*)pArgs)[2];

	pSocket = m_Socket.Find(nSocket);
	if(pSocket != NULL)
	{
		CPS2VM::m_pRAM[pSocket->nBuf1 + 0x02] = 0x20;
		*(uint32*)&CPS2VM::m_pRAM[pSocket->nBuf1 + 0x04] = 0x01;
	}

	//Return frame
	//0  - Success Status
	//1  - ???
	//2  - Size of returned data
	//3+ - Data

	((uint32*)pRet)[0] = 0;
	((uint32*)pRet)[2] = 0x1;
	((uint32*)pRet)[3] = 0x1;

	printf("IOP_DbcMan: ReceiveData(socket = 0x%0.8X, flags = 0x%0.8X, param = 0x%0.8X);\r\n", nSocket, nFlags, nParam);
}

void CDbcMan::GetVersionInformation(void* pArgs, uint32 nArgsSize, void* pRet, uint32 nRetSize)
{
	assert(nArgsSize == 0x400);
	assert(nRetSize == 0x400);

	((uint32*)pRet)[0] = 0x00000200;

	printf("IOP_DbcMan: GetVersionInformation();\r\n");
}

void CDbcMan::DeleteAllSockets()
{
	while(m_Socket.Count() != 0)
	{
		delete m_Socket.Pull();
	}
}
