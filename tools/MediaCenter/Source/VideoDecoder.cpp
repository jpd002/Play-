#include "VideoDecoder.h"
#include "StreamBitStream.h"
#include "StdStream.h"
#include "MemStream.h"
#include "ProgramStreamDecoder.h"
#include "VideoStream_Decoder.h"
#include "RawMpeg2Container.h"
#include <assert.h>

#if 0

#include "XaSoundStreamDecoder.h"
#include "Ac3SoundStreamDecoder.h"

CMemStream									g_soundStream;
ProgramStreamDecoder::PRIVATE_STREAM1_INFO	g_soundStreamInfo;

//-----------------------------------------------------------
//Sound Stuff
//-----------------------------------------------------------

void Ac3SoundStreamProcessor()
{
	static Ac3SoundStreamDecoder* decoder(NULL);
	if(decoder == NULL)
	{
		decoder = new Ac3SoundStreamDecoder();
	}

	static CStreamBitStream audioBitStream(g_soundStream);

	g_soundStream.Seek(g_soundStreamInfo.firstAccessUnit - 1, Framework::STREAM_SEEK_SET);
	g_soundStream.Truncate();

	decoder->Execute(audioBitStream);
}

void XaSoundStreamProcessor()
{
	static XaSoundStreamDecoder* decoder(NULL);
	if(decoder == NULL)
	{
		decoder = new XaSoundStreamDecoder();
	}

	if(g_soundStreamInfo.subStreamNumber == 0xFF)
	{
		//PS2 sound stuff

		//Only process the first channel
		if(g_soundStreamInfo.firstAccessUnit != 0)
		{
			g_soundStream.Seek(0, Framework::STREAM_SEEK_END);
			g_soundStream.Truncate();
		}
		else
		{
			decoder->Execute(g_soundStream);
			g_soundStream.Truncate();
		}
	}
}

bool SoundStreamHandler(CBitStream& stream, const ProgramStreamDecoder::PRIVATE_STREAM1_INFO& streamInfo, uint32 size)
{
	g_soundStream.Seek(0, Framework::STREAM_SEEK_END);
	for(unsigned int i = 0; i < size; i++)
	{
		uint8 data = static_cast<uint8>(stream.GetBits_MSBF(8));
		g_soundStream.Write8(data);
	}
	g_soundStream.Seek(0, Framework::STREAM_SEEK_SET);
	g_soundStreamInfo = streamInfo;
	return false;
}

#endif

CVideoDecoder::CVideoDecoder(std::string path)
    : m_threadDone(false)
{
	m_decoderThread = std::thread([=]() { DecoderThreadProc(path); });
}

CVideoDecoder::~CVideoDecoder()
{
	if(m_decoderThread.joinable())
	{
		m_threadDone = true;
		m_decoderThread.join();
	}
}

