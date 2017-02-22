#include <assert.h>
#include <sys/stat.h>
#include <limits.h>
#include <algorithm>
#include <zlib.h>
#include "GSH_OpenGL.h"
#include "StdStream.h"
#include "bitmap/BMP.h"
#include "../GsPixelFormats.h"

#define TEX0_CLUTINFO_MASK (~0xFFFFFFE000000000ULL)

/////////////////////////////////////////////////////////////
// Texture Loading
/////////////////////////////////////////////////////////////

void CGSH_OpenGL::SetupTextureUpdaters()
{
	for(unsigned int i = 0; i < PSM_MAX; i++)
	{
		m_textureUpdater[i] = &CGSH_OpenGL::TexUpdater_Invalid;
	}

	m_textureUpdater[PSMCT32]		= &CGSH_OpenGL::TexUpdater_Psm32;
	m_textureUpdater[PSMCT24]		= &CGSH_OpenGL::TexUpdater_Psm32;
	m_textureUpdater[PSMCT16]		= &CGSH_OpenGL::TexUpdater_Psm16<CGsPixelFormats::CPixelIndexorPSMCT16>;
	m_textureUpdater[PSMCT24_UNK]	= &CGSH_OpenGL::TexUpdater_Psm32;
	m_textureUpdater[PSMCT16S]		= &CGSH_OpenGL::TexUpdater_Psm16<CGsPixelFormats::CPixelIndexorPSMCT16S>;
	m_textureUpdater[PSMT8]			= &CGSH_OpenGL::TexUpdater_Psm48<CGsPixelFormats::CPixelIndexorPSMT8>;
	m_textureUpdater[PSMT4]			= &CGSH_OpenGL::TexUpdater_Psm48<CGsPixelFormats::CPixelIndexorPSMT4>;
	m_textureUpdater[PSMT8H]		= &CGSH_OpenGL::TexUpdater_Psm48H<24, 0xFF>;
	m_textureUpdater[PSMT4HL]		= &CGSH_OpenGL::TexUpdater_Psm48H<24, 0x0F>;
	m_textureUpdater[PSMT4HH]		= &CGSH_OpenGL::TexUpdater_Psm48H<28, 0x0F>;
}

bool CGSH_OpenGL::IsCompatibleFramebufferPSM(unsigned int psmFb, unsigned int psmTex)
{
	if(psmTex == CGSHandler::PSMCT24)
	{
		return (psmFb == CGSHandler::PSMCT24) || (psmFb == CGSHandler::PSMCT32);
	}
	else
	{
		return (psmFb == psmTex);
	}
}

uint32 CGSH_OpenGL::GetFramebufferBitDepth(uint32 psm)
{
	if((psm == PSMCT32) || (psm == PSMCT24))
	{
		return 32;
	}
	else if((psm == PSMCT16) || (psm == PSMCT16S))
	{
		return 16;
	}
	else
	{
		assert(false);
		return 32;
	}
}

CGSH_OpenGL::TEXTUREFORMAT_INFO CGSH_OpenGL::GetTextureFormatInfo(uint32 psm)
{
	switch(psm)
	{
	case PSMCT32:
	case PSMCT24:
	case PSMCT24_UNK:
		return TEXTUREFORMAT_INFO { GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE };
	case PSMCT16:
	case PSMCT16S:
		return TEXTUREFORMAT_INFO { GL_RGB5_A1, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1 };
	case PSMT8:
	case PSMT4:
	case PSMT8H:
	case PSMT4HL:
	case PSMT4HH:
		return TEXTUREFORMAT_INFO { GL_R8, GL_RED, GL_UNSIGNED_BYTE };
	default:
		assert(false);
		return TEXTUREFORMAT_INFO { GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE };
	}
}

