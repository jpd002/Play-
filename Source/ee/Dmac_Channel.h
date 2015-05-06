#ifndef _DMAC_CHANNEL_H_
#define _DMAC_CHANNEL_H_

#include "Types.h"
#include <functional>
#include "Convertible.h"
#include "zip/ZipArchiveWriter.h"
#include "zip/ZipArchiveReader.h"

class CDMAC;

namespace Dmac
{
	typedef std::function<uint32 (uint32, uint32, uint32, bool)> DmaReceiveHandler;

	class CChannel
	{
	public:
		enum DMATAG_TYPE
		{
			DMATAG_REFE,
			DMATAG_CNT,
			DMATAG_NEXT,
			DMATAG_REF,
			DMATAG_REFS,
			DMATAG_CALL,
			DMATAG_RET,
			DMATAG_END
		};

		struct CHCR : public convertible<uint32>
		{
			unsigned int		nDIR		: 1;
			unsigned int		nReserved0	: 1;
			unsigned int		nMOD		: 2;
			unsigned int		nASP		: 2;
			unsigned int		nTTE		: 1;
			unsigned int		nTIE		: 1;
			unsigned int		nSTR		: 1;
			unsigned int		nReserved1	: 7;
			unsigned int		nTAG		: 16;
		};


								CChannel(CDMAC&, unsigned int, const DmaReceiveHandler&);
		virtual					~CChannel();

		void					SaveState(Framework::CZipArchiveWriter&);
		void					LoadState(Framework::CZipArchiveReader&);

		void					Reset();
		uint32					ReadCHCR();
		void					WriteCHCR(uint32);
		void					Execute();
		void					ExecuteNormal();
		void					ExecuteSourceChain();
		void					SetReceiveHandler(const DmaReceiveHandler&);

		CHCR					m_CHCR;
		uint32					m_nMADR;
		uint32					m_nQWC;
		uint32					m_nTADR;
		uint32					m_nASR[2];

	private:
		enum SCCTRL_BIT
		{
			SCCTRL_RETTOP		= 0x001,
			SCCTRL_INITXFER		= 0x200,
		};

		void					ClearSTR();

		unsigned int			m_nNumber;
		uint32					m_nSCCTRL;
		DmaReceiveHandler		m_pReceive;
		CDMAC&					m_dmac;
	};
};

#endif