void CVideoDecoder::DecoderThreadProc(std::string path)
{
	Framework::CMemStream videoStream;
	Framework::CStreamBitStream videoBitStream(videoStream);

	Framework::CStdStream stream(path.c_str(), "rb");
	uint32 firstWord = stream.Read32();
	stream.Seek(0, Framework::STREAM_SEEK_END);
	uint64 fileSize = stream.Tell();
	stream.Seek(0, Framework::STREAM_SEEK_SET);

	//Initialize container
	CVideoContainer* container(nullptr);
	if(firstWord == 0xba010000)
	{
		auto programStreamDecoder = new ProgramStreamDecoder(stream);
		//	programStreamDecoder->RegisterPrivateStream1Handler(std::tr1::bind(&SoundStreamHandler, PLACEHOLDER_1, PLACEHOLDER_2, PLACEHOLDER_3));
		programStreamDecoder->RegisterVideoStreamHandler(
		    [&](Framework::CBitStream& stream, uint32 size) {
			    assert(videoStream.IsEOF());
			    videoStream.ResetBuffer();
			    for(unsigned int i = 0; i < size; i++)
			    {
				    videoStream.Write8(static_cast<uint8>(stream.GetBits_MSBF(8)));
			    }
			    videoStream.Seek(0, Framework::STREAM_SEEK_SET);
			    return false;
		    });
		container = programStreamDecoder;
	}
	else
	{
		auto rawMpeg2Container = new CRawMpeg2Container(stream);
		rawMpeg2Container->RegisterVideoStreamHandler(
		    [&](uint8* buffer, uint32 bufferSize) {
			    assert(videoStream.IsEOF());
			    videoStream.ResetBuffer();
			    videoStream.Write(buffer, bufferSize);
			    videoStream.Seek(0, Framework::STREAM_SEEK_SET);
		    });
		container = rawMpeg2Container;
	}

	MPEG_VIDEO_STATE videoState;
	VideoStream::Decoder videoStreamDecoder;
	videoStreamDecoder.InitializeState(&videoState);
	videoStreamDecoder.Reset();
	videoStreamDecoder.RegisterOnMacroblockDecodedHandler([&](MPEG_VIDEO_STATE* state) { OnMacroblockDecoded(state); });
	videoStreamDecoder.RegisterOnPictureDecodedHandler([&](MPEG_VIDEO_STATE* state) { OnPictureDecoded(state); });

	while(!m_threadDone)
	{
		if(!videoStream.IsEOF())
		{
			try
			{
				videoStreamDecoder.Execute(&videoState, videoBitStream);
			}
			catch(const Framework::CBitStream::CBitStreamException&)
			{
#ifdef _DEBUG
				uint64 filePos = stream.Tell();
				printf("Video Stream Buffer exhausted (%d/%d).\r\n", static_cast<uint32>(filePos), static_cast<uint32>(fileSize));
#endif
			}
		}
		/*
		if(!g_soundStream.IsEOF())
		{
			switch(g_soundStreamInfo.subStreamNumber)
			{
			case 0xFF:
				XaSoundStreamProcessor();
				break;
			//case 0x80:
			//	Ac3SoundStreamProcessor();
			//	break;
			}
		}
*/
		CVideoContainer::STATUS status = container->Read();
		if(status == ProgramStreamDecoder::STATUS_EOF)
		{
			break;
		}
		else if(status == ProgramStreamDecoder::STATUS_ERROR)
		{
			assert(0);
		}
		else if(status == ProgramStreamDecoder::STATUS_INTERRUPTED)
		{
		}
	}
}

