#include "Iop_DbcMan320.h"
#include "../Log.h"

#define LOG_NAME ("iop_dbcman320")

using namespace Iop;
using namespace std;
using namespace Framework;
using namespace boost;

CDbcMan320::CDbcMan320(CSIF& sif, CDbcMan& dbcMan) :
m_dbcMan(dbcMan)
{
    sif.RegisterModule(MODULE_ID, this);
}

CDbcMan320::~CDbcMan320()
{

}

string CDbcMan320::GetId() const
{
    return "dbcman320";
}

void CDbcMan320::Invoke(CMIPS& context, unsigned int functionId)
{
    throw runtime_error("Not implemented.");
}

void CDbcMan320::Invoke(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	switch(method)
	{
    case 0x80001301:
        m_dbcMan.CreateSocket(args, argsSize, ret, retSize, ram);
        break;
    case 0x80001303:
        {
            CLog::GetInstance().Print(LOG_NAME, "Function0x80001303.\r\n");
            ret[0] = 0x1;
        }
        break;
	case 0x80001304:
		m_dbcMan.SetWorkAddr(args, argsSize, ret, retSize, ram);
		break;
    case 0x8000131A:
        m_dbcMan.ReceiveData(args, argsSize, ret, retSize, ram);
        break;
	case 0x80001363:
		GetVersionInformation(args, argsSize, ret, retSize, ram);
		break;
	default:
		CLog::GetInstance().Print(LOG_NAME, "Unknown method invoked (0x%0.8X).\r\n", method);
		break;
	}
}

void CDbcMan320::SaveState(CZipArchiveWriter& archive)
{

}

void CDbcMan320::LoadState(CZipArchiveReader& archive)
{

}

void CDbcMan320::GetVersionInformation(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	assert(argsSize == 0x90);
	assert(retSize == 0x90);

	ret[0] = 0x00000320;

	CLog::GetInstance().Print(LOG_NAME, "GetVersionInformation();\r\n");
}
