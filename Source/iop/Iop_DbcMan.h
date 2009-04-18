#ifndef _IOP_DBCMAN_H_
#define _IOP_DBCMAN_H_

#include <map>
#include "Iop_Module.h"
#include "Iop_SifMan.h"
#include "../PadListener.h"
#include "zip/ZipArchiveWriter.h"
#include "zip/ZipArchiveReader.h"

namespace Iop
{

	class CDbcMan : public CModule, public CPadListener, public CSifModule
	{
	public:
        struct SOCKET
		{
			uint32 nPort;
			uint32 nSlot;
			uint32 buf1;
			uint32 buf2;
		};

									CDbcMan(CSifMan&);
		virtual						~CDbcMan();
        std::string                 GetId() const;
        std::string                 GetFunctionName(unsigned int) const;
        void                        Invoke(CMIPS&, unsigned int);
        virtual bool				Invoke(uint32, uint32*, uint32, uint32*, uint32, uint8*);
		virtual void				SaveState(CZipArchiveWriter&);
		virtual void				LoadState(CZipArchiveReader&);
        virtual void				SetButtonState(unsigned int, PS2::CControllerInfo::BUTTON, bool, uint8*);
        virtual void                SetAxisState(unsigned int, PS2::CControllerInfo::BUTTON, uint8, uint8*);

		void						CreateSocket(uint32*, uint32, uint32*, uint32, uint8*);
		void						SetWorkAddr(uint32*, uint32, uint32*, uint32, uint8*);
		void						ReceiveData(uint32*, uint32, uint32*, uint32, uint8*);
		void						GetVersionInformation(uint32*, uint32, uint32*, uint32, uint8*);

        void                        SetButtonStatIndex(uint32);
        SOCKET*                     GetSocket(uint32);

    private:
		enum MODULE_ID
		{
			MODULE_ID = 0x80000900
		};

        typedef std::map<uint32, SOCKET> SocketMap;

		SocketMap                   m_sockets;
        uint32                      m_nextSocketId;
        uint32                      m_workAddress;
        uint32                      m_buttonStatIndex;
	};

}

#endif