void CVideoDecoder::OnMacroblockDecoded(MPEG_VIDEO_STATE* state)
{
	const int blockWidth = 16;
	const int blockHeight = 16;

	MACROBLOCK& macroblock(state->macroblock);
	PICTURE_HEADER& pictureHeader(state->pictureHeader);
	SEQUENCE_HEADER& sequenceHeader(state->sequenceHeader);
	BLOCK_DECODER_STATE& decoderState(state->blockDecoderState);

	if(
	    (m_currentFrame.IsEmpty()) ||
	    (m_currentFrame.GetWidth() != sequenceHeader.horizontalSize) ||
	    (m_currentFrame.GetHeight() != sequenceHeader.verticalSize))
	{
		m_currentFrame = FRAME(sequenceHeader.horizontalSize, sequenceHeader.verticalSize);
	}

	unsigned int macroblockX = decoderState.currentMbAddress % sequenceHeader.macroblockWidth;
	unsigned int macroblockY = decoderState.currentMbAddress / sequenceHeader.macroblockWidth;

	if(pictureHeader.pictureCodingType == PICTURE_TYPE_B)
	{
		return;
	}

	if(decoderState.macroblockType & MACROBLOCK_MODE_INTRA)
	{
		CopyBlock8x8(m_currentFrame.y, (macroblockX * 16) + 0, (macroblockY * 16) + 0, macroblock.blockY[0]);
		CopyBlock8x8(m_currentFrame.y, (macroblockX * 16) + 8, (macroblockY * 16) + 0, macroblock.blockY[1]);
		CopyBlock8x8(m_currentFrame.y, (macroblockX * 16) + 0, (macroblockY * 16) + 8, macroblock.blockY[2]);
		CopyBlock8x8(m_currentFrame.y, (macroblockX * 16) + 8, (macroblockY * 16) + 8, macroblock.blockY[3]);

		CopyBlock8x8(m_currentFrame.cr, macroblockX * 8, macroblockY * 8, macroblock.blockCr);
		CopyBlock8x8(m_currentFrame.cb, macroblockX * 8, macroblockY * 8, macroblock.blockCb);
	}
	else if(decoderState.macroblockType & (MACROBLOCK_MODE_MOTION_FORWARD | MACROBLOCK_MODE_BLOCK_PATTERN))
	{
		//Fetch the data from the previous frame (we need to apply offset from motion vectors here)
		int16 prevBlockY[4][8 * 8];
		int16 prevBlockCr[8 * 8];
		int16 prevBlockCb[8 * 8];

		memset(prevBlockY, 0, sizeof(prevBlockY));
		memset(prevBlockCr, 0, sizeof(prevBlockCr));
		memset(prevBlockCb, 0, sizeof(prevBlockCb));

		int16 motionX = decoderState.forwardMotionVector[0];
		int16 motionY = decoderState.forwardMotionVector[1];

		CopyBlock8x8(prevBlockY[0], (macroblockX * 16) + 0, (macroblockY * 16) + 0, motionX, motionY, m_previousFrame.y);
		CopyBlock8x8(prevBlockY[1], (macroblockX * 16) + 8, (macroblockY * 16) + 0, motionX, motionY, m_previousFrame.y);
		CopyBlock8x8(prevBlockY[2], (macroblockX * 16) + 0, (macroblockY * 16) + 8, motionX, motionY, m_previousFrame.y);
		CopyBlock8x8(prevBlockY[3], (macroblockX * 16) + 8, (macroblockY * 16) + 8, motionX, motionY, m_previousFrame.y);

		motionX /= 2;
		motionY /= 2;

		CopyBlock8x8(prevBlockCr, (macroblockX * 8), (macroblockY * 8), motionX, motionY, m_previousFrame.cr);
		CopyBlock8x8(prevBlockCb, (macroblockX * 8), (macroblockY * 8), motionX, motionY, m_previousFrame.cb);

		int16* prevBlocks[6] = {prevBlockY[0], prevBlockY[1], prevBlockY[2], prevBlockY[3], prevBlockCr, prevBlockCb};
		int16* currBlocks[6] = {macroblock.blockY[0], macroblock.blockY[1], macroblock.blockY[2], macroblock.blockY[3], macroblock.blockCr, macroblock.blockCb};

		for(unsigned int i = 0; i < 6; i++)
		{
			if(!(decoderState.codedBlockPattern & (1 << (5 - i)))) continue;

			int16* dstBlock = prevBlocks[i];
			int16* srcBlock = currBlocks[i];

			for(unsigned int y = 0; y < 8; y++)
			{
				for(unsigned int x = 0; x < 8; x++)
				{
					dstBlock[x + (y * 8)] += srcBlock[x + (y * 8)];
				}
			}
		}

		CopyBlock8x8(m_currentFrame.y, (macroblockX * 16) + 0, (macroblockY * 16) + 0, prevBlockY[0]);
		CopyBlock8x8(m_currentFrame.y, (macroblockX * 16) + 8, (macroblockY * 16) + 0, prevBlockY[1]);
		CopyBlock8x8(m_currentFrame.y, (macroblockX * 16) + 0, (macroblockY * 16) + 8, prevBlockY[2]);
		CopyBlock8x8(m_currentFrame.y, (macroblockX * 16) + 8, (macroblockY * 16) + 8, prevBlockY[3]);
		CopyBlock8x8(m_currentFrame.cr, (macroblockX * 8), (macroblockY * 8), prevBlockCr);
		CopyBlock8x8(m_currentFrame.cb, (macroblockX * 8), (macroblockY * 8), prevBlockCb);
	}
}