CGSH_OpenGL::TEXTURE_INFO CGSH_OpenGL::PrepareTexture(const TEX0& tex0)
{
	TEXTURE_INFO texInfo;

	for(const auto& candidateFramebuffer : m_framebuffers)
	{
		bool canBeUsed = false;
		float offsetX = 0;

		//Case: TEX0 points at the start of a frame buffer with the same width
		if(candidateFramebuffer->m_basePtr == tex0.GetBufPtr() &&
			candidateFramebuffer->m_width == tex0.GetBufWidth() &&
			IsCompatibleFramebufferPSM(candidateFramebuffer->m_psm, tex0.nPsm))
		{
			canBeUsed = true;
		}

		//Another case: TEX0 is pointing to the start of a page within our framebuffer (BGDA does this)
		else if(candidateFramebuffer->m_basePtr <= tex0.GetBufPtr() &&
			candidateFramebuffer->m_width == tex0.GetBufWidth() &&
			candidateFramebuffer->m_psm == tex0.nPsm)
		{
			uint32 framebufferOffset = tex0.GetBufPtr() - candidateFramebuffer->m_basePtr;

			//Bail if offset is not aligned on a page boundary
			if((framebufferOffset & (CGsPixelFormats::PAGESIZE - 1)) != 0) continue;

			auto framebufferPageSize = CGsPixelFormats::GetPsmPageSize(candidateFramebuffer->m_psm);
			uint32 framebufferPageCountX = candidateFramebuffer->m_width / framebufferPageSize.first;
			uint32 framebufferPageIndex = framebufferOffset / CGsPixelFormats::PAGESIZE;

			//Bail if pointed page isn't on the first line
			if(framebufferPageIndex >= framebufferPageCountX) continue;

			canBeUsed = true;
			offsetX = static_cast<float>(framebufferPageIndex * framebufferPageSize.first) / static_cast<float>(candidateFramebuffer->m_width);
		}

		if(canBeUsed)
		{
			CommitFramebufferDirtyPages(candidateFramebuffer, 0, tex0.GetHeight());
			if(m_multisampleEnabled)
			{
				ResolveFramebufferMultisample(candidateFramebuffer, m_fbScale);
			}

			float scaleRatioX = static_cast<float>(tex0.GetWidth()) / static_cast<float>(candidateFramebuffer->m_width);
			float scaleRatioY = static_cast<float>(tex0.GetHeight()) / static_cast<float>(candidateFramebuffer->m_height);

			texInfo.textureHandle = candidateFramebuffer->m_texture;
			texInfo.offsetX       = offsetX;
			texInfo.scaleRatioX   = scaleRatioX;
			texInfo.scaleRatioY   = scaleRatioY;

			return texInfo;
		}
	}

	auto texture = m_textureCache.Search(tex0);
	if(!texture)
	{
		//Validate texture dimensions to prevent problems
		auto texWidth = tex0.GetWidth();
		auto texHeight = tex0.GetHeight();
		assert(texWidth <= 1024);
		assert(texHeight <= 1024);
		texWidth = std::min<uint32>(texWidth, 1024);
		texHeight = std::min<uint32>(texHeight, 1024);
		auto texFormat = GetTextureFormatInfo(tex0.nPsm);

		{
			auto textureHandle = Framework::OpenGl::CTexture::Create();
			glBindTexture(GL_TEXTURE_2D, textureHandle);
			glTexStorage2D(GL_TEXTURE_2D, 1, texFormat.internalFormat, texWidth, texHeight);
			CHECKGLERROR();
			m_textureCache.Insert(tex0, std::move(textureHandle));
		}

		texture = m_textureCache.Search(tex0);
		texture->m_cachedArea.Invalidate(0, RAMSIZE);
	}

	texInfo.textureHandle = texture->m_textureHandle;

	glBindTexture(GL_TEXTURE_2D, texture->m_textureHandle);
	auto& cachedArea = texture->m_cachedArea;
	auto texturePageSize = CGsPixelFormats::GetPsmPageSize(tex0.nPsm);
	auto areaRect = cachedArea.GetAreaPageRect();

	while(cachedArea.HasDirtyPages())
	{
		auto dirtyRect = cachedArea.GetDirtyPageRect();
		assert((dirtyRect.width != 0) && (dirtyRect.height != 0));
		cachedArea.ClearDirtyPages(dirtyRect);

		uint32 texX = dirtyRect.x * texturePageSize.first;
		uint32 texY = dirtyRect.y * texturePageSize.second;
		uint32 texWidth = dirtyRect.width * texturePageSize.first;
		uint32 texHeight = dirtyRect.height * texturePageSize.second;
		if(texX >= tex0.GetWidth()) continue;
		if(texY >= tex0.GetHeight()) continue;
		//assert(texX < tex0.GetWidth());
		//assert(texY < tex0.GetHeight());
		if((texX + texWidth) > tex0.GetWidth())
		{
			texWidth = tex0.GetWidth() - texX;
		}
		if((texY + texHeight) > tex0.GetHeight())
		{
			texHeight = tex0.GetHeight() - texY;
		}
		((this)->*(m_textureUpdater[tex0.nPsm]))(tex0.GetBufPtr(), tex0.nBufWidth, texX, texY, texWidth, texHeight);
	}

	cachedArea.ClearDirtyPages();

	return texInfo;
}

