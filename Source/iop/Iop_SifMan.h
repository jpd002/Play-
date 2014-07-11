#ifndef _IOP_SIFMAN_H_
#define _IOP_SIFMAN_H_

#include "../SifModule.h"
#include "Iop_Module.h"

namespace Iop
{
	class CSysmem;

	class CSifMan : public CModule
	{
	public:
								CSifMan();
		virtual					~CSifMan();

		void					GenerateHandlers(uint8*, CSysmem&);

		virtual std::string		GetId() const;
		virtual std::string		GetFunctionName(unsigned int) const;
		virtual void			Invoke(CMIPS&, unsigned int);

		virtual void			RegisterModule(uint32, CSifModule*) = 0;
		virtual bool			IsModuleRegistered(uint32) = 0;
		virtual void			UnregisterModule(uint32) = 0;
		virtual void			SendPacket(void*, uint32) = 0;
		virtual void			SetDmaBuffer(uint32, uint32) = 0;
		virtual void			SendCallReply(uint32, const void*) = 0;
		virtual void			GetOtherData(uint32, uint32, uint32) = 0;

		virtual uint32			SifSetDma(uint32, uint32);

	protected:
		virtual uint32			SifDmaStat(uint32);
		virtual uint32			SifSetDmaCallback(CMIPS&, uint32, uint32, uint32, uint32);

		uint32					m_sifSetDmaCallbackHandlerPtr;
	};
}

#endif
