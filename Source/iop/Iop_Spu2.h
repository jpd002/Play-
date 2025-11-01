#ifndef _SPU2_H_
#define _SPU2_H_

#include <functional>
#include "Iop_Spu2_Core.h"

namespace Iop
{
	class CSpu2
	{
	public:
		CSpu2(CSpuBase&, CSpuBase&);
		virtual ~CSpu2() = default;

		CSpu2(const CSpu2&) = delete;
		CSpu2& operator=(const CSpu2&) = delete;

		uint32 ReadRegister(uint32);
		uint32 WriteRegister(uint32, uint32);

		void Reset();
		Spu2::CCore* GetCore(unsigned int);

		enum
		{
			C_SPDIF_OUT = 0x1F9007C0,
			C_IRQINFO = 0x1F9007C2,
			C_SPDIF_MODE = 0x1F9007C6,
			C_SPDIF_MEDIA = 0x1F9007C8,
		};

		enum
		{
			REGS_BEGIN = 0x1F900000,
			REGS_END = 0x1F90FFFF,
		};

	private:
		typedef std::function<uint32(uint32, uint32)> RegisterAccessFunction;
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

		uint32 ProcessRegisterAccess(const REGISTER_DISPATCH_INFO&, uint32, uint32);
		uint32 ReadRegisterImpl(uint32, uint32);
		uint32 WriteRegisterImpl(uint32, uint32);
		void LogRead(uint32);
		void LogWrite(uint32, uint32);

		REGISTER_DISPATCH_INFO m_readDispatchInfo;
		REGISTER_DISPATCH_INFO m_writeDispatchInfo;
		CorePtr m_core[CORE_NUM];

		uint32 m_spdifOutput = 0x0000;
		uint32 m_spdifMode = 0x0000;
		uint32 m_spdifMedia = 0x0000;
	};
}

#endif