GLuint CGSH_OpenGL::PreparePalette(const TEX0& tex0)
{
	GLuint textureHandle = PalCache_Search(tex0);
	if(textureHandle != 0)
	{
		return textureHandle;
	}

	uint32 convertedClut[256];
	unsigned int entryCount = CGsPixelFormats::IsPsmIDTEX4(tex0.nPsm) ? 16 : 256;

	if(CGsPixelFormats::IsPsmIDTEX4(tex0.nPsm))
	{
		if(tex0.nCPSM == PSMCT32 || tex0.nCPSM == PSMCT24)
		{
			assert(tex0.nCSA < 16);
			uint32 clutOffset = (tex0.nCSA & 0xF) * 16;

			for(unsigned int i = 0; i < 16; i++)
			{
				uint32 color = 
					(static_cast<uint16>(m_pCLUT[i + clutOffset + 0x000])) | 
					(static_cast<uint16>(m_pCLUT[i + clutOffset + 0x100]) << 16);
				convertedClut[i] = color;
			}
		}
		else if(tex0.nCPSM == PSMCT16 || tex0.nCPSM == PSMCT16S)
		{
			//CSA is 5-bit, shouldn't go over 31
			assert(tex0.nCSA < 32);
			uint32 clutOffset = tex0.nCSA * 16;

			for(unsigned int i = 0; i < 16; i++)
			{
				convertedClut[i] = RGBA16ToRGBA32(m_pCLUT[i + clutOffset]);
			}
		}
		else
		{
			assert(false);
		}
	}
	else if(CGsPixelFormats::IsPsmIDTEX8(tex0.nPsm))
	{
		assert(tex0.nCSA == 0);

		if(tex0.nCPSM == PSMCT32 || tex0.nCPSM == PSMCT24)
		{
			for(unsigned int i = 0; i < 256; i++)
			{
				uint32 color = 
					(static_cast<uint16>(m_pCLUT[i + 0x000])) | 
					(static_cast<uint16>(m_pCLUT[i + 0x100]) << 16);
				convertedClut[i] = color;
			}
		}
		else if(tex0.nCPSM == PSMCT16 || tex0.nCPSM == PSMCT16S)
		{
			for(unsigned int i = 0; i < 256; i++)
			{
				convertedClut[i] = RGBA16ToRGBA32(m_pCLUT[i]);
			}
		}
		else
		{
			assert(false);
		}
	}

	textureHandle = PalCache_Search(entryCount, convertedClut);
	if(textureHandle != 0)
	{
		return textureHandle;
	}

	glGenTextures(1, &textureHandle);
	glBindTexture(GL_TEXTURE_2D, textureHandle);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, entryCount, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, convertedClut);

	PalCache_Insert(tex0, convertedClut, textureHandle);

	return textureHandle;
}

