#ifndef _DMACHANNEL_H_
#define _DMACHANNEL_H_

#include "Types.h"
#include "Convertible.h"
#include <functional>
#include <boost/static_assert.hpp>

namespace Psx
{
	class CDmac;

	class CDmaChannel
	{
	public:
		typedef std::tr1::function<uint32 (uint8*, uint32, uint32)> ReceiveFunctionType;

								CDmaChannel(uint32, unsigned int, CDmac&);
		virtual					~CDmaChannel();

		void					Reset();
		void					SetReceiveFunction(const ReceiveFunctionType&);

		void					ResumeDma();

		uint32					ReadRegister(uint32);
		void					WriteRegister(uint32, uint32);

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
		BOOST_STATIC_ASSERT(sizeof(BCR) == sizeof(uint32));

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
		BOOST_STATIC_ASSERT(sizeof(CHCR) == sizeof(uint32));

	private:
		ReceiveFunctionType		m_receiveFunction;
		unsigned int			m_number;
		uint32					m_baseAddress;
		uint32					m_MADR;
		BCR						m_BCR;
		CHCR					m_CHCR;
		CDmac&					m_dmac;
	};
};

#endif
