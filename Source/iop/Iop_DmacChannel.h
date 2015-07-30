#ifndef _IOP_DMACCHANNEL_H_
#define _IOP_DMACCHANNEL_H_

#include "Convertible.h"
#include "Types.h"
#include <functional>

namespace Iop
{
	class CDmac;

	namespace Dmac
	{
		class CChannel
		{
		public:
			typedef std::function<uint32 (uint8*, uint32, uint32)> ReceiveFunctionType;

			enum
			{
				REG_MADR		= 0x00,
				REG_BCR			= 0x04,
				REG_CHCR		= 0x08
			};

			struct BCR : public convertible<uint32>
			{
				unsigned int bs	: 16;
				unsigned int ba	: 16;
			};
			static_assert(sizeof(BCR) == sizeof(uint32), "Size of BCR struct must be 4 bytes.");

			struct CHCR : public convertible<uint32>
			{
				unsigned int dr			: 1;
				unsigned int unused0	: 8;
				unsigned int co			: 1;
				unsigned int li			: 1;
				unsigned int unused1	: 13;
				unsigned int tr			: 1;
				unsigned int unused2	: 7;
			};
			static_assert(sizeof(CHCR) == sizeof(uint32), "Size of CHCR struct must be 4 bytes.");

									CChannel(uint32, unsigned int, CDmac&);
			virtual					~CChannel();

			void					Reset();
			void					SetReceiveFunction(const ReceiveFunctionType&);
			void					ResumeDma();
			uint32					ReadRegister(uint32);
			void					WriteRegister(uint32, uint32);

		private:
			ReceiveFunctionType		m_receiveFunction;
			unsigned int			m_number;
			uint32					m_baseAddress;
			uint32					m_MADR;
			BCR						m_BCR;
			CHCR					m_CHCR;
			CDmac&					m_dmac;
		};
	}
}

#endif
