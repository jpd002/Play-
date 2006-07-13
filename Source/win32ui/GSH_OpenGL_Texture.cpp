#include <assert.h>
#include <sys/stat.h>
#include <limits.h>
#include "GSH_OpenGL.h"
#include "StdStream.h"
#include "BMP.h"

using namespace Framework;

/////////////////////////////////////////////////////////////
// Texture Loading
/////////////////////////////////////////////////////////////

unsigned int CGSH_OpenGL::LoadTexture(GSTEX0* pReg0, GSTEX1* pReg1, CLAMP* pClamp)
{
	unsigned int nTexture;
	int nMagFilter, nMinFilter;
	int nWrapS, nWrapT;
	uint32 nPointer, nWidth, nHeight;
	GSTEXA TexA;

	nPointer	= pReg0->GetBufPtr();
	nWidth		= pReg0->GetWidth();
	nHeight		= pReg0->GetHeight();

	//return NULL;

	//Set the right texture function
	switch(pReg0->nFunction)
	{
	case 0:
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		break;
	case 1:
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		break;
	default:
		assert(0);
		break;
	}

	if(m_pProgram != NULL)
	{
		if((pClamp->nWMS > 1) || (pClamp->nWMT > 1))
		{
			//We gotta use the shader

			glUseProgram((*m_pProgram));

			m_pProgram->SetUniformi("g_nClamp",	pClamp->nWMS - 1,	pClamp->nWMT - 1);
			m_pProgram->SetUniformi("g_nMin",	pClamp->GetMinU(),	pClamp->GetMinV());
			m_pProgram->SetUniformi("g_nMax",	pClamp->GetMaxU(),	pClamp->GetMaxV());
			m_pProgram->SetUniformf("g_nSize",	(float)nWidth,		(float)nHeight);
		}
		else
		{
			glUseProgram(NULL);
		}
	}

	nTexture = TexCache_Search(pReg0);
	if(nTexture != 0) return nTexture;

	DECODE_TEXA(m_nReg[GS_REG_TEXA], TexA);

	glGenTextures(1, &nTexture);

	glBindTexture(GL_TEXTURE_2D, nTexture);

	if(pReg1->nMagFilter == 0)
	{
		nMagFilter = GL_NEAREST;
	}
	else
	{
		nMagFilter = GL_LINEAR;
	}

	switch(pReg1->nMinFilter)
	{
	case 0:
		nMinFilter = GL_NEAREST;
		break;
	case 1:
		nMinFilter = GL_LINEAR;
		break;
	default:
		assert(0);
		break;
	}

	if(m_nForceBilinearTextures)
	{
		nMagFilter = GL_LINEAR;
		nMinFilter = GL_LINEAR;
	}

	switch(pClamp->nWMS)
	{
	case 0:
	case 2:
	case 3:
		nWrapS = GL_REPEAT;
		break;
	case 1:
		nWrapS = GL_CLAMP_TO_EDGE;
		break;
	}

	switch(pClamp->nWMT)
	{
	case 0:
	case 2:
	case 3:
		nWrapT = GL_REPEAT;
		break;
	case 1:
		nWrapT = GL_CLAMP_TO_EDGE;
		break;
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, nMagFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, nMinFilter);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, nWrapS);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, nWrapT);

	//Fix the pitch of the image
	glPixelStorei(GL_UNPACK_ROW_LENGTH, pReg0->GetBufWidth());

	switch(pReg0->nPsm)
	{
	case PSMCT32:
		TexUploader_Psm32(pReg0, &TexA);
		//glPixelTransferf(GL_ALPHA_SCALE, 2.0f);
		//glTexImage2D(GL_TEXTURE_2D, 0, 4, nWidth, nHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_pRAM + nPointer);
		break;
	case PSMCT16:
	case PSMCT16S:
		((this)->*(m_pTexUploader_Psm16))(pReg0, &TexA);
		break;
	case PSMT8:
		((this)->*(m_pTexUploader_Psm8))(pReg0, &TexA);
		break;
	case PSMT4:
		TexUploader_Psm4_Cvt(pReg0, &TexA);
		break;
	case PSMT4HL:
		TexUploader_Psm4H_Cvt<24>(pReg0, &TexA);
		break;
	case PSMT4HH:
		TexUploader_Psm4H_Cvt<28>(pReg0, &TexA);
		break;
	case PSMT8H:
		TexUploader_Psm8H_Cvt(pReg0, &TexA);
		break;
	default:
		assert(0);
		break;
	}

	if(pReg0->nPsm == PSMT8)
	{
		//DumpTexture(nWidth, nHeight);
	}

	//
	/*
	glColor4d(1.0, 1.0, 1.0, 1.0);

	glBegin(GL_QUADS);

	glTexCoord2d(0.0,	0.0);
	glVertex2d(0.0,		0.0);

	glTexCoord2d(0.0,	1.0);
	glVertex2d(0.0,		nHeight);

	glTexCoord2d(1.0,	1.0);
	glVertex2d(nWidth,	nHeight);

	glTexCoord2d(1.0,	0.0);
	glVertex2d(nWidth,	0.0);

	glEnd();

	Flip();
	*/
	//

	glBindTexture(GL_TEXTURE_2D, 0);

	TexCache_Insert(pReg0, nTexture);

	return nTexture;
}

