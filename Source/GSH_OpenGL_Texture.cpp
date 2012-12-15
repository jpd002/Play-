#include <assert.h>
#include <sys/stat.h>
#include <limits.h>
#include <algorithm>
#include <zlib.h>
#include "GSH_OpenGL.h"
#include "StdStream.h"
#include "BMP.h"

#define TEX0_CLUTINFO_MASK (~0xFFFFFFE000000000ULL)

/////////////////////////////////////////////////////////////
// Texture Loading
/////////////////////////////////////////////////////////////

void CGSH_OpenGL::PrepareTexture(const TEX0& tex0)
{
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();

	for(auto framebufferIterator(std::begin(m_framebuffers));
		framebufferIterator != std::end(m_framebuffers); framebufferIterator++)
	{
		const auto& candidateFramebuffer = *framebufferIterator;
		if(candidateFramebuffer->m_basePtr == tex0.GetBufPtr() &&
			candidateFramebuffer->m_width == tex0.GetBufWidth() &&
			candidateFramebuffer->m_canBeUsedAsTexture)
		{
			//We have a winner
			glBindTexture(GL_TEXTURE_2D, candidateFramebuffer->m_texture);

			float scaleRatioX = static_cast<float>(tex0.GetWidth()) / static_cast<float>(candidateFramebuffer->m_width);
			float scaleRatioY = static_cast<float>(tex0.GetHeight()) / static_cast<float>(candidateFramebuffer->m_height);

			//If we're currently in interlaced mode, framebuffer will have twice the height
			bool halfHeight = GetCrtIsInterlaced() && GetCrtIsFrameMode();
			if(halfHeight) scaleRatioY *= 2.0f;

			glScalef(scaleRatioX, scaleRatioY, 1);

			return;
		}
	}

	GLuint nTexture = TexCache_Search(tex0);
	if(nTexture != 0)
	{
		glBindTexture(GL_TEXTURE_2D, nTexture);
		return;
	}

	TEXA texA;
	texA <<= m_nReg[GS_REG_TEXA];

	glGenTextures(1, &nTexture);
	glBindTexture(GL_TEXTURE_2D, nTexture);

	//Fix the pitch of the image
	glPixelStorei(GL_UNPACK_ROW_LENGTH, tex0.GetBufWidth());

	switch(tex0.nPsm)
	{
	case PSMCT32:
		TexUploader_Psm32(tex0, texA);
		break;
	case 9:
		TexUploader_Psm24(tex0, texA);
		break;
	case PSMCT24:
		TexUploader_Psm24(tex0, texA);
		break;
	case PSMCT16:
		((this)->*(m_pTexUploader_Psm16))(tex0, texA);
		break;
	case PSMCT16S:
		TexUploader_Psm16S_Hw(tex0, texA);
		break;
	case PSMT8:
		TexUploader_Psm8(tex0, texA);
		break;
	case PSMT4:
		TexUploader_Psm4(tex0, texA);
		break;
	case PSMT4HL:
		TexUploader_Psm4H<24>(tex0, texA);
		break;
	case PSMT4HH:
		TexUploader_Psm4H<28>(tex0, texA);
		break;
	case PSMT8H:
		TexUploader_Psm8H(tex0, texA);
		break;
	default:
		assert(0);
		break;
	}

//	DumpTexture(nWidth, nHeight, textureChecksum);

	TexCache_Insert(tex0, nTexture);
}

