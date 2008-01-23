#include <assert.h>
#include "IOP_DbcMan.h"
#include "../Log.h"

#define LOG_NAME ("iop_dbcman")

using namespace Iop;
using namespace std;
using namespace Framework;

CDbcMan::CDbcMan(CSIF& sif) :
m_nextSocketId(1)
{
    sif.RegisterModule(MODULE_ID, this);
}

CDbcMan::~CDbcMan()
{

}

string CDbcMan::GetId() const
{
    return "dbcman";
}

void CDbcMan::Invoke(CMIPS& context, unsigned int functionId)
{
    throw runtime_error("Not implemented.");
}

void CDbcMan::Invoke(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	switch(method)
	{
	case 0x80000901:
		CreateSocket(args, argsSize, ret, retSize, ram);
		break;
	case 0x80000904:
		SetWorkAddr(args, argsSize, ret, retSize, ram);
		break;
	case 0x8000091A:
		ReceiveData(args, argsSize, ret, retSize, ram);
		break;
	case 0x80000963:
		GetVersionInformation(args, argsSize, ret, retSize, ram);
		break;
	default:
		CLog::GetInstance().Print(LOG_NAME, "Unknown method invoked (0x%0.8X).\r\n", method);
		break;
	}
}
/*
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
*/
void CDbcMan::SetButtonState(unsigned int nPadNumber, CPadListener::BUTTON nButton, bool nPressed)
{
    for(SocketMap::const_iterator socketIterator(m_sockets.begin());
        socketIterator != m_sockets.end(); socketIterator++)
	{
		const SOCKET& socket = socketIterator->second;
		if(socket.nPort != nPadNumber) continue;

        uint16 nStatus = (socket.buf1[0x1C] << 8) | (socket.buf1[0x1D]);
		nStatus &= (~nButton);
		if(!nPressed)
		{
			nStatus |= nButton;
		}

		socket.buf1[0x1C] = static_cast<uint8>(nStatus >> 8);
		socket.buf1[0x1D] = static_cast<uint8>(nStatus >> 0);

//		*(uint16*)&CPS2VM::m_pRAM[pSocket->nBuf1 + 0x1C] ^= 0x2010;
	}
}

void CDbcMan::CreateSocket(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	uint32 nType1, nType2, nPort, nSlot, nBuf1, nBuf2;

	assert(argsSize >= 0x30);
	assert(retSize >= 0x04);

	nType1	= args[0x00];
	nType2	= args[0x01];
	nPort	= args[0x02];
	nSlot	= args[0x03];
	nBuf1	= args[0x0A];
	nBuf2	= args[0x0B];

    SOCKET socket;
	socket.nPort    = nPort;
	socket.nSlot    = nSlot;
	socket.buf1     = &ram[nBuf1];
	socket.buf2     = &ram[nBuf2];

    uint32 id = m_nextSocketId++;
    m_sockets[id] = socket;

	ret[0x09] = id;

	CLog::GetInstance().Print(LOG_NAME, "CreateSocket(type1 = 0x%0.8X, type2 = 0x%0.8X, port = %i, slot = %i, buf1 = 0x%0.8X, buf2 = 0x%0.8X);\r\n", \
		nType1, \
		nType2, \
		nPort, \
		nSlot, \
		nBuf1, \
		nBuf2);
}

void CDbcMan::SetWorkAddr(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	assert(argsSize == 0x400);
	assert(retSize == 0x400);

	//0 - Some number (0x0200) (size?)
	//1 - Address to bind with

	uint32 nAddress = args[1];

	//Set Ready (?) status
	ram[nAddress] = 1;

	ret[0] = 0x00000000;

	CLog::GetInstance().Print(LOG_NAME, "SetWorkAddr(addr = 0x%0.8X);\r\n", nAddress);
}

void CDbcMan::ReceiveData(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	uint32 nParam, nFlags, nSocket;

	//Param Frame
	//0 - Socket ID
	//1 - Value passed in parameter to the library
	//2 - Some parameter (0x01, or some address)

	nSocket = args[0];
	nFlags	= args[1];
	nParam	= args[2];

    SocketMap::iterator socketIterator = m_sockets.find(nSocket);
	if(socketIterator != m_sockets.end())
	{
        SOCKET& socket(socketIterator->second);
		socket.buf1[0x02] = 0x20;
		*reinterpret_cast<uint32*>(&socket.buf1[0x04]) = 0x01;
	}

	//Return frame
	//0  - Success Status
	//1  - ???
	//2  - Size of returned data
	//3+ - Data

	ret[0] = 0;
	ret[2] = 0x1;
	ret[3] = 0x1;

	CLog::GetInstance().Print(LOG_NAME, "ReceiveData(socket = 0x%0.8X, flags = 0x%0.8X, param = 0x%0.8X);\r\n", nSocket, nFlags, nParam);
}

void CDbcMan::GetVersionInformation(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	assert(argsSize == 0x400);
	assert(retSize == 0x400);

	ret[0] = 0x00000200;

	CLog::GetInstance().Print(LOG_NAME, "GetVersionInformation();\r\n");
}
