#pragma once

#include "GSHandler.h"

namespace GsTransfer
{
	struct TransferRangeSrc
	{
		static uint32 GetBufferAddress(const CGSHandler::BITBLTBUF& bltBuf)
		{
			return bltBuf.GetSrcPtr();
		}
		static uint32 GetBufferWidth(const CGSHandler::BITBLTBUF& bltBuf)
		{
			return bltBuf.GetSrcWidth();
		}
		static uint32 GetBufferPsm(const CGSHandler::BITBLTBUF& bltBuf)
		{
			return bltBuf.nSrcPsm;
		}
		static uint32 GetSAX(const CGSHandler::TRXPOS& trxPos)
		{
			return trxPos.nSSAX;
		}
		static uint32 GetSAY(const CGSHandler::TRXPOS& trxPos)
		{
			return trxPos.nSSAY;
		}
	};

	struct TransferRangeDst
	{
		static uint32 GetBufferAddress(const CGSHandler::BITBLTBUF& bltBuf)
		{
			return bltBuf.GetDstPtr();
		}
		static uint32 GetBufferWidth(const CGSHandler::BITBLTBUF& bltBuf)
		{
			return bltBuf.GetDstWidth();
		}
		static uint32 GetBufferPsm(const CGSHandler::BITBLTBUF& bltBuf)
		{
			return bltBuf.nDstPsm;
		}
		static uint32 GetSAX(const CGSHandler::TRXPOS& trxPos)
		{
			return trxPos.nDSAX;
		}
		static uint32 GetSAY(const CGSHandler::TRXPOS& trxPos)
		{
			return trxPos.nDSAY;
		}
	};

	template <typename TransferRangeTraits>
	static std::pair<uint32, uint32> GetRange(const CGSHandler::BITBLTBUF& bltBuf, const CGSHandler::TRXREG& trxReg, const CGSHandler::TRXPOS& trxPos)
	{
		uint32 transferAddress = TransferRangeTraits::GetBufferAddress(bltBuf);
		uint32 psm = TransferRangeTraits::GetBufferPsm(bltBuf);

		//Find the pages that are touched by this transfer
		auto transferPageSize = CGsPixelFormats::GetPsmPageSize(psm);

		// DBZ Budokai Tenkaichi 2 and 3 use invalid (empty) buffer sizes
		// Account for that, by assuming trxReg.nRRW.
		auto width = TransferRangeTraits::GetBufferWidth(bltBuf);
		if(width == 0)
		{
			width = trxReg.nRRW;
		}

		uint32 sax = TransferRangeTraits::GetSAX(trxPos);
		uint32 say = TransferRangeTraits::GetSAY(trxPos);

		//Make sure we are within our buffer
		assert((trxReg.nRRW + sax) <= width);

		//Espgaluda uses an offset into a big memory area. The Y offset is not necessarily
		//a multiple of the page height. We need to make sure to take this into account.
		uint32 intraPageOffsetY = say % transferPageSize.second;

		uint32 pageCountX = (width + transferPageSize.first - 1) / transferPageSize.first;
		uint32 pageCountY = (intraPageOffsetY + trxReg.nRRH + transferPageSize.second - 1) / transferPageSize.second;

		uint32 pageCount = pageCountX * pageCountY;
		uint32 transferSize = pageCount * CGsPixelFormats::PAGESIZE;
		uint32 transferOffset = (say / transferPageSize.second) * pageCountX * CGsPixelFormats::PAGESIZE;

		return std::make_pair(transferAddress + transferOffset, transferSize);
	}

	static auto GetSrcRange(const CGSHandler::BITBLTBUF& bltBuf, const CGSHandler::TRXREG& trxReg, const CGSHandler::TRXPOS& trxPos)
	{
		return GetRange<TransferRangeSrc>(bltBuf, trxReg, trxPos);
	}

	static auto GetDstRange(const CGSHandler::BITBLTBUF& bltBuf, const CGSHandler::TRXREG& trxReg, const CGSHandler::TRXPOS& trxPos)
	{
		return GetRange<TransferRangeDst>(bltBuf, trxReg, trxPos);
	}
}