void CVideoDecoder::OnPictureDecoded(MPEG_VIDEO_STATE* state)
{
	MACROBLOCK& macroblock(state->macroblock);
	PICTURE_HEADER& pictureHeader(state->pictureHeader);
	SEQUENCE_HEADER& sequenceHeader(state->sequenceHeader);

	if(
	    (m_frame.IsEmpty()) ||
	    (m_frame.GetWidth() != sequenceHeader.horizontalSize) ||
	    (m_frame.GetHeight() != sequenceHeader.verticalSize))
	{
		m_frame = Framework::CBitmap(sequenceHeader.horizontalSize, sequenceHeader.verticalSize, 32);
	}

	auto pixelsY = reinterpret_cast<int16*>(m_currentFrame.y.GetPixels());
	auto pixelsCr = reinterpret_cast<int16*>(m_currentFrame.cr.GetPixels());
	auto pixelsCb = reinterpret_cast<int16*>(m_currentFrame.cb.GetPixels());
	auto pixels = reinterpret_cast<uint32*>(m_frame.GetPixels());

	for(unsigned int j = 0; j < sequenceHeader.verticalSize; j++)
	{
		for(unsigned int i = 0; i < sequenceHeader.horizontalSize; i++)
		{
			float nY = pixelsY[i + (j * sequenceHeader.horizontalSize)];
			float nCb = pixelsCb[i / 2 + (j / 2 * sequenceHeader.horizontalSize / 2)];
			float nCr = pixelsCr[i / 2 + (j / 2 * sequenceHeader.horizontalSize / 2)];

			//float nY = 128;
			//float nCb = 128;
			//float nCr = 128;

			float nR = nY + 1.402f * (nCr - 128);
			float nG = nY - 0.34414f * (nCb - 128) - 0.71414f * (nCr - 128);
			float nB = nY + 1.772f * (nCb - 128);

			if(nR < 0)
			{
				nR = 0;
			}
			if(nR > 255)
			{
				nR = 255;
			}
			if(nG < 0)
			{
				nG = 0;
			}
			if(nG > 255)
			{
				nG = 255;
			}
			if(nB < 0)
			{
				nB = 0;
			}
			if(nB > 255)
			{
				nB = 255;
			}

			pixels[i + (j * sequenceHeader.horizontalSize)] = (((uint8)nB) << 16) | (((uint8)nG) << 8) | (((uint8)nR) << 0);
		}
	}

	if(pictureHeader.pictureCodingType != PICTURE_TYPE_B)
	{
		m_previousFrame = m_currentFrame;
	}

	NewFrame(m_frame);
}

void CVideoDecoder::CopyBlock8x8(Framework::CBitmap& dst, unsigned int x, unsigned int y, const int16* srcPixels)
{
	const int blockWidth = 8;
	const int blockHeight = 8;

	auto dstPixels = reinterpret_cast<int16*>(dst.GetPixels());

	for(unsigned int j = 0; j < blockHeight; j++)
	{
		for(unsigned int i = 0; i < blockWidth; i++)
		{
			unsigned dstX = (i + x);
			unsigned dstY = (j + y);

			if(dstX >= dst.GetWidth()) continue;
			if(dstY >= dst.GetHeight()) continue;

			dstPixels[dstX + (dstY * dst.GetWidth())] = srcPixels[i + (j * blockWidth)];
		}
	}
}