void CGSH_OpenGL::DumpTexture(unsigned int nWidth, unsigned int nHeight, uint32 checksum)
{
#ifdef _WIN32
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

void CGSH_OpenGL::TexUpdater_Invalid(uint32 bufPtr, uint32 bufWidth, unsigned int texX, unsigned int texY, unsigned int texWidth, unsigned int texHeight)
{
	assert(0);
}

void CGSH_OpenGL::TexUpdater_Psm32(uint32 bufPtr, uint32 bufWidth, unsigned int texX, unsigned int texY, unsigned int texWidth, unsigned int texHeight)
{
	CGsPixelFormats::CPixelIndexorPSMCT32 indexor(m_pRAM, bufPtr, bufWidth);

	uint32* dst = reinterpret_cast<uint32*>(m_pCvtBuffer);
	for(unsigned int y = 0; y < texHeight; y++)
	{
		for(unsigned int x = 0; x < texWidth; x++)
		{
			dst[x] = indexor.GetPixel(texX + x, texY + y);
		}

		dst += texWidth;
	}

	glTexSubImage2D(GL_TEXTURE_2D, 0, texX, texY, texWidth, texHeight, GL_RGBA, GL_UNSIGNED_BYTE, m_pCvtBuffer);
	CHECKGLERROR();
}

template <typename IndexorType>
void CGSH_OpenGL::TexUpdater_Psm16(uint32 bufPtr, uint32 bufWidth, unsigned int texX, unsigned int texY, unsigned int texWidth, unsigned int texHeight)
{
	IndexorType indexor(m_pRAM, bufPtr, bufWidth);

	auto dst = reinterpret_cast<uint16*>(m_pCvtBuffer);
	for(unsigned int y = 0; y < texHeight; y++)
	{
		for(unsigned int x = 0; x < texWidth; x++)
		{
			auto pixel = indexor.GetPixel(texX + x, texY + y);
			auto cvtPixel = 
				(((pixel & 0x001F) >>  0) << 11) |	//R
				(((pixel & 0x03E0) >>  5) <<  6) |	//G
				(((pixel & 0x7C00) >> 10) <<  1) |	//B
				(pixel >> 15);						//A
			dst[x] = cvtPixel;
		}

		dst += texWidth;
	}

	glTexSubImage2D(GL_TEXTURE_2D, 0, texX, texY, texWidth, texHeight, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, m_pCvtBuffer);
	CHECKGLERROR();
}

template <typename IndexorType>
void CGSH_OpenGL::TexUpdater_Psm48(uint32 bufPtr, uint32 bufWidth, unsigned int texX, unsigned int texY, unsigned int texWidth, unsigned int texHeight)
{
	IndexorType indexor(m_pRAM, bufPtr, bufWidth);

	uint8* dst = m_pCvtBuffer;
	for(unsigned int y = 0; y < texHeight; y++)
	{
		for(unsigned int x = 0; x < texWidth; x++)
		{
			uint8 pixel = indexor.GetPixel(texX + x, texY + y);
			dst[x] = pixel;
		}

		dst += texWidth;
	}

	glTexSubImage2D(GL_TEXTURE_2D, 0, texX, texY, texWidth, texHeight, GL_RED, GL_UNSIGNED_BYTE, m_pCvtBuffer);
	CHECKGLERROR();
}

template <uint32 shiftAmount, uint32 mask>
void CGSH_OpenGL::TexUpdater_Psm48H(uint32 bufPtr, uint32 bufWidth, unsigned int texX, unsigned int texY, unsigned int texWidth, unsigned int texHeight)
{
	CGsPixelFormats::CPixelIndexorPSMCT32 indexor(m_pRAM, bufPtr, bufWidth);

	uint8* dst = m_pCvtBuffer;
	for(unsigned int y = 0; y < texHeight; y++)
	{
		for(unsigned int x = 0; x < texWidth; x++)
		{
			uint32 pixel = indexor.GetPixel(texX + x, texY + y);
			pixel = (pixel >> shiftAmount) & mask;
			dst[x] = static_cast<uint8>(pixel);
		}

		dst += texWidth;
	}

	glTexSubImage2D(GL_TEXTURE_2D, 0, texX, texY, texWidth, texHeight, GL_RED, GL_UNSIGNED_BYTE, m_pCvtBuffer);
	CHECKGLERROR();
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
		if(CGsPixelFormats::IsPsmIDTEX4(tex0.nPsm) != palette->m_isIDTEX4) continue;
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

	unsigned int entryCount = CGsPixelFormats::IsPsmIDTEX4(tex0.nPsm) ? 16 : 256;

	texture->m_isIDTEX4		= CGsPixelFormats::IsPsmIDTEX4(tex0.nPsm);
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