void CGSH_OpenGL::DumpTexture(unsigned int nWidth, unsigned int nHeight)
{
	CBitmap* pBitmap;
	CStdStream* pStream;
	char sFilename[256];
	unsigned int i;
	struct _stat Stat;

	for(i = 0; i < UINT_MAX; i++)
	{
		sprintf(sFilename, "./textures/tex_%0.8X.bmp", i);
		if(_stat(sFilename, &Stat) == -1) break;
	}

	pStream = new CStdStream(fopen(sFilename, "wb"));
	pBitmap = new CBitmap(nWidth, nHeight, 32);

	glGetTexImage(GL_TEXTURE_2D, 0, GL_BGRA, GL_UNSIGNED_BYTE, pBitmap->GetData());
	CBMP::ToBMP(pBitmap, pStream);

	delete pBitmap;
	delete pStream;
}

void CGSH_OpenGL::ReadCLUT4(GSTEX0* pTex0)
{
	//assert(pTex0->nCSA == 0);
	//assert(pTex0->nCLD == 1);

	if(pTex0->nCLD != 1)
	{
		memmove(m_pCLUT32, m_pCLUT32 + (pTex0->nCSA * 16), 0x40);
	}
	else
	{
		if(pTex0->nCSM == 1)
		{
			TEXCLUT* pTexClut;
			unsigned int nOffsetX, nOffsetY;
			uint16* pDst;

			assert(pTex0->nCPSM == PSMCT16);

			pTexClut = GetTexClut();

			CPixelIndexorPSMCT16 Indexor(m_pRAM, pTex0->GetCLUTPtr(), pTexClut->nCBW);
			nOffsetX = pTexClut->GetOffsetU();
			nOffsetY = pTexClut->GetOffsetV();
			pDst = m_pCLUT16;

			for(unsigned int i = 0; i < 0x10; i++)
			{
				(*pDst++) = Indexor.GetPixel(nOffsetX + i, nOffsetY);
			}
		}
		else
		{
			if(pTex0->nCPSM == PSMCT32)
			{
				CPixelIndexorPSMCT32 Indexor(m_pRAM, pTex0->GetCLUTPtr(), 1);
				uint32* pDst;

				pDst = m_pCLUT32;

				for(unsigned int j = 0; j < 2; j++)
				{
					for(unsigned int i = 0; i < 8; i++)
					{
						(*pDst++) = Indexor.GetPixel(i, j);
					}
				}
			}
			else if(pTex0->nCPSM == PSMCT16)
			{
				CPixelIndexorPSMCT16 Indexor(m_pRAM, pTex0->GetCLUTPtr(), 1);
				uint16* pDst;

				pDst = m_pCLUT16;

				for(unsigned int j = 0; j < 2; j++)
				{
					for(unsigned int i = 0; i < 8; i++)
					{
						(*pDst++) = Indexor.GetPixel(i, j);
					}
				}
			}
			else
			{
				assert(0);
			}
		}
	}
}

