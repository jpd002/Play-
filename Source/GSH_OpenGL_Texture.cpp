#include <assert.h>
#include <sys/stat.h>
#include <limits.h>
#include <algorithm>
#include <zlib.h>
#include "GSH_OpenGL.h"
#include "StdStream.h"
#include "BMP.h"

using namespace std;
using namespace Framework;

/////////////////////////////////////////////////////////////
// Texture Loading
/////////////////////////////////////////////////////////////

unsigned int CGSH_OpenGL::LoadTexture(GSTEX0* pReg0, GSTEX1* pReg1, CLAMP* pClamp)
{
	int nMagFilter, nMinFilter;
	int nWrapS, nWrapT;
	GSTEXA TexA;

	uint32 nPointer     = pReg0->GetBufPtr();
	uint32 nWidth		= pReg0->GetWidth();
	uint32 nHeight		= pReg0->GetHeight();

	//Set the right texture function
	switch(pReg0->nFunction)
	{
	case 0:
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		break;
	case 1:
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		break;
    case 2:
    case 3:
        //Just use modulate for now, but we'd need to use a different combining mode ideally
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
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

			glUseProgram(*m_pProgram);

			unsigned int nMode[2];
			unsigned int nMin[2];
			unsigned int nMax[2];

			nMode[0] = pClamp->nWMS - 1;
			nMode[1] = pClamp->nWMT - 1;

			nMin[0] = pClamp->GetMinU();
			nMin[1] = pClamp->GetMinV();

			nMax[0] = pClamp->GetMaxU();
			nMax[1] = pClamp->GetMaxV();

			for(unsigned int i = 0; i < 2; i++)
			{
				if(nMode[i] == 2)
				{
					//Check if we can transform in something more elegant

					for(unsigned int j = 1; j < 0x3FF; j = ((j << 1) | 1))
					{
						if(nMin[i] < j) break;
						if(nMin[i] != j) continue;

						if((nMin[i] & nMax[i]) != 0) break;

						nMin[i]++;
						nMode[i] = 3;
					}
				}
			}

			m_pProgram->SetUniformi("g_nClamp",	nMode[0],			nMode[1]);
			m_pProgram->SetUniformi("g_nMin",	nMin[0],			nMin[1]);
			m_pProgram->SetUniformi("g_nMax",	nMax[0],			nMax[1]);
			m_pProgram->SetUniformf("g_nSize",	(float)nWidth,		(float)nHeight);
		}
		else
		{
			glUseProgram(NULL);
		}
	}

    unsigned int nTexture = TexCache_SearchLive(pReg0);
	if(nTexture != 0) return nTexture;

	DECODE_TEXA(m_nReg[GS_REG_TEXA], TexA);

    //Test code
    uint32 textureChecksum = 0; 
    switch(pReg0->nPsm)
    {
	case PSMT8:
        textureChecksum = ConvertTexturePsm8(pReg0, &TexA);
		break;
    case PSMT4:
        textureChecksum = ConvertTexturePsm4(pReg0, &TexA);
        break;
    case PSMT8H:
        textureChecksum = ConvertTexturePsm8H(pReg0, &TexA);
        break;
    }

    if(textureChecksum)
    {
        //Check if we don't already have it somewhere...    
        nTexture = TexCache_SearchDead(pReg0, textureChecksum);
        if(nTexture != 0)
        {
            return nTexture;
        }
    }

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
		break;
	case PSMCT16:
		((this)->*(m_pTexUploader_Psm16))(pReg0, &TexA);
		break;
	case PSMCT16S:
		TexUploader_Psm16S_Hw(pReg0, &TexA);
		break;
	case PSMT8:
		//((this)->*(m_pTexUploader_Psm8))(pReg0, &TexA);
        UploadTexturePsm8(pReg0, &TexA);
		break;
	case PSMT4:
		//TexUploader_Psm4_Cvt(pReg0, &TexA);
        UploadTexturePsm8(pReg0, &TexA);
		break;
	case PSMT4HL:
		TexUploader_Psm4H_Cvt<24>(pReg0, &TexA);
		break;
	case PSMT4HH:
		TexUploader_Psm4H_Cvt<28>(pReg0, &TexA);
		break;
	case PSMT8H:
		//TexUploader_Psm8H_Cvt(pReg0, &TexA);
        UploadTexturePsm8(pReg0, &TexA);
		break;
	default:
		assert(0);
		break;
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

