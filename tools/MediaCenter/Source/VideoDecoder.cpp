#include "VideoDecoder.h"
#include "StreamBitStream.h"
#include "StdStream.h"
#include "MemStream.h"
#include "ProgramStreamDecoder.h"
#include "VideoStream_Decoder.h"
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
	m_decoderThread = std::thread([=] () { DecoderThreadProc(path); });
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
	Framework::CStdStream stream(path.c_str(), "rb");
	stream.Seek(0, Framework::STREAM_SEEK_END);
	uint64 fileSize = stream.Tell();
	stream.Seek(0, Framework::STREAM_SEEK_SET);

	Framework::CStreamBitStream bitStream(stream);

	Framework::CMemStream videoStream;
	Framework::CStreamBitStream videoBitStream(videoStream);

	ProgramStreamDecoder programStreamDecoder;
//	programStreamDecoder.RegisterPrivateStream1Handler(std::tr1::bind(&SoundStreamHandler, PLACEHOLDER_1, PLACEHOLDER_2, PLACEHOLDER_3));
	programStreamDecoder.RegisterVideoStreamHandler(
		[&] (Framework::CBitStream& stream, uint32 size)
		{
			assert(videoStream.IsEOF());
			videoStream.ResetBuffer();
			for(unsigned int i = 0; i < size; i++)
			{
				videoStream.Write8(static_cast<uint8>(stream.GetBits_MSBF(8)));
			}
			videoStream.Seek(0, Framework::STREAM_SEEK_SET);
			return false;
		});
		

	MPEG_VIDEO_STATE videoState;
	VideoStream::Decoder videoStreamDecoder;
	videoStreamDecoder.InitializeState(&videoState);
	videoStreamDecoder.Reset();
	videoStreamDecoder.RegisterOnMacroblockDecodedHandler([&] (MPEG_VIDEO_STATE* state) { OnMacroblockDecoded(state); });
	
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
		ProgramStreamDecoder::STATUS status = programStreamDecoder.Decode(bitStream);
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
	uint32 pixels[blockWidth * blockHeight];
	
	if(pictureHeader.pictureCodingType != PICTURE_TYPE_I) return;

	if(
		(m_frame.IsEmpty()) ||
		(m_frame.GetWidth() != sequenceHeader.horizontalSize) || 
		(m_frame.GetHeight() != sequenceHeader.verticalSize))
	{
		m_frame = Framework::CBitmap(sequenceHeader.horizontalSize, sequenceHeader.verticalSize, 32);
	}

	for(unsigned int j = 0; j < blockHeight; j++)
	{
		for(unsigned int i = 0; i < blockWidth; i++)
		{
			int lumiIndexX = i % 8;
			int lumiIndexY = j % 8;
			int lumiBlockIndex = (i / 8) + ((j / 8) * 2);
			int chromiIndexX = i / 2;
			int chromiIndexY = j / 2;
			
			assert(lumiIndexX < 8);
			assert(lumiIndexY < 8);
			assert(chromiIndexX < 8);
			assert(chromiIndexY < 8);
			
			float nY  = macroblock.blockY[lumiBlockIndex][lumiIndexX + (lumiIndexY * 8)];
			float nCb = macroblock.blockCb[chromiIndexX + (chromiIndexY * 8)];
			float nCr = macroblock.blockCr[chromiIndexX + (chromiIndexY * 8)];
			
			//float nY = 128;
			//float nCb = 128;
			//float nCr = 128;
			
			float nR = nY							+ 1.402f	* (nCr - 128);
			float nG = nY - 0.34414f * (nCb - 128)	- 0.71414f	* (nCr - 128);
			float nB = nY + 1.772f	 * (nCb - 128);
			
			if(nR < 0) { nR = 0; } if(nR > 255) { nR = 255; }
			if(nG < 0) { nG = 0; } if(nG > 255) { nG = 255; }
			if(nB < 0) { nB = 0; } if(nB > 255) { nB = 255; }
			
			pixels[i + (j * blockWidth)] = (((uint8)nB) << 16) | (((uint8)nG) << 8) | (((uint8)nR) << 0);	
		}
	}
	
	unsigned int macroblockX = decoderState.currentMbAddress % sequenceHeader.macroblockWidth;
	unsigned int macroblockY = decoderState.currentMbAddress / sequenceHeader.macroblockWidth;
	CopyBlock(m_frame, macroblockX * 16, macroblockY * 16, pixels);
	
	if(decoderState.currentMbAddress == sequenceHeader.macroblockMaxAddress - 1)
	{
		NewFrame(m_frame);
	}
}

void CVideoDecoder::CopyBlock(Framework::CBitmap& dst, unsigned int x, unsigned int y, uint32* srcPixels)
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