void CVideoDecoder::CopyBlock8x8(int16* dstPixels, unsigned int x, unsigned int y, int16 dx, int16 dy, const Framework::CBitmap& src)
{
	const int blockWidth = 8;
	const int blockHeight = 8;

	int16 baseX = dx >> 1;
	int16 baseY = dy >> 1;

	bool halfX = (dx & 1) != 0;
	bool halfY = (dy & 1) != 0;

	auto srcPixels = reinterpret_cast<int16*>(src.GetPixels());

	if(!halfX && !halfY)
	{
		for(unsigned int j = 0; j < blockHeight; j++)
		{
			for(unsigned int i = 0; i < blockWidth; i++)
			{
				unsigned int srcX = (i + x + baseX);
				unsigned int srcY = (j + y + baseY);

				if(srcX >= src.GetWidth()) continue;
				if(srcY >= src.GetHeight()) continue;

				dstPixels[i + (j * blockWidth)] = srcPixels[srcX + (srcY * src.GetWidth())];
			}
		}
	}
	else if(!halfX && halfY)
	{
		for(unsigned int j = 0; j < blockHeight; j++)
		{
			for(unsigned int i = 0; i < blockWidth; i++)
			{
				unsigned int srcX = (i + x + baseX);
				unsigned int srcY = (j + y + baseY);

				if(srcX >= src.GetWidth()) continue;
				if(srcY >= src.GetHeight()) continue;

				int pixel0 = srcPixels[srcX + ((srcY + 0) * src.GetWidth())];
				int pixel1 = srcPixels[srcX + ((srcY + 1) * src.GetWidth())];

				dstPixels[i + (j * blockWidth)] = static_cast<int16>((pixel0 + pixel1 + 1) / 2);
			}
		}
	}
	else if(halfX && !halfY)
	{
		for(unsigned int j = 0; j < blockHeight; j++)
		{
			for(unsigned int i = 0; i < blockWidth; i++)
			{
				unsigned int srcX = (i + x + baseX);
				unsigned int srcY = (j + y + baseY);

				if(srcX >= src.GetWidth()) continue;
				if(srcY >= src.GetHeight()) continue;

				int pixel0 = srcPixels[srcX + 0 + (srcY * src.GetWidth())];
				int pixel1 = srcPixels[srcX + 1 + (srcY * src.GetWidth())];

				dstPixels[i + (j * blockWidth)] = static_cast<int16>((pixel0 + pixel1 + 1) / 2);
			}
		}
	}
	else if(halfX && halfY)
	{
		for(unsigned int j = 0; j < blockHeight; j++)
		{
			for(unsigned int i = 0; i < blockWidth; i++)
			{
				unsigned int srcX = (i + x + baseX);
				unsigned int srcY = (j + y + baseY);

				if(srcX >= src.GetWidth()) continue;
				if(srcY >= src.GetHeight()) continue;

				int pixel0 = srcPixels[srcX + 0 + ((srcY + 0) * src.GetWidth())];
				int pixel1 = srcPixels[srcX + 1 + ((srcY + 0) * src.GetWidth())];
				int pixel2 = srcPixels[srcX + 0 + ((srcY + 1) * src.GetWidth())];
				int pixel3 = srcPixels[srcX + 1 + ((srcY + 1) * src.GetWidth())];

				dstPixels[i + (j * blockWidth)] = static_cast<int16>((pixel0 + pixel1 + pixel2 + pixel3 + 2) / 4);
			}
		}
	}
}

void CVideoDecoder::CopyMacroblock(Framework::CBitmap& dst, unsigned int x, unsigned int y, const uint32* srcPixels)
{
	const int blockWidth = 16;
	const int blockHeight = 16;

	uint32* dstPixels = reinterpret_cast<uint32*>(dst.GetPixels());

	for(unsigned int j = 0; j < blockHeight; j++)
	{
		for(unsigned int i = 0; i < blockWidth; i++)
		{
			unsigned dstX = (i + x);
			unsigned dstY = (j + y);

			if(dstX >= dst.GetWidth()) continue;
			if(dstY >= dst.GetHeight()) continue;

			dstPixels[dstX + (dstY * dst.GetWidth())] = srcPixels[i + (j * blockWidth)];
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
// FRAME

CVideoDecoder::FRAME::FRAME()
{
}

CVideoDecoder::FRAME::FRAME(unsigned int width, unsigned int height)
{
	y = Framework::CBitmap(width, height, 16);
	cr = Framework::CBitmap(width / 2, height / 2, 16);
	cb = Framework::CBitmap(width / 2, height / 2, 16);
}

CVideoDecoder::FRAME::~FRAME()
{
}

CVideoDecoder::FRAME& CVideoDecoder::FRAME::operator=(FRAME&& src)
{
	y = std::move(src.y);
	cr = std::move(src.cr);
	cb = std::move(src.cb);
	return (*this);
}

bool CVideoDecoder::FRAME::IsEmpty() const
{
	return y.IsEmpty();
}

unsigned int CVideoDecoder::FRAME::GetWidth() const
{
	return y.GetWidth();
}

unsigned int CVideoDecoder::FRAME::GetHeight() const
{
	return y.GetHeight();
}