void CGSH_OpenGL::ReadCLUT8(GSTEX0* pTex0)
{
	uint8 nIndex;

	assert(pTex0->nCSA == 0);
//	assert(pTex0->nCLD == 1);
	assert(pTex0->nCSM == 0);

	if(pTex0->nCPSM == PSMCT32)
	{
		CPixelIndexorPSMCT32 Indexor(m_pRAM, pTex0->GetCLUTPtr(), 1);

		for(unsigned int j = 0; j < 16; j++)
		{
			for(unsigned int i = 0; i < 16; i++)
			{
				nIndex = i + (j * 16);
				nIndex = (nIndex & ~0x18) | ((nIndex & 0x08) << 1) | ((nIndex & 0x10) >> 1);
				m_pCLUT32[nIndex] = Indexor.GetPixel(i, j);
			}
		}
	}
	else if(pTex0->nCPSM == PSMCT16)
	{
		CPixelIndexorPSMCT16 Indexor(m_pRAM, pTex0->GetCLUTPtr(), 1);

		for(unsigned int j = 0; j < 16; j++)
		{
			for(unsigned int i = 0; i < 16; i++)
			{
				nIndex = i + (j * 16);
				nIndex = (nIndex & ~0x18) | ((nIndex & 0x08) << 1) | ((nIndex & 0x10) >> 1);
				m_pCLUT16[nIndex] = Indexor.GetPixel(i, j);
			}
		}
	}
	else
	{
		assert(0);
	}
}

void CGSH_OpenGL::TexUploader_Psm32(GSTEX0* pReg0, GSTEXA* pTexA)
{
	unsigned int nWidth, nHeight, nDstPitch, i, j;
	uint32 nPointer;
	uint32* pDst;

	nPointer	= pReg0->GetBufPtr();
	nWidth		= pReg0->GetWidth();
	nHeight		= pReg0->GetHeight();
	nDstPitch	= nWidth;
	pDst		= (uint32*)m_pCvtBuffer;

	CPixelIndexorPSMCT32 Indexor(m_pRAM, nPointer, pReg0->nBufWidth);

	for(j = 0; j < nHeight; j++)
	{
		for(i = 0; i < nWidth; i++)
		{
			pDst[i] = Indexor.GetPixel(i, j);
		}

		pDst += nDstPitch;
	}

	glPixelTransferf(GL_ALPHA_SCALE, 2.0f);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, nWidth);
	glTexImage2D(GL_TEXTURE_2D, 0, 4, nWidth, nHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_pCvtBuffer);
}

void CGSH_OpenGL::TexUploader_Psm8_Hw(GSTEX0* pReg0, GSTEXA* pTexA)
{
	unsigned int nWidth, nHeight;
	uint32 nPointer;
	uint32 nPalette[0x100];
	uint32 i, j, nPos;
	uint8 nAlpha;

	nPointer	= pReg0->GetBufPtr();
	nWidth		= pReg0->GetWidth();
	nHeight		= pReg0->GetHeight();

	assert(pReg0->nCLD == 1);
	assert(pReg0->nCSM == 0);
	assert(pReg0->nCSA == 0);

	//Load CLUT
	//----------------------------------------

	if(pReg0->nCPSM == PSMCT32)
	{
		uint32* pCLUT;
		pCLUT = (uint32*)(m_pRAM + pReg0->GetCLUTPtr());

		for(i = 0; i < 16; i++)
		{
			nPos = ((i & 1) * 8) + ((i / 2) * 0x20);

			for(j = 0; j < 8; j++)
			{
				nPalette[nPos + j] = pCLUT[j];

				nAlpha = *((uint8*)&nPalette[nPos + j] + 3);
				nAlpha = (uint8)min((unsigned int)nAlpha * 2, 0xFF);
				*((uint8*)&nPalette[nPos + j] + 3) = nAlpha;
			}
			pCLUT += 0x08;

			for(j = 0; j < 8; j++)
			{
				nPalette[nPos + j + 0x10] = pCLUT[j];

				nAlpha = *((uint8*)&nPalette[nPos + j + 0x10] + 3);
				nAlpha = (uint8)min((unsigned int)nAlpha * 2, 0xFF);
				*((uint8*)&nPalette[nPos + j + 0x10] + 3) = nAlpha;
			}
			pCLUT += 0x08;

			pCLUT += 0x30;
		}
	}
	else if(pReg0->nCPSM == PSMCT16)
	{
		uint16* pCLUT;
		uint32 nColor, nLine;

		pCLUT = (uint16*)(m_pRAM + pReg0->GetCLUTPtr());

		for(i = 0; i < 256; i++)
		{
			nColor = (i & ~0x18) | ((i & 0x08) << 1) | ((i & 0x10) >> 1);
			nLine = (nColor & 0xF0) >> 4;
			nColor = pCLUT[(nColor & 0x0F) + (nLine * 0x40)];
			nPalette[i] = RGBA16ToRGBA32((uint16)nColor);
		}
	}
	else
	{
		assert(0);
	}

	glColorTableEXT(GL_TEXTURE_2D, GL_RGBA, 256, GL_RGBA, GL_UNSIGNED_BYTE, nPalette);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_COLOR_INDEX8_EXT, nWidth, nHeight, 0, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, m_pRAM + nPointer);
}

