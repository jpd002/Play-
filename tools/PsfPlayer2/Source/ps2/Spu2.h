#ifndef _SPU2_H_
#define _SPU2_H_

#include <vector>
#include <functional>
#include "Spu2_Core.h"

namespace PS2
{
	class CSpu2
	{
	public:
						CSpu2(uint32);
		virtual			~CSpu2();

		uint32			ReadRegister(uint32);
		uint32			WriteRegister(uint32, uint32);

        enum
        {
            REGS_BEGIN  = 0x1F900000,
            REGS_END    = 0x1F90FFFF,
        };

	private:
		typedef std::tr1::function<uint32 (uint32, uint32)> RegisterAccessFunction;
		typedef std::vector<Spu2::CCore> CoreArrayType;

		enum
		{
			CORE_NUM = 2
		};

		struct REGISTER_DISPATCH_INFO
		{
			RegisterAccessFunction global;
			RegisterAccessFunction core[2];
		};

		uint32						ProcessRegisterAccess(const REGISTER_DISPATCH_INFO&, uint32, uint32);
		uint32						ReadRegisterImpl(uint32, uint32);
		uint32						WriteRegisterImpl(uint32, uint32);
		void						LogRead(uint32);
		void						LogWrite(uint32, uint32);

		REGISTER_DISPATCH_INFO		m_readDispatchInfo;
		REGISTER_DISPATCH_INFO		m_writeDispatchInfo;
		uint32						m_baseAddress;
		CoreArrayType				m_cores;
	};
}

#endif