//    DumpTexture(nWidth, nHeight);

	glBindTexture(GL_TEXTURE_2D, 0);

	TexCache_Insert(pReg0, nTexture, textureChecksum);

	return nTexture;
}

void CGSH_OpenGL::DumpTexture(unsigned int nWidth, unsigned int nHeight)
{
#ifdef WIN32
	char sFilename[256];

	for(unsigned int i = 0; i < UINT_MAX; i++)
	{
	    struct _stat Stat;
		sprintf(sFilename, "./textures/tex_%0.8X.bmp", i);
		if(_stat(sFilename, &Stat) == -1) break;
	}

	CBitmap Bitmap(nWidth, nHeight, 32);

	glGetTexImage(GL_TEXTURE_2D, 0, GL_BGRA, GL_UNSIGNED_BYTE, Bitmap.GetPixels());
	CBMP::ToBMP(&Bitmap, &CStdStream(fopen(sFilename, "wb")));
#endif
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
				nAlpha = (uint8)min<uint8>((unsigned int)nAlpha * 2, 0xFF);
				*((uint8*)&nPalette[nPos + j] + 3) = nAlpha;
			}
			pCLUT += 0x08;

			for(j = 0; j < 8; j++)
			{
				nPalette[nPos + j + 0x10] = pCLUT[j];

				nAlpha = *((uint8*)&nPalette[nPos + j + 0x10] + 3);
				nAlpha = (uint8)min<uint8>((unsigned int)nAlpha * 2, 0xFF);
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

void CGSH_OpenGL::TexUploader_Psm8_Cvt(GSTEX0* pReg0, GSTEXA* pTexA)
{
	uint32 nPointer         = pReg0->GetBufPtr();
	unsigned int nWidth		= pReg0->GetWidth();
	unsigned int nHeight	= pReg0->GetHeight();
	unsigned int nDstPitch	= nWidth;
    uint32* dstBuffer       = reinterpret_cast<uint32*>(m_pCvtBuffer);
    uint32* pDst		    = dstBuffer;
    uint32 checksum         = crc32(0, NULL, 0);

	CPixelIndexorPSMT8 Indexor(m_pRAM, nPointer, pReg0->nBufWidth);

	ReadCLUT8(pReg0);

	if(pReg0->nCPSM == PSMCT32)
	{
		for(unsigned int j = 0; j < nHeight; j++)
		{
			for(unsigned int i = 0; i < nWidth; i++)
			{
				uint8 nPixel = Indexor.GetPixel(i, j);
				pDst[i] = m_pCLUT32[nPixel];
				//pDst[i] = (nPixel) | (nPixel << 8) | (nPixel << 16) | 0xFF000000;
			}

            checksum = crc32(checksum, reinterpret_cast<Bytef*>(pDst), sizeof(uint32) * nWidth);
			pDst += nDstPitch;
		}
	}
	else if(pReg0->nCPSM == PSMCT16)
	{
		for(unsigned int j = 0; j < nHeight; j++)
		{
			for(unsigned int i = 0; i < nWidth; i++)
			{
				uint8 nPixel = Indexor.GetPixel(i, j);
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
    glTexImage2D(GL_TEXTURE_2D, 0, 4, nWidth, nHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, dstBuffer);
}

uint32 CGSH_OpenGL::ConvertTexturePsm8(GSTEX0* pReg0, GSTEXA* pTexA)
{
	uint32 nPointer         = pReg0->GetBufPtr();
	unsigned int nWidth		= pReg0->GetWidth();
	unsigned int nHeight	= pReg0->GetHeight();
	unsigned int nDstPitch	= nWidth;
    uint32* dstBuffer       = reinterpret_cast<uint32*>(m_pCvtBuffer);
    uint32* pDst		    = dstBuffer;
    uint32 checksum         = crc32(0, NULL, 0);

	CPixelIndexorPSMT8 Indexor(m_pRAM, nPointer, pReg0->nBufWidth);

	ReadCLUT8(pReg0);

	if(pReg0->nCPSM == PSMCT32)
	{
		for(unsigned int j = 0; j < nHeight; j++)
		{
			for(unsigned int i = 0; i < nWidth; i++)
			{
				uint8 nPixel = Indexor.GetPixel(i, j);
				pDst[i] = m_pCLUT32[nPixel];
				//pDst[i] = (nPixel) | (nPixel << 8) | (nPixel << 16) | 0xFF000000;
			}

            checksum = crc32(checksum, reinterpret_cast<Bytef*>(pDst), sizeof(uint32) * nWidth);
			pDst += nDstPitch;
		}
	}
	else if(pReg0->nCPSM == PSMCT16)
	{
		for(unsigned int j = 0; j < nHeight; j++)
		{
			for(unsigned int i = 0; i < nWidth; i++)
			{
				uint8 nPixel = Indexor.GetPixel(i, j);
				pDst[i] = RGBA16ToRGBA32(m_pCLUT16[nPixel]);
			}

            checksum = crc32(checksum, reinterpret_cast<Bytef*>(pDst), sizeof(uint32) * nWidth);
            pDst += nDstPitch;
		}
	}
	else
	{
		assert(0);
	}

    return checksum;
}

uint32 CGSH_OpenGL::ConvertTexturePsm8H(GSTEX0* pReg0, GSTEXA* pTexA)
{
	unsigned int i, j;
	uint32 nPixel;

	uint32 nPointer         = pReg0->GetBufPtr();
	unsigned int nWidth		= pReg0->GetWidth();
	unsigned int nHeight	= pReg0->GetHeight();
	unsigned int nDstPitch	= nWidth;
	uint32* pDst		    = (uint32*)m_pCvtBuffer;
    uint32 checksum         = crc32(0, NULL, 0);

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

            checksum = crc32(checksum, reinterpret_cast<Bytef*>(pDst), sizeof(uint32) * nWidth);
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

            checksum = crc32(checksum, reinterpret_cast<Bytef*>(pDst), sizeof(uint32) * nWidth);
            pDst += nDstPitch;
		}
	}
	else
	{
		assert(0);
	}

    return checksum;
}

uint32 CGSH_OpenGL::ConvertTexturePsm4(GSTEX0* pReg0, GSTEXA* pTexA)
{
    uint32 nPointer         = pReg0->GetBufPtr();
	unsigned int nWidth     = pReg0->GetWidth();
	unsigned int nHeight    = pReg0->GetHeight();
	unsigned int nDstPitch  = nWidth;
    uint32* dstBuffer       = reinterpret_cast<uint32*>(m_pCvtBuffer);
    uint32* pDst            = dstBuffer;
    uint32 checksum         = crc32(0, NULL, 0);

    CPixelIndexorPSMT4 Indexor(m_pRAM, nPointer, pReg0->nBufWidth);

	ReadCLUT4(pReg0);

	if(pReg0->nCPSM == PSMCT32)
	{
		for(unsigned int j = 0; j < nHeight; j++)
		{
			for(unsigned int i = 0; i < nWidth; i++)
			{
				uint32 nPixel = Indexor.GetPixel(i, j);
				pDst[i] = m_pCLUT32[nPixel];
			}

            checksum = crc32(checksum, reinterpret_cast<Bytef*>(pDst), sizeof(uint32) * nWidth);
            pDst += nDstPitch;
		}
	}
	else if(pReg0->nCPSM == PSMCT16)
	{
		assert(0);
		for(unsigned int j = 0; j < nHeight; j++)
		{
			for(unsigned int i = 0; i < nWidth; i++)
			{
				uint32 nPixel = Indexor.GetPixel(i, j);
				pDst[i] = RGBA16ToRGBA32(m_pCLUT16[nPixel]);
			}

			pDst += nDstPitch;
		}
	}
	else
	{
		assert(0);
	}

    return checksum;
}

void CGSH_OpenGL::UploadTexturePsm8(GSTEX0* pReg0, GSTEXA* pTexA)
{
	unsigned int nWidth		= pReg0->GetWidth();
	unsigned int nHeight	= pReg0->GetHeight();

	glPixelTransferf(GL_ALPHA_SCALE, 2.0f);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, nWidth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, nWidth, nHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_pCvtBuffer);
}

void CGSH_OpenGL::TexUploader_Psm16_Hw(GSTEX0* pReg0, GSTEXA* pTexA)
{
	unsigned int nWidth, nHeight;
	uint32 nPointer;

	nPointer	= pReg0->GetBufPtr();
	nWidth		= pReg0->GetWidth();
	nHeight		= pReg0->GetHeight();

	FetchImagePSMCT16((uint16*)m_pCvtBuffer, nPointer, pReg0->nBufWidth, nWidth, nHeight);

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

void CGSH_OpenGL::TexUploader_Psm16S_Hw(GSTEX0* pReg0, GSTEXA* pTexA)
{
	unsigned int nWidth, nHeight;
	uint32 nPointer;

	nPointer	= pReg0->GetBufPtr();
	nWidth		= pReg0->GetWidth();
	nHeight		= pReg0->GetHeight();

	FetchImagePSMCT16S(reinterpret_cast<uint16*>(m_pCvtBuffer), nPointer, pReg0->nBufWidth, nWidth, nHeight);

	if(pReg0->nColorComp == 1)
	{
		//This isn't right... TA0 isn't considered
		glPixelTransferf(GL_ALPHA_SCALE, static_cast<float>(pTexA->nTA1));
	}
	else
	{
		glPixelTransferf(GL_ALPHA_SCALE, 0);
	}

	glPixelStorei(GL_UNPACK_ROW_LENGTH, nWidth);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB5_A1, nWidth, nHeight, 0, GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV, m_pCvtBuffer);
}

void CGSH_OpenGL::TexUploader_Psm4_Cvt(GSTEX0* pReg0, GSTEXA* pTexA)
{
    uint32 nPointer         = pReg0->GetBufPtr();
	unsigned int nWidth     = pReg0->GetWidth();
	unsigned int nHeight    = pReg0->GetHeight();
	unsigned int nDstPitch  = nWidth;

    //With PBO
//    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, m_pixelBuffers[m_currentPixelBuffer]);
//    m_currentPixelBuffer++;
//    m_currentPixelBuffer %= MAX_PIXEL_BUFFERS;
//    uint32* dstBuffer = reinterpret_cast<uint32*>(glMapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB));

    //Without PBO
    uint32* dstBuffer       = reinterpret_cast<uint32*>(m_pCvtBuffer);

    uint32* pDst            = dstBuffer;

    //////
    CPixelIndexorPSMT4 Indexor(m_pRAM, nPointer, pReg0->nBufWidth);

	ReadCLUT4(pReg0);

	if(pReg0->nCPSM == PSMCT32)
	{
		for(unsigned int j = 0; j < nHeight; j++)
		{
			for(unsigned int i = 0; i < nWidth; i++)
			{
				uint32 nPixel = Indexor.GetPixel(i, j);
				pDst[i] = m_pCLUT32[nPixel];
			}

			pDst += nDstPitch;
		}
	}
	else if(pReg0->nCPSM == PSMCT16)
	{
		for(unsigned int j = 0; j < nHeight; j++)
		{
			for(unsigned int i = 0; i < nWidth; i++)
			{
				uint32 nPixel = Indexor.GetPixel(i, j);
				pDst[i] = RGBA16ToRGBA32(m_pCLUT16[nPixel]);
			}

			pDst += nDstPitch;
		}
	}
	else
	{
		assert(0);
	}
    //////

	glPixelTransferf(GL_ALPHA_SCALE, 2.0f);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, nWidth);

    //Without PBO
    glTexImage2D(GL_TEXTURE_2D, 0, 4, nWidth, nHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, dstBuffer);	

    //With PBO
    //glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB);
    //glTexImage2D(GL_TEXTURE_2D, 0, 4, nWidth, nHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);	
    //glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
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

CGSH_OpenGL::CTexture::CTexture() :
m_nStart(0),
m_nSize(0),
m_nPSM(0),
m_nCLUTAddress(0),
m_nTex0(0),
m_nTexClut(0),
m_nIsCSM2(false),
m_nTexture(0),
m_checksum(0),
m_live(false)
{

}

CGSH_OpenGL::CTexture::~CTexture()
{
    Free();
}

void CGSH_OpenGL::CTexture::Free()
{
	if(m_nTexture != 0)
	{
		glDeleteTextures(1, &m_nTexture);
		m_nTexture = 0;
        m_live = false;
	}
}

void CGSH_OpenGL::CTexture::InvalidateFromMemorySpace(uint32 nStart, uint32 nSize)
{
    if(!m_live) return;

	bool nInvalid = false;
	uint32 nEnd = nStart + nSize;
	uint32 nTexEnd = m_nStart + m_nSize;

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
        m_live = false;
	}
}