template <> uint8 CGSHandler::CPixelIndexor<CGSHandler::STORAGEPSMT8>::GetPixel(unsigned int nX, unsigned int nY)
{
	typedef CGSHandler::STORAGEPSMT8 Storage;

	unsigned int nByte, nTable;

	static unsigned int nColSwizzleTable[2][2][8] =
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

	uint32 nPageNum, nBlockNum, nColumnNum, nOffset;

	nPageNum = (nX / Storage::PAGEWIDTH) + (nY / Storage::PAGEHEIGHT) * (m_nWidth * 64) / Storage::PAGEWIDTH;

	nX %= Storage::PAGEWIDTH;
	nY %= Storage::PAGEHEIGHT;

	nBlockNum = Storage::m_nBlockSwizzleTable[nY / Storage::BLOCKHEIGHT][nX / Storage::BLOCKWIDTH];

	nX %= Storage::BLOCKWIDTH;
	nY %= Storage::BLOCKHEIGHT;

	nColumnNum = (nY / Storage::COLUMNHEIGHT);

	nY %= Storage.COLUMNHEIGHT;

	nOffset = m_nPointer + (nPageNum * PAGESIZE) + (nBlockNum * BLOCKSIZE) + (nColumnNum * COLUMNSIZE);
	nOffset &= (RAMSIZE - 1);

	if((nX < 8) && (nY < 2))
	{
		nByte = 0;
		nTable = 0;
	}
	else if((nX >= 8) && (nY < 2))
	{
		nByte = 2;
		nTable = 0;
	}
	else if((nX < 8) && (nY >= 2))
	{
		nByte = 1;
		nTable = 1;
	}
	else
	{
		nByte = 3;
		nTable = 1;
	}

	if(nColumnNum & 0x01) nTable ^= 0x01;

	nX &= 0x7;
	nY &= 0x1;

	return ((uint8*)&m_pMemory[nOffset])[nColSwizzleTable[nTable][nY][nX] * 4 + nByte];
	//return &((uint8*)&m_pMemory[nOffset])[Storage::m_nColumnSwizzleTable[nY][nX]];
}

void CGSH_OpenGL::TexUploader_Psm8_Cvt(GSTEX0* pReg0, GSTEXA* pTexA)
{
	unsigned int nWidth, nHeight, nDstPitch, i, j;
	uint32 nPointer;
	uint32* pDst;
	uint8 nPixel;

	nPointer	= pReg0->GetBufPtr();
	nWidth		= pReg0->GetWidth();
	nHeight		= pReg0->GetHeight();
	nDstPitch	= nWidth;
	pDst		= (uint32*)m_pCvtBuffer;

	CPixelIndexorPSMT8 Indexor(m_pRAM, nPointer, pReg0->nBufWidth);

	ReadCLUT8(pReg0);

	if(pReg0->nCPSM == PSMCT32)
	{
		for(j = 0; j < nHeight; j++)
		{
			for(i = 0; i < nWidth; i++)
			{
				nPixel = Indexor.GetPixel(i, j);
				pDst[i] = m_pCLUT32[nPixel];
				//pDst[i] = (nPixel) | (nPixel << 8) | (nPixel << 16) | 0xFF000000;
			}

			pDst += nDstPitch;
		}
	}
	else if(pReg0->nCPSM == PSMCT16)
	{
		for(j = 0; j < nHeight; j++)
		{
			for(i = 0; i < nWidth; i++)
			{
				nPixel = Indexor.GetPixel(i, j);
				pDst[i] = RGBA16ToRGBA32(m_pCLUT16[nPixel]);
			}

			pDst += nDstPitch;
		}
	}
	else
	{
		assert(0);
	}

	glPixelTransferf(GL_ALPHA_SCALE, 2.0f);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, nWidth);
	glTexImage2D(GL_TEXTURE_2D, 0, 4, nWidth, nHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_pCvtBuffer);
}

