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

		assert(trxReg.nRRW != 0);
		assert(trxReg.nRRH != 0);

		//Since there's a possibility of using RRW for the buffer width, we need to round up
		uint32 pagePitch = (width + transferPageSize.first - 1) / transferPageSize.first;

		uint32 sax = TransferRangeTraits::GetSAX(trxPos);
		uint32 say = TransferRangeTraits::GetSAY(trxPos);

		uint32 pageStartX = sax / transferPageSize.first;
		uint32 pageStartY = say / transferPageSize.second;
		uint32 pageEndX = (sax + trxReg.nRRW - 1) / transferPageSize.first;
		uint32 pageEndY = (say + trxReg.nRRH - 1) / transferPageSize.second;

		//Single page transfer
		if((pageStartX == pageEndX) && (pageStartY == pageEndY))
		{
			uint32 transferPage = (pageStartY * pagePitch) + pageStartX;
			uint32 transferOffset = transferPage * CGsPixelFormats::PAGESIZE;
			return std::make_pair(transferAddress + transferOffset, CGsPixelFormats::PAGESIZE);
		}

		uint32 pageCountY = pageEndY - pageStartY + 1;

		uint32 pageCount = pagePitch * pageCountY;
		uint32 transferSize = pageCount * CGsPixelFormats::PAGESIZE;
		uint32 transferOffset = ((pageStartY * pagePitch) + pageStartX) * CGsPixelFormats::PAGESIZE;

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
