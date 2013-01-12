#include <assert.h>
#include "MainWindow.h"
#include "StreamBitStream.h"
#include "XaSoundStreamDecoder.h"
#include "Ac3SoundStreamDecoder.h"
#include "StdStream.h"
#include "MemStream.h"
#include "MpegVideoState.h"
#include "ProgramStreamDecoder.h"
#include "VideoStream_Decoder.h"
#include "placeholder_def.h"
#include "Bitmap.h"
#include "BMP.h"
#include <sys/stat.h>
#include <functional>
#include <thread>

using namespace Framework;

typedef std::tr1::function<void (unsigned int, unsigned int, uint8*)> SetImageFunction;

Framework::CBitmap*							g_bitmap(NULL);
CMemStream									g_videoStream;
CMemStream									g_soundStream;
SetImageFunction							g_setImage;
ProgramStreamDecoder::PRIVATE_STREAM1_INFO	g_soundStreamInfo;

//-----------------------------------------------------------
//Video Stuff
//-----------------------------------------------------------

void DumpPicture()
{
//	char sFilename[256];
//	
//	for(unsigned int i = 0; i < UINT_MAX; i++)
//	{
//	    struct stat Stat;
//		sprintf(sFilename, "pic_%0.8d.bmp", i);
//		if(stat(sFilename, &Stat) == -1) break;
//	}
//	CBMP::ToBMP(g_bitmap, &CStdStream(fopen(sFilename, "wb")));
}

void CopyBlock(unsigned int x, unsigned int y, uint32* src)
{
	const int blockWidth = 16;
	const int blockHeight = 16;
	
	assert(g_bitmap != NULL);
	uint32* dst = reinterpret_cast<uint32*>(g_bitmap->GetPixels());
	
	for(unsigned int j = 0; j < blockHeight; j++)
	{
		for(unsigned int i = 0; i < blockWidth; i++)
		{
			unsigned dstX = (i + x);
			unsigned dstY = (j + y);
			
			if(dstX >= g_bitmap->GetWidth()) continue;
			if(dstY >= g_bitmap->GetHeight()) continue;
			
			dst[dstX + (dstY * g_bitmap->GetWidth())] = src[i + (j * blockWidth)];
		}
	}
}

void OnMacroblockDecoded(MPEG_VIDEO_STATE* state)
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
	   (g_bitmap == NULL) || 
	   (g_bitmap->GetWidth() != sequenceHeader.horizontalSize) || 
	   (g_bitmap->GetHeight() != sequenceHeader.verticalSize))
	{
		if(g_bitmap != NULL)
		{
			delete g_bitmap;
			g_bitmap = NULL;
		}
		
		g_bitmap = new Framework::CBitmap(sequenceHeader.horizontalSize, sequenceHeader.verticalSize, 32);
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
	CopyBlock(macroblockX * 16, macroblockY * 16, pixels);
	
	if(decoderState.currentMbAddress == sequenceHeader.macroblockMaxAddress - 1)
	{
#ifdef _DEBUG
		printf("Dumping image.\r\n");
#endif
		g_setImage(g_bitmap->GetWidth(), g_bitmap->GetHeight(), g_bitmap->GetPixels());
//		DumpPicture();
	}
}

bool VideoStreamHandler(CBitStream& stream, uint32 size)
{
	assert(g_videoStream.IsEOF());
	g_videoStream.ResetBuffer();
	for(unsigned int i = 0; i < size; i++)
	{
		g_videoStream.Write8(static_cast<uint8>(stream.GetBits_MSBF(8)));
	}
	g_videoStream.Seek(0, Framework::STREAM_SEEK_SET);
	return false;
}

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

void DecoderThreadProc(const char* path)
{
	CStdStream stream(path, "rb");
	stream.Seek(0, Framework::STREAM_SEEK_END);
	uint64 fileSize = stream.Tell();
	stream.Seek(0, Framework::STREAM_SEEK_SET);

	CStreamBitStream bitStream(stream);
	CStreamBitStream videoBitStream(g_videoStream);

	ProgramStreamDecoder programStreamDecoder;
	programStreamDecoder.RegisterPrivateStream1Handler(std::tr1::bind(&SoundStreamHandler, PLACEHOLDER_1, PLACEHOLDER_2, PLACEHOLDER_3));
	programStreamDecoder.RegisterVideoStreamHandler(std::tr1::bind(&VideoStreamHandler, PLACEHOLDER_1, PLACEHOLDER_2));

	MPEG_VIDEO_STATE videoState;
	VideoStream::Decoder videoStreamDecoder;
	videoStreamDecoder.InitializeState(&videoState);
	videoStreamDecoder.Reset();
	videoStreamDecoder.RegisterOnMacroblockDecodedHandler(std::tr1::bind(&OnMacroblockDecoded, PLACEHOLDER_1));
	
	while(1)
	{
		if(!g_videoStream.IsEOF())
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

int main(int argc, char** argv)
{
	if(argc < 2)
	{
		return 0;
	}

	CMainWindow mainWindow;

	g_setImage = std::bind(&CMainWindow::SetImage, &mainWindow, PLACEHOLDER_1, PLACEHOLDER_2, PLACEHOLDER_3);

	std::thread decoderThread(std::bind(&DecoderThreadProc, argv[1]));
	mainWindow.Center();
	mainWindow.Show(SW_SHOW);

	Framework::Win32::CWindow::StdMsgLoop(mainWindow);

	return 0;
}
