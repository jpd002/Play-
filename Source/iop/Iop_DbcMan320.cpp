#include "Iop_DbcMan320.h"
#include "../Log.h"

#define LOG_NAME ("iop_dbcman320")

using namespace Iop;

CDbcMan320::CDbcMan320(CSifMan& sif, CDbcMan& dbcMan) :
m_dbcMan(dbcMan)
{
    sif.RegisterModule(MODULE_ID, this);
}

CDbcMan320::~CDbcMan320()
{

}

std::string CDbcMan320::GetId() const
{
    return "dbcman320";
}

std::string CDbcMan320::GetFunctionName(unsigned int) const
{
    return "unknown";
}

void CDbcMan320::Invoke(CMIPS& context, unsigned int functionId)
{
	throw std::runtime_error("Not implemented.");
}

bool CDbcMan320::Invoke(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	switch(method)
	{
	case 0x80001301:
		m_dbcMan.CreateSocket(args, argsSize, ret, retSize, ram);
		break;
	case 0x80001303:
		{
			InitializeSocket(args, argsSize, ret, retSize, ram);
			CLog::GetInstance().Print(LOG_NAME, "Function0x80001303.\r\n");
//			m_dbcMan.SetButtonStatIndex(0x2C);
			ret[0] = 0x1;
		}
		break;
	case 0x80001304:
		m_dbcMan.SetWorkAddr(args, argsSize, ret, retSize, ram);
		break;
	case 0x8000131A:
		ReceiveData(args, argsSize, ret, retSize, ram);
		break;
	case 0x80001363:
		GetVersionInformation(args, argsSize, ret, retSize, ram);
		break;
	default:
		CLog::GetInstance().Print(LOG_NAME, "Unknown method invoked (0x%0.8X).\r\n", method);
		break;
	}
	return true;
}

void CDbcMan320::SaveState(Framework::CZipArchiveWriter& archive)
{

}

void CDbcMan320::LoadState(Framework::CZipArchiveReader& archive)
{

}

void CDbcMan320::ReceiveData(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	//Param Frame
	//0 - Socket ID
	//1 - Value passed in parameter to the library
	//2 - Some parameter (0x01, or some address)

	uint32 nSocket  = args[0];
	uint32 nFlags   = args[1];
	uint32 nParam	= args[2];

	CDbcMan::SOCKET* socket = m_dbcMan.GetSocket(nSocket);
	if(socket != NULL)
	{
		uint8* buffer = &ram[socket->buf1];

		//For Guilty Gear
		buffer[0x03] = 0x10;

		//For Valkyrie Profile 2 - Size of the data to copy
		buffer[0x02] = 0x10;

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

void CDbcMan320::GetVersionInformation(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	assert(argsSize == 0x90);
	assert(retSize == 0x90);

	ret[0] = 0x00000320;

	CLog::GetInstance().Print(LOG_NAME, "GetVersionInformation();\r\n");
}

void CDbcMan320::InitializeSocket(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	uint32 nSocket = args[0];

	CDbcMan::SOCKET* sock = m_dbcMan.GetSocket(nSocket);
	if(sock == NULL) return;

	uint32 buttonStatIndex = m_dbcMan.GetButtonStatIndex();

	uint8* buffer = &ram[sock->buf1];
	buffer[buttonStatIndex + 4] = 0x7F;
	buffer[buttonStatIndex + 5] = 0x7F;
	buffer[buttonStatIndex + 6] = 0x7F;
	buffer[buttonStatIndex + 7] = 0x7F;
}
