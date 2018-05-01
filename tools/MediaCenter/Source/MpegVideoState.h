#ifndef _MPEGVIDEOSTATE_H_
#define _MPEGVIDEOSTATE_H_

#include "Types.h"

enum PICTURE_TYPE
{
	PICTURE_TYPE_I = 1,
	PICTURE_TYPE_P = 2,
	PICTURE_TYPE_B = 3,
};

enum CHROMA420_TYPE
{
	CHROMA_INVALID = 0,
	CHROMA_420 = 1,
	CHROMA_422 = 2,
	CHROMA_444 = 3,
};

enum MACROBLOCK_MODES
{
	MACROBLOCK_MODE_INTRA = 0x01,
	MACROBLOCK_MODE_BLOCK_PATTERN = 0x02,
	MACROBLOCK_MODE_MOTION_BACKWARD = 0x04,
	MACROBLOCK_MODE_MOTION_FORWARD = 0x08,
	MACROBLOCK_MODE_QUANT = 0x10,
};

enum MOTION_TYPES
{
	MOTION_FIELD = 1,
	MOTION_FRAME = 2,
	MOTION_DMV = 3,
};

struct PICTURE_HEADER
{
	uint16 temporalReference;
	uint8 pictureCodingType;
	uint16 vbvDelay;
	uint32 number;

	uint8 pelForwardVector;
	uint8 forwardFCode;

	uint8 pelBackwardVector;
	uint8 backwardFCode;
};

struct PICTURE_CODING_EXTENSION
{
	uint8 fcode00;
	uint8 fcode01;
	uint8 fcode10;
	uint8 fcode11;
	uint8 intraDcPrecision;
	uint8 pictureStructure;
	uint8 topFieldFirst;
	uint8 framePredFrameDct;
	uint8 concealmentMotionVectors;
	uint8 quantizerScaleType;
	uint8 intraVlcFormat;
	uint8 alternateScan;
	uint8 repeatFirstField;
	uint8 chroma420Type;
	uint8 progressiveFrame;
	uint8 compositeDisplayFlag;

	uint8 vaxis;
	uint8 fieldSequence;
	uint8 subCarrier;
	uint8 burstAmplitude;
	uint8 subCarrierPhase;
};

struct SEQUENCE_HEADER
{
	uint8 isMpeg2;

	uint16 horizontalSize;
	uint16 verticalSize;
	uint8 aspectRatio;
	uint8 frameRate;
	uint32 bitRate;
	uint16 vbvBufferSize;
	uint8 constrainedParameterFlag;
	uint8 loadIntraQuantiserMatrix;
	uint8 loadNonIntraQuantiserMatrix;
	uint8 intraQuantiserMatrix[0x40];
	uint8 nonIntraQuantiserMatrix[0x40];

	uint16 macroblockWidth;
	uint16 macroblockHeight;
	uint32 macroblockMaxAddress;
};

struct SEQUENCE_EXTENSION
{
	uint8 profileAndLevel;
	uint8 progressiveSequence;
	uint8 chromaFormat;
	uint8 horizontalSizeExtension;
	uint8 verticalSizeExtension;
	uint16 bitRateExtension;
	uint8 vbvBufferSizeExtension;
	uint8 lowDelay;
	uint8 frameRateExtensionN;
	uint8 frameRateExtensionD;
};

struct GOP_HEADER
{
	uint32 timeCode;
	uint8 closedGop;
	uint8 brokenLink;
};

struct BLOCK_DECODER_STATE
{
	bool resetDc;
	int16 dcPredictor[3];
	uint8 macroblockType;
	uint32 currentMbAddress;
	uint32 mbIncrement;
	uint8 dctType;
	uint8 quantizerScaleCode;
	uint8 codedBlockPattern;
	int16 dcDifferential;
	uint8 motionVectorCount;
	uint8 motionType;
	int16 forwardMotionVector[2];
	int16 backwardMotionVector[2];
	int16 motionCode[2];
	uint16 motionResidual[2];
};

struct MACROBLOCK
{
	int16 blockY[4][64];
	int16 blockCb[64];
	int16 blockCr[64];
};

struct MPEG_VIDEO_STATE
{
	SEQUENCE_HEADER sequenceHeader;
	SEQUENCE_EXTENSION sequenceExtension;
	GOP_HEADER gopHeader;
	PICTURE_HEADER pictureHeader;
	PICTURE_CODING_EXTENSION pictureCodingExtension;
	BLOCK_DECODER_STATE blockDecoderState;
	MACROBLOCK macroblock;
};

#endif
