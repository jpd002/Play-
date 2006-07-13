#ifndef _IOP_DBCMAN_H_
#define _IOP_DBCMAN_H_

#include "IOP_Module.h"
#include "PadListener.h"
#include "List.h"

namespace IOP
{

	class CDbcMan : public CModule, public CPadListener
	{
	public:
									CDbcMan();
		virtual						~CDbcMan();
		virtual void				Invoke(uint32, void*, uint32, void*, uint32);
		virtual void				SaveState(Framework::CStream*);
		virtual void				LoadState(Framework::CStream*);
		virtual void				SetButtonState(unsigned int, CPadListener::BUTTON, bool);

		enum MODULE_ID
		{
			MODULE_ID = 0x80000900
		};

	private:
		struct SOCKET
		{
			uint32 nPort;
			uint32 nSlot;
			uint32 nBuf1;
			uint32 nBuf2;
		};

		void						CreateSocket(void*, uint32, void*, uint32);
		void						SetWorkAddr(void*, uint32, void*, uint32);
		void						ReceiveData(void*, uint32, void*, uint32);
		void						GetVersionInformation(void*, uint32, void*, uint32);

		void						DeleteAllSockets();

		void						Log(const char*, ...);

		Framework::CList<SOCKET>	m_Socket;
	};

}

#endif

