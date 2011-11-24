#include <assert.h>
#include <boost/lexical_cast.hpp>
#include "IOP_DbcMan.h"
#include "../Log.h"
#include "../RegisterStateFile.h"

#define LOG_NAME ("iop_dbcman")

#define SOCKET_STATE_BASE_PATH ("iop_dbcman/socket_")
#define SOCKET_STATE_EXTENSION (".xml")

using namespace Iop;
using namespace std;
using namespace Framework;
using namespace boost;
using namespace PS2;

CDbcMan::CDbcMan(CSifMan& sif) :
m_nextSocketId(0),
m_workAddress(0),
m_buttonStatIndex(0x1C)
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

string CDbcMan::GetFunctionName(unsigned int) const
{
    return "unknown";
}

void CDbcMan::Invoke(CMIPS& context, unsigned int functionId)
{
    throw runtime_error("Not implemented.");
}

bool CDbcMan::Invoke(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
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
    return true;
}

void CDbcMan::SaveState(CZipArchiveWriter& archive)
{
    for(SocketMap::const_iterator socketIterator(m_sockets.begin());
        socketIterator != m_sockets.end(); socketIterator++)
    {
        uint32 id = socketIterator->first;
        const SOCKET& socket = socketIterator->second;
        string path = SOCKET_STATE_BASE_PATH + lexical_cast<string>(id) + SOCKET_STATE_EXTENSION;
        CRegisterStateFile* registerFile = new CRegisterStateFile(path.c_str());
        registerFile->SetRegister32("id", id);
        registerFile->SetRegister32("port", socket.nPort);
        registerFile->SetRegister32("slot", socket.nSlot);
        registerFile->SetRegister32("buffer1", socket.buf1);
        registerFile->SetRegister32("buffer2", socket.buf2);
        archive.InsertFile(registerFile);
    }
}

void CDbcMan::LoadState(CZipArchiveReader& archive)
{
    string regexString = SOCKET_STATE_BASE_PATH + string(".*") + SOCKET_STATE_EXTENSION;
    CZipArchiveReader::FileNameList fileList = archive.GetFileNameList(regexString.c_str());
    for(CZipArchiveReader::FileNameList::const_iterator fileIterator(fileList.begin());
        fileIterator != fileList.end(); fileIterator++)
    {
        CRegisterStateFile registerFile(*archive.BeginReadFile(fileIterator->c_str()));
        SOCKET socket;
        uint32 id = registerFile.GetRegister32("id");
        socket.nPort = registerFile.GetRegister32("port");
        socket.nSlot = registerFile.GetRegister32("slot");
        socket.buf1 = registerFile.GetRegister32("buffer1");
        socket.buf2 = registerFile.GetRegister32("buffer2");
        m_sockets[id] = socket;
        m_nextSocketId = max(id + 1, m_nextSocketId);
    }
}

void CDbcMan::SetButtonState(unsigned int nPadNumber, CControllerInfo::BUTTON nButton, bool nPressed, uint8* ram)
{
    uint32 buttonMask = GetButtonMask(nButton);
    uint32 buttonStatIndex = m_buttonStatIndex;

    for(SocketMap::const_iterator socketIterator(m_sockets.begin());
        socketIterator != m_sockets.end(); socketIterator++)
	{
		const SOCKET& socket = socketIterator->second;
		if(socket.nPort != nPadNumber) continue;

        uint8* buffer = &ram[socket.buf1];
        uint16 nStatus = (buffer[buttonStatIndex + 0] << 8) | (buffer[buttonStatIndex + 1]);
		nStatus &= (~buttonMask);
		if(!nPressed)
		{
			nStatus |= buttonMask;
		}

		buffer[buttonStatIndex + 0] = static_cast<uint8>(nStatus >> 8);
		buffer[buttonStatIndex + 1] = static_cast<uint8>(nStatus >> 0);

//		*(uint16*)&CPS2VM::m_pRAM[pSocket->nBuf1 + 0x1C] ^= 0x2010;
//		*(uint16*)&ram[socket.buf1 + 0x3C] ^= 0xFFFF;
	}
}

void CDbcMan::SetAxisState(unsigned int padNumber, CControllerInfo::BUTTON axis, uint8 axisValue, uint8* ram)
{
	static const unsigned int s_axisIndex[4] =
	{
		4,
		5,
		6,
		7
	};

	uint32 buttonStatIndex = m_buttonStatIndex;
	for(SocketMap::const_iterator socketIterator(m_sockets.begin());
		socketIterator != m_sockets.end(); socketIterator++)
	{
		const SOCKET& socket = socketIterator->second;
		if(socket.nPort != padNumber) continue;

		uint8* buffer = &ram[socket.buf1];
		buffer[buttonStatIndex + s_axisIndex[axis]] = axisValue;
	}
}

void CDbcMan::CreateSocket(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	assert(argsSize >= 0x30);
	assert(retSize >= 0x04);

	uint32 nType1   = args[0x00];
	uint32 nType2   = args[0x01];
	uint32 nPort    = args[0x02];
	uint32 nSlot    = args[0x03];
	uint32 nBuf1    = args[0x0A];
	uint32 nBuf2    = args[0x0B];

    SOCKET socket;
	socket.nPort    = nPort;
	socket.nSlot    = nSlot;
	socket.buf1     = nBuf1;
	socket.buf2     = nBuf2;

    uint32 id = m_nextSocketId++;
    m_sockets[id] = socket;

	ret[0x09] = id;

    assert(m_workAddress != 0);
    *(reinterpret_cast<uint32*>(&ram[m_workAddress]) + id) = 1;

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
	assert(argsSize >= 0x90);
	assert(retSize >= 0x90);

	//0 - Some number (0x0200) (size?)
	//1 - Address to bind with

	m_workAddress = args[1];

	//Set Ready (?) status
	*reinterpret_cast<uint32*>(&ram[m_workAddress]) = 1;

	ret[0] = 0x00000000;

	CLog::GetInstance().Print(LOG_NAME, "SetWorkAddr(addr = 0x%0.8X);\r\n", m_workAddress);
}

void CDbcMan::ReceiveData(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	//Param Frame
	//0 - Socket ID
	//1 - Value passed in parameter to the library
	//2 - Some parameter (0x01, or some address)

	uint32 nSocket  = args[0];
	uint32 nFlags   = args[1];
	uint32 nParam	= args[2];

    SocketMap::iterator socketIterator = m_sockets.find(nSocket);
	if(socketIterator != m_sockets.end())
	{
        SOCKET& socket(socketIterator->second);
        uint8* buffer = &ram[socket.buf1];

		buffer[0x02] = 0x20;

        *reinterpret_cast<uint32*>(&buffer[0x04]) = 0x01;

        //For Guilty Gear
		*reinterpret_cast<uint32*>(&buffer[0x7C]) = 0x01;
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

CDbcMan::SOCKET* CDbcMan::GetSocket(uint32 socketId)
{
    SocketMap::iterator socketIterator = m_sockets.find(socketId);
	if(socketIterator == m_sockets.end()) return NULL;
    return &socketIterator->second;
}

uint32 CDbcMan::GetButtonStatIndex() const
{
	return m_buttonStatIndex;
}

void CDbcMan::SetButtonStatIndex(uint32 buttonStatIndex)
{
    m_buttonStatIndex = buttonStatIndex;
}