void CGSH_OpenGL::PreparePalette(const TEX0& tex0)
{
	GLuint textureHandle = PalCache_Search(tex0);
	if(textureHandle != 0)
	{
		glBindTexture(GL_TEXTURE_2D, textureHandle);
		return;
	}

	uint32 convertedClut[256];
	unsigned int entryCount = IsPsmIDTEX4(tex0.nPsm) ? 16 : 256;

	if(IsPsmIDTEX4(tex0.nPsm))
	{
		uint32 clutOffset = tex0.nCSA * 16;

		if(tex0.nCPSM == PSMCT32)
		{
			assert(tex0.nCSA < 16);

			for(unsigned int i = 0; i < 16; i++)
			{
				uint32 color = 
					(static_cast<uint16>(m_pCLUT[i + clutOffset + 0x000])) | 
					(static_cast<uint16>(m_pCLUT[i + clutOffset + 0x100]) << 16);
				uint32 alpha = MulBy2Clamp(color >> 24);
				color &= ~0xFF000000;
				color |= alpha << 24;
				convertedClut[i] = color;
			}
		}
		else
		{
			assert(tex0.nCSA < 32);

			for(unsigned int i = 0; i < 16; i++)
			{
				convertedClut[i] = RGBA16ToRGBA32(m_pCLUT[i + clutOffset]);
			}
		}
	}
	else if(IsPsmIDTEX8(tex0.nPsm))
	{
		assert(tex0.nCSA == 0);

		if(tex0.nCPSM == PSMCT32)
		{
			for(unsigned int i = 0; i < 256; i++)
			{
				uint32 color = 
					(static_cast<uint16>(m_pCLUT[i + 0x000])) | 
					(static_cast<uint16>(m_pCLUT[i + 0x100]) << 16);
				uint32 alpha = MulBy2Clamp(color >> 24);
				color &= ~0xFF000000;
				color |= alpha << 24;
				convertedClut[i] = color;
			}
		}
		else
		{
			for(unsigned int i = 0; i < 256; i++)
			{
				convertedClut[i] = RGBA16ToRGBA32(m_pCLUT[i]);
			}
		}
	}

	textureHandle = PalCache_Search(entryCount, convertedClut);
	if(textureHandle != 0)
	{
		glBindTexture(GL_TEXTURE_2D, textureHandle);
		return;
	}

	glGenTextures(1, &textureHandle);
	glBindTexture(GL_TEXTURE_2D, textureHandle);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, entryCount, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, convertedClut);

	PalCache_Insert(tex0, convertedClut, textureHandle);
}

void CGSH_OpenGL::DumpTexture(unsigned int nWidth, unsigned int nHeight, uint32 checksum)
{
#ifdef WIN32
	char sFilename[256];

	for(unsigned int i = 0; i < UINT_MAX; i++)
	{
		struct _stat Stat;
		sprintf(sFilename, "./textures/tex_%0.8X_%0.8X.bmp", i, checksum);
		if(_stat(sFilename, &Stat) == -1) break;
	}

	Framework::CBitmap bitmap(nWidth, nHeight, 32);

	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, bitmap.GetPixels());
	Framework::CStdStream outputStream(fopen(sFilename, "wb"));
	Framework::CBMP::WriteBitmap(bitmap, outputStream);
#endif
}

void CGSH_OpenGL::TexUploader_Psm32(const TEX0& tex0, const TEXA& texA)
{
	uint32 nPointer			= tex0.GetBufPtr();
	unsigned int nWidth		= tex0.GetWidth();
	unsigned int nHeight	= tex0.GetHeight();
	unsigned int nDstPitch	= nWidth;
	uint32* pDst			= (uint32*)m_pCvtBuffer;

	CPixelIndexorPSMCT32 Indexor(m_pRAM, nPointer, tex0.nBufWidth);

	for(unsigned int j = 0; j < nHeight; j++)
	{
		for(unsigned int i = 0; i < nWidth; i++)
		{
			pDst[i] = Indexor.GetPixel(i, j);
		}

		pDst += nDstPitch;
	}

	glPixelTransferf(GL_ALPHA_SCALE, 2.0f);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, nWidth);
	glTexImage2D(GL_TEXTURE_2D, 0, 4, nWidth, nHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_pCvtBuffer);
}

void CGSH_OpenGL::TexUploader_Psm24(const TEX0& tex0, const TEXA& texA)
{
	uint32 nPointer			= tex0.GetBufPtr();
	unsigned int nWidth		= tex0.GetWidth();
	unsigned int nHeight	= tex0.GetHeight();
	unsigned int nDstPitch	= nWidth;
	uint32* pDst			= (uint32*)m_pCvtBuffer;

	CPixelIndexorPSMCT32 Indexor(m_pRAM, nPointer, tex0.nBufWidth);

	for(unsigned int j = 0; j < nHeight; j++)
	{
		for(unsigned int i = 0; i < nWidth; i++)
		{
			pDst[i] = Indexor.GetPixel(i, j);
		}

		pDst += nDstPitch;
	}

	glPixelTransferf(GL_ALPHA_SCALE, 2.0f);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, nWidth);
	glTexImage2D(GL_TEXTURE_2D, 0, 4, nWidth, nHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_pCvtBuffer);
}

