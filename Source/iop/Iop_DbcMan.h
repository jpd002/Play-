#ifndef _IOP_DBCMAN_H_
#define _IOP_DBCMAN_H_

#include <map>
#include "Iop_Module.h"
#include "../SIF.h"
#include "../PadListener.h"
#include "zip/ZipArchiveWriter.h"
#include "zip/ZipArchiveReader.h"

namespace Iop
{

	class CDbcMan : public CModule, public CPadListener, public CSifModule
	{
	public:
									CDbcMan(CSIF& sif);
		virtual						~CDbcMan();
        std::string                 GetId() const;
        void                        Invoke(CMIPS&, unsigned int);
        virtual void				Invoke(uint32, uint32*, uint32, uint32*, uint32, uint8*);
		virtual void				SaveState(CZipArchiveWriter&);
		virtual void				LoadState(CZipArchiveReader&);
		virtual void				SetButtonState(unsigned int, CPadListener::BUTTON, bool, uint8*);

	private:
		enum MODULE_ID
		{
			MODULE_ID = 0x80000900
		};

        struct SOCKET
		{
			uint32 nPort;
			uint32 nSlot;
			uint32 buf1;
			uint32 buf2;
		};

        typedef std::map<uint32, SOCKET> SocketMap;

		void						CreateSocket(uint32*, uint32, uint32*, uint32, uint8*);
		void						SetWorkAddr(uint32*, uint32, uint32*, uint32, uint8*);
		void						ReceiveData(uint32*, uint32, uint32*, uint32, uint8*);
		void						GetVersionInformation(uint32*, uint32, uint32*, uint32, uint8*);

		SocketMap                   m_sockets;
        uint32                      m_nextSocketId;
	};

}

#endif