void CGSH_OpenGL::TexUploader_Psm16_Hw(GSTEX0* pReg0, GSTEXA* pTexA)
{

	unsigned int nWidth, nHeight;
	uint32 nPointer;

	nPointer	= pReg0->GetBufPtr();
	nWidth		= pReg0->GetWidth();
	nHeight		= pReg0->GetHeight();

	FetchImagePSCMT16((uint16*)m_pCvtBuffer, nPointer, pReg0->nBufWidth, nWidth, nHeight);

	if(pReg0->nColorComp == 1)
	{
		//This isn't right... TA0 isn't considered
		glPixelTransferf(GL_ALPHA_SCALE, (float)pTexA->nTA1);
	}
	else
	{
		glPixelTransferf(GL_ALPHA_SCALE, 0);
	}

	glPixelStorei(GL_UNPACK_ROW_LENGTH, nWidth);
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB5_A1, nWidth, nHeight, 0, GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV, m_pRAM + nPointer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB5_A1, nWidth, nHeight, 0, GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV, m_pCvtBuffer);
}

void CGSH_OpenGL::TexUploader_Psm16_Cvt(GSTEX0* pReg0, GSTEXA* pTexA)
{
	unsigned int nWidth, nHeight, nSrcPitch, nDstPitch, i, j;
	uint8 nTA0, nTA1;
	uint8 nR, nG, nB, nA;
	uint16* pSrc;
	uint32* pDst;
	uint16 nPixel;
	uint32 nPointer;

	nPointer	= pReg0->GetBufPtr();
	nWidth		= pReg0->GetWidth();
	nHeight		= pReg0->GetHeight();

	//No clue about this... 'lobotom.elf'
	nSrcPitch = 0x280;
	nDstPitch = nWidth;

	pSrc = (uint16*)(m_pRAM + nPointer);
	pDst = (uint32*)(m_pCvtBuffer);

	if(pReg0->nColorComp == 1)
	{
		nTA0 = pTexA->nTA0;
		nTA1 = pTexA->nTA1;
	}
	else
	{
		nTA0 = 0;
		nTA1 = 0;
	}

	for(j = 0; j < nHeight; j++)
	{
		for(i = 0; i < nWidth; i++)
		{
			nPixel = pSrc[i];

			nB = (nPixel >>  0) & 0x1F;
			nG = (nPixel >>  5) & 0x1F;
			nR = (nPixel >> 10) & 0x1F;
			nA = (nPixel >> 15) & 0x01;

			nB <<= 3;
			nG <<= 3;
			nR <<= 3;
			nA = (nA == 0) ? nTA0 : nTA1;

			*(((uint8*)&pDst[i]) + 0) = nB;
			*(((uint8*)&pDst[i]) + 1) = nG;
			*(((uint8*)&pDst[i]) + 2) = nR;
			*(((uint8*)&pDst[i]) + 3) = nA;
		}

		pSrc += nSrcPitch;
		pDst += nDstPitch;
	}

	glPixelTransferf(GL_ALPHA_SCALE, 2.0f);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, nWidth);
	glTexImage2D(GL_TEXTURE_2D, 0, 4, nWidth, nHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_pCvtBuffer);
}