void CGSH_OpenGL::TexUploader_Psm16_Hw(const TEX0& tex0, const TEXA& texA)
{
	uint32 nPointer			= tex0.GetBufPtr();
	unsigned int nWidth		= tex0.GetWidth();
	unsigned int nHeight	= tex0.GetHeight();

	FetchImagePSMCT16((uint16*)m_pCvtBuffer, nPointer, tex0.nBufWidth, nWidth, nHeight);

	if(tex0.nColorComp == 1)
	{
		//This isn't right... TA0 isn't considered
		glPixelTransferf(GL_ALPHA_SCALE, (float)texA.nTA1);
	}
	else
	{
		glPixelTransferf(GL_ALPHA_SCALE, 0);
	}

	glPixelStorei(GL_UNPACK_ROW_LENGTH, nWidth);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB5_A1, nWidth, nHeight, 0, GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV, m_pCvtBuffer);
}

void CGSH_OpenGL::TexUploader_Psm16_Cvt(const TEX0& tex0, const TEXA& texA)
{
	//This function probably doesn't work since it doesn't unswizzle properly using FetchImagePSMCT16

	uint32 nPointer			= tex0.GetBufPtr();
	unsigned int nWidth		= tex0.GetWidth();
	unsigned int nHeight	= tex0.GetHeight();

	//No clue about this... 'lobotom.elf'
	unsigned int nSrcPitch = 0x280;
	unsigned int nDstPitch = nWidth;

	uint16* pSrc = (uint16*)(m_pRAM + nPointer);
	uint32* pDst = (uint32*)(m_pCvtBuffer);

	uint8 nTA0, nTA1;
	if(tex0.nColorComp == 1)
	{
		nTA0 = texA.nTA0;
		nTA1 = texA.nTA1;
	}
	else
	{
		nTA0 = 0;
		nTA1 = 0;
	}

	for(unsigned int j = 0; j < nHeight; j++)
	{
		for(unsigned int i = 0; i < nWidth; i++)
		{
			uint16 nPixel = pSrc[i];

			uint8 nB = (nPixel >>  0) & 0x1F;
			uint8 nG = (nPixel >>  5) & 0x1F;
			uint8 nR = (nPixel >> 10) & 0x1F;
			uint8 nA = (nPixel >> 15) & 0x01;

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

void CGSH_OpenGL::TexUploader_Psm16S_Hw(const TEX0& tex0, const TEXA& texA)
{
	uint32 nPointer			= tex0.GetBufPtr();
	unsigned int nWidth		= tex0.GetWidth();
	unsigned int nHeight	= tex0.GetHeight();

	FetchImagePSMCT16S(reinterpret_cast<uint16*>(m_pCvtBuffer), nPointer, tex0.nBufWidth, nWidth, nHeight);

	if(tex0.nColorComp == 1)
	{
		//This isn't right... TA0 isn't considered
		glPixelTransferf(GL_ALPHA_SCALE, static_cast<float>(texA.nTA1));
	}
	else
	{
		glPixelTransferf(GL_ALPHA_SCALE, 0);
	}

	glPixelStorei(GL_UNPACK_ROW_LENGTH, nWidth);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB5_A1, nWidth, nHeight, 0, GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV, m_pCvtBuffer);
}

void CGSH_OpenGL::TexUploader_Psm8(const TEX0& tex0, const TEXA& texA)
{
	uint32 nPointer			= tex0.GetBufPtr();
	unsigned int nWidth		= tex0.GetWidth();
	unsigned int nHeight	= tex0.GetHeight();
	unsigned int nDstPitch	= nWidth;
	uint8* pDst				= m_pCvtBuffer;

	CPixelIndexorPSMT8 Indexor(m_pRAM, nPointer, tex0.nBufWidth);

	for(unsigned int j = 0; j < nHeight; j++)
	{
		for(unsigned int i = 0; i < nWidth; i++)
		{
			uint8 nPixel = Indexor.GetPixel(i, j);
			pDst[i] = nPixel;
		}

		pDst += nDstPitch;
	}

	glPixelTransferf(GL_ALPHA_SCALE, 1.0f);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, nWidth);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, nWidth, nHeight, 0, GL_ALPHA, GL_UNSIGNED_BYTE, m_pCvtBuffer);
}

void CGSH_OpenGL::TexUploader_Psm4(const TEX0& tex0, const TEXA& texA)
{
	uint32 nPointer			= tex0.GetBufPtr();
	unsigned int nWidth		= tex0.GetWidth();
	unsigned int nHeight	= tex0.GetHeight();
	unsigned int nDstPitch	= nWidth;
	uint8* pDst				= m_pCvtBuffer;

	CPixelIndexorPSMT4 Indexor(m_pRAM, nPointer, tex0.nBufWidth);

	for(unsigned int j = 0; j < nHeight; j++)
	{
		for(unsigned int i = 0; i < nWidth; i++)
		{
			uint32 nPixel = Indexor.GetPixel(i, j);
			pDst[i] = static_cast<uint8>(nPixel);
		}

		pDst += nDstPitch;
	}

	glPixelTransferf(GL_ALPHA_SCALE, 1.0f);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, nWidth);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, nWidth, nHeight, 0, GL_ALPHA, GL_UNSIGNED_BYTE, m_pCvtBuffer);
}