/////////////////////////////////////////////////////////////
// Texture Caching
/////////////////////////////////////////////////////////////

unsigned int CGSH_OpenGL::TexCache_SearchLive(GSTEX0* pTex0)
{
    for(TextureList::iterator textureIterator(m_TexCache.begin());
        textureIterator != m_TexCache.end(); textureIterator++)
	{
        CTexture* texture = *textureIterator;
        if(!texture->m_live) continue;
        //if(!texture.IsValid()) continue;
        //if(m_TexCache[i].m_nStart != pTex0->GetBufPtr()) continue;
        //if(m_TexCache[i].m_nPSM != pTex0->nPsm) continue;
        //if(m_TexCache[i].m_nCLUTAddress != pTex0->GetCLUTPtr()) continue;
	    if(*(uint64*)pTex0 != texture->m_nTex0) continue;
	    if(texture->m_nIsCSM2)
	    {
		    if(((*(uint64*)GetTexClut()) & 0x3FFFFF) != texture->m_nTexClut) continue;
	    }
        m_TexCache.erase(textureIterator);
        m_TexCache.push_front(texture);
        return texture->m_nTexture;
	}

	return 0;
}

unsigned int CGSH_OpenGL::TexCache_SearchDead(GSTEX0* pTex0, uint32 checksum)
{
    for(TextureList::iterator textureIterator(m_TexCache.begin());
        textureIterator != m_TexCache.end(); textureIterator++)
    {
        CTexture* texture = *textureIterator;
        if(texture->m_nTexture == 0) continue;
        if(texture->m_checksum != checksum) continue;
        if(texture->m_nTex0 != *reinterpret_cast<uint64*>(pTex0)) continue;
        texture->m_live = true;
        m_TexCache.erase(textureIterator);
        m_TexCache.push_front(texture);
        return texture->m_nTexture;
    }

    return 0;
}

