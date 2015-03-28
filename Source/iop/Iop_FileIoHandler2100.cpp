#include "Iop_FileIoHandler2100.h"
#include "Iop_Ioman.h"
#include "../Log.h"

#define LOG_NAME ("iop_fileio")

using namespace Iop;

CFileIoHandler2100::CFileIoHandler2100(CIoman* ioman)
: CHandler(ioman)
{

}

void CFileIoHandler2100::Invoke(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	switch(method)
	{
	case 0:
		{
			assert(retSize == 4);
			auto command = reinterpret_cast<OPENCOMMAND*>(args);
			*ret = m_ioman->Open(command->flags, command->fileName);
		}
		break;
	case 1:
		{
			assert(retSize == 4);
			auto command = reinterpret_cast<CLOSECOMMAND*>(args);
			*ret = m_ioman->Close(command->fd);
		}
		break;
	case 2:
		{
			assert(retSize == 4);
			auto command = reinterpret_cast<READCOMMAND*>(args);
			*ret = m_ioman->Read(command->fd, command->size, reinterpret_cast<void*>(ram + command->buffer));
		}
		break;
	case 4:
		{
			assert(retSize == 4);
			auto command = reinterpret_cast<SEEKCOMMAND*>(args);
			*ret = m_ioman->Seek(command->fd, command->offset, command->whence);
		}
		break;
	case 255:
		//Not really sure about that...
		if(retSize == 8)
		{
			memcpy(ret, "....rawr", 8);
		}
		else if(retSize == 4)
		{
			memcpy(ret, "....", 4);
		}
		else
		{
			assert(0);
		}
		break;
	default:
		CLog::GetInstance().Print(LOG_NAME, "Unknown function (%d) called.\r\n", method);
		break;
	}
}