void CGSH_OpenGL::TexUploader_Psm4_Cvt(GSTEX0* pReg0, GSTEXA* pTexA)
{
	unsigned int nWidth, nHeight, nDstPitch, i, j;
	uint32 nPointer;
	uint32* pDst;
	uint32 nPixel;

	nPointer	= pReg0->GetBufPtr();
	nWidth		= pReg0->GetWidth();
	nHeight		= pReg0->GetHeight();
	nDstPitch	= nWidth;
	pDst		= (uint32*)m_pCvtBuffer;

	CPixelIndexorPSMT4 Indexor(m_pRAM, nPointer, pReg0->nBufWidth);

	ReadCLUT4(pReg0);

	if(pReg0->nCPSM == PSMCT32)
	{
		for(j = 0; j < nHeight; j++)
		{
			/*
			for(i = 0; i < nWidth; i += 2)
			{
				nPixel = *(Indexor.GetPixel(i / 2, j));
				//pDst[i + 0] = m_pCLUT32[(nPixel >> 0x00) & 0x0F];
				//pDst[i + 1] = m_pCLUT32[(nPixel >> 0x04) & 0x0F];
				pDst[i + 0] = 0xFFFF0000 | ((nPixel & 0x0F) << 12) | ((nPixel & 0x0F) << 4);
				pDst[i + 1] = 0xFFFF0000 | ((nPixel & 0xF0) <<  8) | ((nPixel & 0xF0) << 0);
			}
			*/

			for(i = 0; i < nWidth; i++)
			{
				nPixel = Indexor.GetPixel(i, j);
				//pDst[i] = 0xFFFF0000 | ((nPixel & 0x0F) << 12) | ((nPixel & 0x0F) << 4);
				pDst[i] = m_pCLUT32[nPixel];
			}

			pDst += nDstPitch;
		}
	}
	else if(pReg0->nCPSM == PSMCT16)
	{
		for(j = 0; j < nHeight; j++)
		{
			for(i = 0; i < nWidth; i++)
			{
				nPixel = Indexor.GetPixel(i, j);
				pDst[i] = RGBA16ToRGBA32(m_pCLUT16[nPixel]);
			}

			pDst += nDstPitch;
		}
	}
	else
	{
		assert(0);
	}


	glPixelTransferf(GL_ALPHA_SCALE, 2.0f);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, nWidth);
	glTexImage2D(GL_TEXTURE_2D, 0, 4, nWidth, nHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_pCvtBuffer);	
}

template <uint32 nShift>
void CGSH_OpenGL::TexUploader_Psm4H_Cvt(GSTEX0* pReg0, GSTEXA* pTexA)
{
	unsigned int nWidth, nHeight, nDstPitch, i, j;
	uint32 nPointer;
	uint32* pDst;
	uint32 nPixel;

	nPointer	= pReg0->GetBufPtr();
	nWidth		= pReg0->GetWidth();
	nHeight		= pReg0->GetHeight();
	nDstPitch	= nWidth;
	pDst		= (uint32*)m_pCvtBuffer;

	CPixelIndexorPSMCT32 Indexor(m_pRAM, nPointer, pReg0->nBufWidth);

	ReadCLUT4(pReg0);

	if(pReg0->nCPSM == PSMCT32)
	{
		for(j = 0; j < nHeight; j++)
		{
			for(i = 0; i < nWidth; i++)
			{
				nPixel = Indexor.GetPixel(i, j);
				nPixel = (nPixel >> nShift) & 0x0F;
				pDst[i] = m_pCLUT32[nPixel];
			}

			pDst += nDstPitch;
		}
	}
	else if(pReg0->nCPSM == PSMCT16)
	{
		for(j = 0; j < nHeight; j++)
		{
			for(i = 0; i < nWidth; i++)
			{
				nPixel = Indexor.GetPixel(i, j);
				nPixel = (nPixel >> nShift) & 0x0F;
				pDst[i] = RGBA16ToRGBA32(m_pCLUT16[nPixel]);
			}

			pDst += nDstPitch;
		}
	}
	else
	{
		assert(0);
	}


	glPixelTransferf(GL_ALPHA_SCALE, 2.0f);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, nWidth);
	glTexImage2D(GL_TEXTURE_2D, 0, 4, nWidth, nHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_pCvtBuffer);
}

void CGSH_OpenGL::TexUploader_Psm8H_Cvt(GSTEX0* pReg0, GSTEXA* pTexA)
{
	unsigned int nWidth, nHeight, nDstPitch, i, j;
	uint32 nPointer;
	uint32* pDst;
	uint32 nPixel;

	nPointer	= pReg0->GetBufPtr();
	nWidth		= pReg0->GetWidth();
	nHeight		= pReg0->GetHeight();
	nDstPitch	= nWidth;
	pDst		= (uint32*)m_pCvtBuffer;

	CPixelIndexorPSMCT32 Indexor(m_pRAM, nPointer, pReg0->nBufWidth);

	ReadCLUT8(pReg0);

	if(pReg0->nCPSM == PSMCT32)
	{
		for(j = 0; j < nHeight; j++)
		{
			for(i = 0; i < nWidth; i++)
			{
				nPixel = Indexor.GetPixel(i, j);
				nPixel = (nPixel >> 24);
				pDst[i] = m_pCLUT32[nPixel];
			}

			pDst += nDstPitch;
		}
	}
	else if(pReg0->nCPSM == PSMCT16)
	{
		for(j = 0; j < nHeight; j++)
		{
			for(i = 0; i < nWidth; i++)
			{
				nPixel = Indexor.GetPixel(i, j);
				nPixel = (nPixel >> 24);
				pDst[i] = RGBA16ToRGBA32(m_pCLUT16[nPixel]);
			}

			pDst += nDstPitch;
		}
	}
	else
	{
		assert(0);
	}

	glPixelTransferf(GL_ALPHA_SCALE, 2.0f);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, nWidth);
	glTexImage2D(GL_TEXTURE_2D, 0, 4, nWidth, nHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_pCvtBuffer);
}