void CGSH_OpenGL::TexCache_Insert(GSTEX0* pTex0, unsigned int nTexture, uint32 checksum)
{
    CTexture* pTexture = *m_TexCache.rbegin();
    pTexture->Free();

	pTexture->m_nStart			= pTex0->GetBufPtr();
	pTexture->m_nSize			= pTex0->GetBufWidth() * pTex0->GetHeight() * GetPsmPixelSize(pTex0->nPsm) / 8;
//	pTexture->m_nPSM			= pTex0->nPsm;
//	pTexture->m_nCLUTAddress	= pTex0->GetCLUTPtr();
	pTexture->m_nTex0			= *reinterpret_cast<uint64*>(pTex0);
	pTexture->m_nTexClut		= (*(uint64*)GetTexClut()) & 0x3FFFFF;
	pTexture->m_nIsCSM2			= pTex0->nCSM == 1;
	pTexture->m_nTexture		= nTexture;
    pTexture->m_checksum        = checksum;
    pTexture->m_live            = true;

    m_TexCache.pop_back();
    m_TexCache.push_front(pTexture);
}

void CGSH_OpenGL::TexCache_InvalidateTextures(uint32 nStart, uint32 nSize)
{
    for(TextureList::iterator textureIterator(m_TexCache.begin());
        textureIterator != m_TexCache.end(); textureIterator++)
	{
        (*textureIterator)->InvalidateFromMemorySpace(nStart, nSize);
	}
}

void CGSH_OpenGL::TexCache_Flush()
{
    for(TextureList::iterator textureIterator(m_TexCache.begin());
        textureIterator != m_TexCache.end(); textureIterator++)
	{
        (*textureIterator)->Free();
	}
}
