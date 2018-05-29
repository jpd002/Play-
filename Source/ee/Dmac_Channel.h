#pragma once

#include "Types.h"
#include <functional>
#include "Convertible.h"
#include "zip/ZipArchiveWriter.h"
#include "zip/ZipArchiveReader.h"

class CDMAC;

namespace Dmac
{
	typedef std::function<uint32(uint32, uint32, uint32, bool)> DmaReceiveHandler;

	class CChannel
	{
	public:
		enum DMATAG_SRC_TYPE
		{
			DMATAG_SRC_REFE,
			DMATAG_SRC_CNT,
			DMATAG_SRC_NEXT,
			DMATAG_SRC_REF,
			DMATAG_SRC_REFS,
			DMATAG_SRC_CALL,
			DMATAG_SRC_RET,
			DMATAG_SRC_END
		};

		enum DMATAG_DST_TYPE
		{
			DMATAG_DST_CNTS,
			DMATAG_DST_CNT,
			DMATAG_DST_END = 7,
		};

		struct DMAtag : public convertible<uint64>
		{
			unsigned int qwc : 16;
			unsigned int reserved : 10;
			unsigned int pce : 2;
			unsigned int id : 3;
			unsigned int irq : 1;
			unsigned int addr : 32;
		};
		static_assert(sizeof(DMAtag) == sizeof(uint64), "Size of DMAtag struct must be 8 bytes.");

		enum CHCR_DIR
		{
			CHCR_DIR_TO = 0,
			CHCR_DIR_FROM = 1,
		};

		struct CHCR : public convertible<uint32>
		{
			unsigned int nDIR : 1;
			unsigned int nReserved0 : 1;
			unsigned int nMOD : 2;
			unsigned int nASP : 2;
			unsigned int nTTE : 1;
			unsigned int nTIE : 1;
			unsigned int nSTR : 1;
			unsigned int nReserved1 : 7;
			unsigned int nTAG : 16;
		};

		CChannel(CDMAC&, unsigned int, const DmaReceiveHandler&);
		virtual ~CChannel() = default;

		void SaveState(Framework::CZipArchiveWriter&);
		void LoadState(Framework::CZipArchiveReader&);

		void Reset();
		uint32 ReadCHCR();
		void WriteCHCR(uint32);
		void Execute();
		void ExecuteNormal();
		void ExecuteInterleave();
		void ExecuteSourceChain();
		void ExecuteDestinationChain();
		void SetReceiveHandler(const DmaReceiveHandler&);

		CHCR m_CHCR;
		uint32 m_nMADR;
		uint32 m_nQWC;
		uint32 m_nTADR;
		uint32 m_nASR[2];

	private:
		enum SCCTRL_BIT
		{
			SCCTRL_RETTOP = 0x001,
			SCCTRL_INITXFER = 0x200,
		};

		void ClearSTR();

		unsigned int m_number = 0;
		uint32 m_nSCCTRL;
		DmaReceiveHandler m_receive;
		CDMAC& m_dmac;
	};
};
