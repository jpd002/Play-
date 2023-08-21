#include "GsTransferInvalidationTest.h"
#include "gs/GSHandler.h"
#include "gs/GsPixelFormats.h"
#include "gs/GsTransferRange.h"

static CGSHandler::BITBLTBUF MakeSrcBltBuf(uint32 psm, uint32 bufPtr, uint32 bufWidth)
{
	assert((bufPtr & 0xFF) == 0);
	assert((bufWidth & 0x3F) == 0);

	auto bltBuf = make_convertible<CGSHandler::BITBLTBUF>(0);
	bltBuf.nSrcPsm = psm;
	bltBuf.nSrcPtr = bufPtr / 0x100;
	bltBuf.nSrcWidth = bufWidth / 0x40;
	return bltBuf;
}

static CGSHandler::BITBLTBUF MakeDstBltBuf(uint32 psm, uint32 bufPtr, uint32 bufWidth)
{
	assert((bufPtr & 0xFF) == 0);
	assert((bufWidth & 0x3F) == 0);

	auto bltBuf = make_convertible<CGSHandler::BITBLTBUF>(0);
	bltBuf.nDstPsm = psm;
	bltBuf.nDstPtr = bufPtr / 0x100;
	bltBuf.nDstWidth = bufWidth / 0x40;
	return bltBuf;
}

static CGSHandler::TRXREG MakeTrxReg(uint32 width, uint32 height)
{
	auto trxReg = make_convertible<CGSHandler::TRXREG>(0);
	trxReg.nRRW = width;
	trxReg.nRRH = height;
	return trxReg;
}

static CGSHandler::TRXPOS MakeSrcTrxPos(uint32 x, uint32 y)
{
	auto trxPos = make_convertible<CGSHandler::TRXPOS>(0);
	trxPos.nSSAX = x;
	trxPos.nSSAY = y;
	return trxPos;
}

static CGSHandler::TRXPOS MakeDstTrxPos(uint32 x, uint32 y)
{
	auto trxPos = make_convertible<CGSHandler::TRXPOS>(0);
	trxPos.nDSAX = x;
	trxPos.nDSAY = y;
	return trxPos;
}

template <uint32 psm, typename Storage>
static void SinglePageTransferTest()
{
	uint32 bufPtr = 0x300000;
	uint32 bufWidth = Storage::PAGEWIDTH;

	auto bltBuf = MakeDstBltBuf(psm, bufPtr, bufWidth);
	auto trxReg = MakeTrxReg(Storage::PAGEWIDTH, Storage::PAGEHEIGHT);
	auto trxPos = make_convertible<CGSHandler::TRXPOS>(0);

	auto [transferAddress, transferSize] = GsTransfer::GetDstRange(bltBuf, trxReg, trxPos);

	TEST_VERIFY(transferAddress == bufPtr);
	TEST_VERIFY(transferSize == CGsPixelFormats::PAGESIZE);
}

template <uint32 psm, typename Storage>
static void SinglePageOffsetTransferTest(uint32 offsetX, uint32 offsetY)
{
	uint32 bufPtr = 0x11A000;
	uint32 bufWidth = 640;

	assert(offsetX <= bufWidth);

	uint32 pageOffsetX = offsetX / Storage::PAGEWIDTH;
	uint32 pageOffsetY = offsetY / Storage::PAGEHEIGHT;
	uint32 pagePitch = bufWidth / Storage::PAGEWIDTH;

	auto bltBuf = MakeSrcBltBuf(psm, bufPtr, bufWidth);
	auto trxReg = MakeTrxReg(1, 4);
	auto trxPos = MakeSrcTrxPos(offsetX, offsetY);

	auto [transferAddress, transferSize] = GsTransfer::GetSrcRange(bltBuf, trxReg, trxPos);

	TEST_VERIFY(transferAddress == bufPtr + (((pagePitch * pageOffsetY) + pageOffsetX) * CGsPixelFormats::PAGESIZE));
	TEST_VERIFY(transferSize == CGsPixelFormats::PAGESIZE);
}

template <uint32 psm, typename Storage>
static void MultiPageTransferTest()
{
	uint32 bufPtr = 0x120000;
	uint32 bufWidth = 512;

	uint32 transferHeight = 256;

	uint32 pageCountY = transferHeight / Storage::PAGEHEIGHT;
	uint32 pagePitch = bufWidth / Storage::PAGEWIDTH;

	auto bltBuf = MakeDstBltBuf(psm, bufPtr, bufWidth);
	auto trxReg = MakeTrxReg(256, transferHeight);
	auto trxPos = MakeDstTrxPos(16, 16);

	auto [transferAddress, transferSize] = GsTransfer::GetDstRange(bltBuf, trxReg, trxPos);

	TEST_VERIFY(transferAddress == bufPtr);
	TEST_VERIFY(transferSize == (pageCountY + 1) * pagePitch * CGsPixelFormats::PAGESIZE);
}

