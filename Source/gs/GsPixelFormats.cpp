#include "GsPixelFormats.h"

const int CGsPixelFormats::STORAGEPSMCT32::m_nBlockSwizzleTable[4][8] =
{
	{	0,	1,	4,	5,	16,	17,	20,	21	},
	{	2,	3,	6,	7,	18,	19,	22,	23	},
	{	8,	9,	12,	13,	24,	25,	28,	29	},
	{	10,	11,	14,	15,	26,	27,	30,	31	},
};

const int CGsPixelFormats::STORAGEPSMCT32::m_nColumnSwizzleTable[2][8] =
{
	{	0,	1,	4,	5,	8,	9,	12,	13,	},
	{	2,	3,	6,	7,	10,	11,	14,	15,	},
};

const int CGsPixelFormats::STORAGEPSMCT16::m_nBlockSwizzleTable[8][4] =
{
	{	0,	2,	8,	10,	},
	{	1,	3,	9,	11,	},
	{	4,	6,	12,	14,	},
	{	5,	7,	13,	15,	},
	{	16,	18,	24,	26,	},
	{	17,	19,	25,	27,	},
	{	20,	22,	28,	30,	},
	{	21,	23,	29,	31,	},
};

const int CGsPixelFormats::STORAGEPSMCT16::m_nColumnSwizzleTable[2][16] =
{
	{	0,	2,	8,	10,	16,	18,	24,	26,	1,	3,	9,	11,	17,	19,	25,	27,	},
	{	4,	6,	12,	14,	20,	22,	28,	30,	5,	7,	13,	15,	21,	23,	29,	31,	},
};

const int CGsPixelFormats::STORAGEPSMCT16S::m_nBlockSwizzleTable[8][4] =
{
	{	0,	2,	16,	18,	},
	{	1,	3,	17,	19,	},
	{	8,	10,	24,	26,	},
	{	9,	11,	25,	27,	},
	{	4,	6,	20,	22,	},
	{	5,	7,	21,	23,	},
	{	12,	14,	28,	30,	},
	{	13,	15,	29,	31,	},
};

const int CGsPixelFormats::STORAGEPSMCT16S::m_nColumnSwizzleTable[2][16] =
{
	{	0,	2,	8,	10,	16,	18,	24,	26,	1,	3,	9,	11,	17,	19,	25,	27,	},
	{	4,	6,	12,	14,	20,	22,	28,	30,	5,	7,	13,	15,	21,	23,	29,	31,	},
};

const int CGsPixelFormats::STORAGEPSMT8::m_nBlockSwizzleTable[4][8] =
{
	{	0,	1,	4,	5,	16,	17,	20,	21	},
	{	2,	3,	6,	7,	18,	19,	22,	23	},
	{	8,	9,	12,	13,	24,	25,	28,	29	},
	{	10,	11,	14,	15,	26,	27,	30,	31	},
};

const int CGsPixelFormats::STORAGEPSMT8::m_nColumnWordTable[2][2][8] =
{
	{
		{	0,	1,	4,	5,	8,	9,	12,	13,	},
		{	2,	3,	6,	7,	10,	11,	14,	15,	},
	},
	{
		{	8,	9,	12,	13,	0,	1,	4,	5,	},
		{	10,	11,	14,	15,	2,	3,	6,	7,	},
	},
};

const int CGsPixelFormats::STORAGEPSMT4::m_nBlockSwizzleTable[8][4] =
{
	{	0,	2,	8,	10,	},
	{	1,	3,	9,	11,	},
	{	4,	6,	12,	14,	},
	{	5,	7,	13,	15,	},
	{	16,	18,	24,	26,	},
	{	17,	19,	25,	27,	},
	{	20,	22,	28,	30,	},
	{	21,	23,	29,	31,	}
};

const int CGsPixelFormats::STORAGEPSMT4::m_nColumnWordTable[2][2][8] =
{
	{
		{	0,	1,	4,	5,	8,	9,	12,	13,	},
		{	2,	3,	6,	7,	10,	11,	14,	15,	},
	},
	{
		{	8,	9,	12,	13,	0,	1,	4,	5,	},
		{	10,	11,	14,	15,	2,	3,	6,	7,	},
	},
};

unsigned int CGsPixelFormats::GetPsmPixelSize(unsigned int psm)
{
	switch(psm)
	{
	case CGSHandler::PSMCT32:
	case CGSHandler::PSMT4HH:
	case CGSHandler::PSMT4HL:
	case CGSHandler::PSMT8H:
		return 32;
		break;
	case CGSHandler::PSMCT24:
	case CGSHandler::PSMCT24_UNK:
		return 24;
		break;
	case CGSHandler::PSMCT16:
	case CGSHandler::PSMCT16S:
		return 16;
		break;
	case CGSHandler::PSMT8:
		return 8;
		break;
	case CGSHandler::PSMT4:
		return 4;
		break;
	default:
		assert(0);
		return 0;
		break;
	}
}

std::pair<uint32, uint32> CGsPixelFormats::GetPsmPageSize(unsigned int psm)
{
	switch(psm)
	{
	case CGSHandler::PSMCT32:
	case CGSHandler::PSMCT24:
	case CGSHandler::PSMCT24_UNK:
	case CGSHandler::PSMT8H:
	case CGSHandler::PSMT4HH:
	case CGSHandler::PSMT4HL:
	case CGSHandler::PSMZ32:
	case CGSHandler::PSMZ24:
		return std::make_pair(CGsPixelFormats::STORAGEPSMCT32::PAGEWIDTH, CGsPixelFormats::STORAGEPSMCT32::PAGEHEIGHT);
	case CGSHandler::PSMCT16:
		return std::make_pair(CGsPixelFormats::STORAGEPSMCT16::PAGEWIDTH, CGsPixelFormats::STORAGEPSMCT16::PAGEHEIGHT);
	case CGSHandler::PSMCT16S:
		return std::make_pair(CGsPixelFormats::STORAGEPSMCT16S::PAGEWIDTH, CGsPixelFormats::STORAGEPSMCT16S::PAGEHEIGHT);
	case CGSHandler::PSMT8:
		return std::make_pair(CGsPixelFormats::STORAGEPSMT8::PAGEWIDTH, CGsPixelFormats::STORAGEPSMT8::PAGEHEIGHT);
	case CGSHandler::PSMT4:
		return std::make_pair(CGsPixelFormats::STORAGEPSMT4::PAGEWIDTH, CGsPixelFormats::STORAGEPSMT4::PAGEHEIGHT);
	default:
		assert(0);
		//Return a dummy value to prevent crashing
		return std::make_pair(CGsPixelFormats::STORAGEPSMCT32::PAGEWIDTH, CGsPixelFormats::STORAGEPSMCT32::PAGEHEIGHT);
		break;
	}
}

bool CGsPixelFormats::IsPsmIDTEX(unsigned int psm)
{
	return IsPsmIDTEX4(psm) || IsPsmIDTEX8(psm);
}

bool CGsPixelFormats::IsPsmIDTEX4(unsigned int psm)
{
	return psm == CGSHandler::PSMT4 || psm == CGSHandler::PSMT4HH || psm == CGSHandler::PSMT4HL;
}

bool CGsPixelFormats::IsPsmIDTEX8(unsigned int psm)
{
	return psm == CGSHandler::PSMT8 || psm == CGSHandler::PSMT8H;
}