template <uint32 nShift>
void CGSH_OpenGL::TexUploader_Psm4H(const TEX0& tex0, const TEXA& texA)
{
	uint32 nPointer			= tex0.GetBufPtr();
	unsigned int nWidth		= tex0.GetWidth();
	unsigned int nHeight	= tex0.GetHeight();
	unsigned int nDstPitch	= nWidth;
	uint8* pDst				= m_pCvtBuffer;

	CPixelIndexorPSMCT32 Indexor(m_pRAM, nPointer, tex0.nBufWidth);

	for(unsigned int j = 0; j < nHeight; j++)
	{
		for(unsigned int i = 0; i < nWidth; i++)
		{
			uint32 nPixel = Indexor.GetPixel(i, j);
			nPixel = (nPixel >> nShift) & 0x0F;
			pDst[i] = static_cast<uint8>(nPixel);
		}

		pDst += nDstPitch;
	}

	glPixelTransferf(GL_ALPHA_SCALE, 1.0f);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, nWidth);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, nWidth, nHeight, 0, GL_ALPHA, GL_UNSIGNED_BYTE, m_pCvtBuffer);
}

void CGSH_OpenGL::TexUploader_Psm8H(const TEX0& tex0, const TEXA& texA)
{
	uint32 nPointer			= tex0.GetBufPtr();
	unsigned int nWidth		= tex0.GetWidth();
	unsigned int nHeight	= tex0.GetHeight();
	unsigned int nDstPitch	= nWidth;
	uint8* pDst				= m_pCvtBuffer;

	CPixelIndexorPSMCT32 Indexor(m_pRAM, nPointer, tex0.nBufWidth);

	for(unsigned int j = 0; j < nHeight; j++)
	{
		for(unsigned int i = 0; i < nWidth; i++)
		{
			uint32 nPixel = Indexor.GetPixel(i, j);
			nPixel = (nPixel >> 24);
			pDst[i] = static_cast<uint8>(nPixel);
		}

		pDst += nDstPitch;
	}

	glPixelTransferf(GL_ALPHA_SCALE, 1.0f);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, nWidth);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, nWidth, nHeight, 0, GL_ALPHA, GL_UNSIGNED_BYTE, m_pCvtBuffer);
}

/////////////////////////////////////////////////////////////
// Texture
/////////////////////////////////////////////////////////////

CGSH_OpenGL::CTexture::CTexture()
: m_start(0)
, m_size(0)
, m_tex0(0)
, m_texture(0)
, m_live(false)
{

}

CGSH_OpenGL::CTexture::~CTexture()
{
	Free();
}

void CGSH_OpenGL::CTexture::Free()
{
	if(m_texture != 0)
	{
		glDeleteTextures(1, &m_texture);
		m_texture = 0;
		m_live = false;
	}
}