static void SimpleOffsetYTransferTest()
{
	//In this test, the transfer position is skipping a few pages
	//transferAddress should start at (bufWidth / 64) * 2
	//This is used for movie playback in many games

	uint32 bufPtr = 0x100000;
	uint32 bufWidth = 640;
	uint32 pageCountX = bufWidth / CGsPixelFormats::STORAGEPSMCT32::PAGEWIDTH;
	uint32 pageOffsetY = 2;

	auto bltBuf = MakeDstBltBuf(CGSHandler::PSMCT32, bufPtr, bufWidth);
	auto trxReg = MakeTrxReg(16, 16);
	auto trxPos = MakeDstTrxPos(0, CGsPixelFormats::STORAGEPSMCT32::PAGEHEIGHT * pageOffsetY);

	auto [transferAddress, transferSize] = GsTransfer::GetDstRange(bltBuf, trxReg, trxPos);

	TEST_VERIFY(transferAddress == (bufPtr + (CGsPixelFormats::PAGESIZE * pageCountX * pageOffsetY)));
	TEST_VERIFY(transferSize == CGsPixelFormats::PAGESIZE);
}

static void StraddleOffsetYTransferTest()
{
	//Based on FF12 opening movie (when you start a new game)

	uint32 bufPtr = 0;
	uint32 bufWidth = 512;

	auto bltBuf = MakeDstBltBuf(CGSHandler::PSMCT32, bufPtr, bufWidth);
	auto trxReg = MakeTrxReg(16, 16);
	auto trxPos = MakeDstTrxPos(0, 1394);

	auto [transferAddress, transferSize] = GsTransfer::GetDstRange(bltBuf, trxReg, trxPos);

	TEST_VERIFY(transferAddress == 0x002B0000);
	TEST_VERIFY(transferSize == 0x00020000);
}

static void ZeroBufWidthTransferTest()
{
	//This is done by DBZ: Tenkaichi 2
	//This transfer should touch a page at least

	uint32 bufPtr = 0x380000;
	uint32 bufWidth = 0;

	auto bltBuf = MakeDstBltBuf(CGSHandler::PSMCT32, bufPtr, bufWidth);
	auto trxReg = MakeTrxReg(16, 16);
	auto trxPos = make_convertible<CGSHandler::TRXPOS>(0);

	auto [transferAddress, transferSize] = GsTransfer::GetDstRange(bltBuf, trxReg, trxPos);

	TEST_VERIFY(transferAddress == bufPtr);
	TEST_VERIFY(transferSize == CGsPixelFormats::PAGESIZE);
}

void EspgaludaTransferTest()
{
	uint32 bufPtr = 0x320000;
	uint32 bufWidth = 1024;

	auto bltBuf = MakeDstBltBuf(CGSHandler::PSMT8, bufPtr, bufWidth);
	auto trxReg = MakeTrxReg(160, 128);
	auto trxPos = MakeDstTrxPos(16, 16);

	auto [transferAddress, transferSize] = GsTransfer::GetDstRange(bltBuf, trxReg, trxPos);

	//PSMT8 page size is 128x64
	//This transfer spans 8 pages on the X dimension -> ceil(bufWidth / psm8.pageWidth) -> ceil(1024 / 128)
	//This transfer also touches 3 pages on the Y dimension -> ceil((trxReg.y + trxPos.dy) / psm8.pageHeight) -> ceil((128 + 16) / 64)

	TEST_VERIFY(transferAddress == bufPtr);
	TEST_VERIFY(transferSize == (8 * 3) * CGsPixelFormats::PAGESIZE);
}

void CGsTransferInvalidationTest::Execute()
{
	SinglePageTransferTest<CGSHandler::PSMCT32, CGsPixelFormats::STORAGEPSMCT32>();
	SinglePageTransferTest<CGSHandler::PSMCT16, CGsPixelFormats::STORAGEPSMCT16>();
	SinglePageTransferTest<CGSHandler::PSMT8, CGsPixelFormats::STORAGEPSMT8>();
	SinglePageOffsetTransferTest<CGSHandler::PSMZ32, CGsPixelFormats::STORAGEPSMZ32>(158, 259);
	SinglePageOffsetTransferTest<CGSHandler::PSMZ16S, CGsPixelFormats::STORAGEPSMZ16S>(432, 780);
	MultiPageTransferTest<CGSHandler::PSMCT32, CGsPixelFormats::STORAGEPSMCT32>();
	MultiPageTransferTest<CGSHandler::PSMT4, CGsPixelFormats::STORAGEPSMT4>();
	SimpleOffsetYTransferTest();
	StraddleOffsetYTransferTest();
	ZeroBufWidthTransferTest();
	EspgaludaTransferTest();
}
