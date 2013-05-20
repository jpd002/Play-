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