void CGSH_OpenGL::CTexture::InvalidateFromMemorySpace(uint32 nStart, uint32 nSize)
{
	if(!m_live) return;

	bool nInvalid = false;
	uint32 nEnd = nStart + nSize;
	uint32 nTexEnd = m_start + m_size;

	if((nStart >= m_start) && (nStart < nTexEnd))
	{
		nInvalid = true;
	}

	if((nEnd >= m_start) && (nEnd < nTexEnd))
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

GLuint CGSH_OpenGL::TexCache_Search(const TEX0& tex0)
{
	uint64 maskedTex0 = static_cast<uint64>(tex0) & TEX0_CLUTINFO_MASK;

	for(auto textureIterator(m_textureCache.begin());
		textureIterator != m_textureCache.end(); textureIterator++)
	{
		auto texture = *textureIterator;
		if(!texture->m_live) continue;
		if(maskedTex0 != texture->m_tex0) continue;
		m_textureCache.erase(textureIterator);
		m_textureCache.push_front(texture);
		return texture->m_texture;
	}

	return 0;
}

void CGSH_OpenGL::TexCache_Insert(const TEX0& tex0, GLuint textureHandle)
{
	auto texture = *m_textureCache.rbegin();
	texture->Free();

	texture->m_start		= tex0.GetBufPtr();
	texture->m_size			= tex0.GetBufWidth() * tex0.GetHeight() * GetPsmPixelSize(tex0.nPsm) / 8;
	texture->m_tex0			= static_cast<uint64>(tex0) & TEX0_CLUTINFO_MASK;
	texture->m_texture		= textureHandle;
	texture->m_live			= true;

	m_textureCache.pop_back();
	m_textureCache.push_front(texture);
}

void CGSH_OpenGL::TexCache_InvalidateTextures(uint32 start, uint32 size)
{
	std::for_each(std::begin(m_textureCache), std::end(m_textureCache), 
		[start, size] (TexturePtr& texture) { texture->InvalidateFromMemorySpace(start, size); });
}

void CGSH_OpenGL::TexCache_Flush()
{
	std::for_each(std::begin(m_textureCache), std::end(m_textureCache), 
		[] (TexturePtr& texture) { texture->Free(); });
}

/////////////////////////////////////////////////////////////
// Palette
/////////////////////////////////////////////////////////////

CGSH_OpenGL::CPalette::CPalette()
: m_isIDTEX4(false)
, m_cpsm(0)
, m_csa(0)
, m_texture(0)
, m_live(false)
{

}

CGSH_OpenGL::CPalette::~CPalette()
{
	Free();
}

void CGSH_OpenGL::CPalette::Free()
{
	if(m_texture != 0)
	{
		glDeleteTextures(1, &m_texture);
		m_texture = 0;
		m_live = false;
	}
}

void CGSH_OpenGL::CPalette::Invalidate(uint32 csa)
{
	if(!m_live) return;

	m_live = false;
}

/////////////////////////////////////////////////////////////
// Palette Caching
/////////////////////////////////////////////////////////////

GLuint CGSH_OpenGL::PalCache_Search(const TEX0& tex0)
{
	for(auto paletteIterator(m_paletteCache.begin());
		paletteIterator != m_paletteCache.end(); paletteIterator++)
	{
		auto palette = *paletteIterator;
		if(!palette->m_live) continue;
		if(IsPsmIDTEX4(tex0.nPsm) != palette->m_isIDTEX4) continue;
		if(tex0.nCPSM != palette->m_cpsm) continue;
		if(tex0.nCSA != palette->m_csa) continue;
		m_paletteCache.erase(paletteIterator);
		m_paletteCache.push_front(palette);
		return palette->m_texture;
	}

	return 0;
}

GLuint CGSH_OpenGL::PalCache_Search(unsigned int entryCount, const uint32* contents)
{
	for(auto paletteIterator(m_paletteCache.begin());
		paletteIterator != m_paletteCache.end(); paletteIterator++)
	{
		auto palette = *paletteIterator;
		
		if(palette->m_texture == 0) continue;
		
		unsigned int palEntryCount = palette->m_isIDTEX4 ? 16 : 256;
		if(palEntryCount != entryCount) continue;

		if(memcmp(contents, palette->m_contents, sizeof(uint32) * entryCount) != 0) continue;

		palette->m_live = true;

		m_paletteCache.erase(paletteIterator);
		m_paletteCache.push_front(palette);
		return palette->m_texture;
	}

	return 0;
}

void CGSH_OpenGL::PalCache_Insert(const TEX0& tex0, const uint32* contents, GLuint textureHandle)
{
	auto texture = *m_paletteCache.rbegin();
	texture->Free();

	unsigned int entryCount = IsPsmIDTEX4(tex0.nPsm) ? 16 : 256;

	texture->m_isIDTEX4		= IsPsmIDTEX4(tex0.nPsm);
	texture->m_cpsm			= tex0.nCPSM;
	texture->m_csa			= tex0.nCSA;
	texture->m_texture		= textureHandle;
	texture->m_live			= true;
	memcpy(texture->m_contents, contents, entryCount * sizeof(uint32));

	m_paletteCache.pop_back();
	m_paletteCache.push_front(texture);
}

void CGSH_OpenGL::PalCache_Invalidate(uint32 csa)
{
	std::for_each(std::begin(m_paletteCache), std::end(m_paletteCache),
		[csa] (PalettePtr& palette) { palette->Invalidate(csa); });
}

void CGSH_OpenGL::PalCache_Flush()
{
	std::for_each(std::begin(m_paletteCache), std::end(m_paletteCache), 
		[] (PalettePtr& palette) { palette->Free(); });
}
