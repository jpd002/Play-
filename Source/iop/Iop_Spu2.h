#ifndef _SPU2_H_
#define _SPU2_H_

#include <boost/utility.hpp>
#include <functional>
#include "Iop_Spu2_Core.h"

namespace Iop
{
	class CSpu2 : boost::noncopyable
	{
	public:
						CSpu2(CSpuBase&, CSpuBase&);
		virtual			~CSpu2();

		uint32			ReadRegister(uint32);
		uint32			WriteRegister(uint32, uint32);

		void			Reset();
		Spu2::CCore*	GetCore(unsigned int);

		enum
		{
			C_IRQINFO	= 0x1F9007C2
		};

		enum
		{
			REGS_BEGIN	= 0x1F900000,
			REGS_END	= 0x1F90FFFF,
		};

	private:
		typedef std::function<uint32 (uint32, uint32)> RegisterAccessFunction;
		typedef std::unique_ptr<Spu2::CCore> CorePtr;

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
		CorePtr						m_core[CORE_NUM];
	};
}

#endif