/////////////////////////////////////////////////////////////
// Texture
/////////////////////////////////////////////////////////////

CGSH_OpenGL::CTexture::CTexture()
{
	m_nStart	= 0;
	m_nSize		= 0;
	m_nTexture	= 0;
}

CGSH_OpenGL::CTexture::~CTexture()
{
	Invalidate();
}

void CGSH_OpenGL::CTexture::Invalidate()
{
	if(m_nTexture != 0)
	{
		glDeleteTextures(1, &m_nTexture);
		m_nTexture = 0;
	}
}

void CGSH_OpenGL::CTexture::InvalidateFromMemorySpace(uint32 nStart, uint32 nSize)
{
	bool nInvalid;
	uint32 nTexEnd, nEnd;

	if(!IsValid()) return;

	nInvalid = false;
	nEnd = nStart + nSize;
	nTexEnd = m_nStart + m_nSize;

	if((nStart >= m_nStart) && (nStart < nTexEnd))
	{
		nInvalid = true;
	}

	if((nEnd >= m_nStart) && (nEnd < nTexEnd))
	{
		nInvalid = true;
	}

	if(nInvalid)
	{
		Invalidate();
	}
}

bool CGSH_OpenGL::CTexture::IsValid()
{
	return (m_nTexture != 0);
}

/////////////////////////////////////////////////////////////
// Texture Caching
/////////////////////////////////////////////////////////////

unsigned int CGSH_OpenGL::TexCache_Search(GSTEX0* pTex0)
{
	unsigned int i;

	for(i = 0; i < MAXCACHE; i++)
	{
		if(!m_TexCache[i].IsValid()) continue;
//		if(m_TexCache[i].m_nStart != pTex0->GetBufPtr()) continue;
//		if(m_TexCache[i].m_nPSM != pTex0->nPsm) continue;
//		if(m_TexCache[i].m_nCLUTAddress != pTex0->GetCLUTPtr()) continue;
		if(*(uint64*)pTex0 != m_TexCache[i].m_nTex0) continue;
		if(m_TexCache[i].m_nIsCSM2)
		{
			if(((*(uint64*)GetTexClut()) & 0x3FFFFF) != m_TexCache[i].m_nTexClut) continue;
		}

		return m_TexCache[i].m_nTexture;
	}

	return 0;
}

void CGSH_OpenGL::TexCache_Insert(GSTEX0* pTex0, unsigned int nTexture)
{
	CTexture* pTexture;
	pTexture = &m_TexCache[m_nTexCacheIndex];

	pTexture->Invalidate();

	pTexture->m_nStart			= pTex0->GetBufPtr();
	pTexture->m_nSize			= pTex0->GetBufWidth() * pTex0->GetHeight() * GetPsmPixelSize(pTex0->nPsm) / 8;
//	pTexture->m_nPSM			= pTex0->nPsm;
//	pTexture->m_nCLUTAddress	= pTex0->GetCLUTPtr();
	pTexture->m_nTex0			= *(uint64*)pTex0;
	pTexture->m_nTexClut		= (*(uint64*)GetTexClut()) & 0x3FFFFF;
	pTexture->m_nIsCSM2			= pTex0->nCSM == 1;
	pTexture->m_nTexture		= nTexture;

	m_nTexCacheIndex++;
	m_nTexCacheIndex %= MAXCACHE;
}

void CGSH_OpenGL::TexCache_InvalidateTextures(uint32 nStart, uint32 nSize)
{
	unsigned int i;

	for(i = 0; i < MAXCACHE; i++)
	{
		m_TexCache[i].InvalidateFromMemorySpace(nStart, nSize);
	}
}

void CGSH_OpenGL::TexCache_Flush()
{
	for(unsigned int i = 0; i < MAXCACHE; i++)
	{
		m_TexCache[i].Invalidate();
	}
}
