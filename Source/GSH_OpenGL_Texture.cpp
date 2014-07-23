#include <assert.h>
#include <sys/stat.h>
#include <limits.h>
#include <algorithm>
#include <zlib.h>
#include "GSH_OpenGL.h"
#include "StdStream.h"
#include "bitmap/BMP.h"
#include "GsPixelFormats.h"

#define TEX0_CLUTINFO_MASK (~0xFFFFFFE000000000ULL)

static bool DoMemoryRangesOverlap(uint32 start1, uint32 size1, uint32 start2, uint32 size2)
{
	uint32 min1 = start1;
	uint32 max1 = start1 + size1;

	uint32 min2 = start2;
	uint32 max2 = start2 + size2;

	if(max1 < min2) return false;
	if(min1 > max2) return false;
	
	return true;
}

/////////////////////////////////////////////////////////////
// Texture Loading
/////////////////////////////////////////////////////////////

void CGSH_OpenGL::SetupTextureUploaders()
{
	for(unsigned int i = 0; i < PSM_MAX; i++)
	{
		m_textureUploader[i] = &CGSH_OpenGL::TexUploader_Invalid;
		m_textureUpdater[i] = &CGSH_OpenGL::TexUpdater_Invalid;
	}

	m_textureUploader[PSMCT32]		= &CGSH_OpenGL::TexUploader_Psm32;
	m_textureUploader[PSMCT24]		= &CGSH_OpenGL::TexUploader_Psm32;
	m_textureUploader[PSMCT16]		= &CGSH_OpenGL::TexUploader_Psm16<CGsPixelFormats::CPixelIndexorPSMCT16>;
	m_textureUploader[PSMCT24_UNK]	= &CGSH_OpenGL::TexUploader_Psm32;
	m_textureUploader[PSMCT16S]		= &CGSH_OpenGL::TexUploader_Psm16<CGsPixelFormats::CPixelIndexorPSMCT16S>;
	m_textureUploader[PSMT8]		= &CGSH_OpenGL::TexUploader_Psm48<CGsPixelFormats::CPixelIndexorPSMT8>;
	m_textureUploader[PSMT4]		= &CGSH_OpenGL::TexUploader_Psm48<CGsPixelFormats::CPixelIndexorPSMT4>;
	m_textureUploader[PSMT8H]		= &CGSH_OpenGL::TexUploader_Psm48H<24, 0xFF>;
	m_textureUploader[PSMT4HL]		= &CGSH_OpenGL::TexUploader_Psm48H<24, 0x0F>;
	m_textureUploader[PSMT4HH]		= &CGSH_OpenGL::TexUploader_Psm48H<28, 0x0F>;

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

void CGSH_OpenGL::PrepareTexture(const TEX0& tex0)
{
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();

	for(const auto& candidateFramebuffer : m_framebuffers)
	{
		if(!candidateFramebuffer->m_canBeUsedAsTexture) continue;

		bool canBeUsed = false;
		float offsetX = 0;

		//Case: TEX0 points at the start of a frame buffer with the same width
		if(candidateFramebuffer->m_basePtr == tex0.GetBufPtr() &&
			candidateFramebuffer->m_width == tex0.GetBufWidth())
		{
			canBeUsed = true;
		}

		//Another case: TEX0 is pointing to the start of a page within our framebuffer (BGDA does this)
		else if(candidateFramebuffer->m_basePtr <= tex0.GetBufPtr() &&
			candidateFramebuffer->m_psm == tex0.nPsm)
		{
			uint32 framebufferOffset = tex0.GetBufPtr() - candidateFramebuffer->m_basePtr;

			//Bail if offset is not aligned on a page boundary
			if((framebufferOffset & (CGsPixelFormats::PAGESIZE - 1)) != 0) continue;

			auto framebufferPageSize = GetPsmPageSize(candidateFramebuffer->m_psm);
			uint32 framebufferPageCountX = candidateFramebuffer->m_width / framebufferPageSize.first;
			uint32 framebufferPageIndex = framebufferOffset / CGsPixelFormats::PAGESIZE;

			//Bail if pointed page isn't on the first line
			if(framebufferPageIndex >= framebufferPageCountX) continue;

			canBeUsed = true;
			offsetX = -static_cast<float>(framebufferPageIndex * framebufferPageSize.first) / static_cast<float>(candidateFramebuffer->m_width);
		}

		if(canBeUsed)
		{
			//We have a winner
			glBindTexture(GL_TEXTURE_2D, candidateFramebuffer->m_texture);

			float scaleRatioX = static_cast<float>(tex0.GetWidth()) / static_cast<float>(candidateFramebuffer->m_width);
			float scaleRatioY = static_cast<float>(tex0.GetHeight()) / static_cast<float>(candidateFramebuffer->m_height);

			//If we're currently in interlaced mode, framebuffer will have twice the height
			bool halfHeight = GetCrtIsInterlaced() && GetCrtIsFrameMode();
			if(halfHeight) scaleRatioY *= 2.0f;

			glTranslatef(offsetX, 0, 0);
			glScalef(scaleRatioX, scaleRatioY, 1);

			return;
		}
	}

	auto texture = TexCache_Search(tex0);
	if(texture)
	{
		glBindTexture(GL_TEXTURE_2D, texture->m_texture);

		if(texture->HasDirtyPages())
		{
			auto texturePageSize = GetPsmPageSize(tex0.nPsm);
			uint32 pageCountX = (tex0.GetBufWidth() + texturePageSize.first - 1) / texturePageSize.first;
			
			for(unsigned int dirtyPageIndex = 0; dirtyPageIndex < CTexture::MAX_DIRTYPAGES; dirtyPageIndex++)
			{
				if(!texture->IsPageDirty(dirtyPageIndex)) continue;

				uint32 pageX = dirtyPageIndex % pageCountX;
				uint32 pageY = dirtyPageIndex / pageCountX;
				uint32 texX = pageX * texturePageSize.first;
				uint32 texY = pageY * texturePageSize.second;
				uint32 texWidth = texturePageSize.first;
				uint32 texHeight = texturePageSize.second;
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
				((this)->*(m_textureUpdater[tex0.nPsm]))(tex0, texX, texY, texWidth, texHeight);
			}

			texture->ClearDirtyPages();
		}
	}
	else
	{
		GLuint nTexture = 0;
		glGenTextures(1, &nTexture);
		glBindTexture(GL_TEXTURE_2D, nTexture);
		((this)->*(m_textureUploader[tex0.nPsm]))(tex0);
		TexCache_Insert(tex0, nTexture);
	}
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

void CGSH_OpenGL::TexUploader_Invalid(const TEX0& tex0)
{
	assert(0);
}

void CGSH_OpenGL::TexUploader_Psm32(const TEX0& tex0)
{
	unsigned int width		= tex0.GetWidth();
	unsigned int height		= tex0.GetHeight();
	uint32* dst				= reinterpret_cast<uint32*>(m_pCvtBuffer);

	CGsPixelFormats::CPixelIndexorPSMCT32 Indexor(m_pRAM, tex0.GetBufPtr(), tex0.nBufWidth);

	for(unsigned int j = 0; j < height; j++)
	{
		for(unsigned int i = 0; i < width; i++)
		{
			dst[i] = Indexor.GetPixel(i, j);
		}

		dst += width;
	}

	glTexImage2D(GL_TEXTURE_2D, 0, 4, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_pCvtBuffer);
}

template <typename IndexorType>
void CGSH_OpenGL::TexUploader_Psm16(const TEX0& tex0)
{
	unsigned int width		= tex0.GetWidth();
	unsigned int height		= tex0.GetHeight();
	uint16* dst				= reinterpret_cast<uint16*>(m_pCvtBuffer);

	IndexorType indexor(m_pRAM, tex0.GetBufPtr(), tex0.nBufWidth);

	for(unsigned int j = 0; j < height; j++)
	{
		for(unsigned int i = 0; i < width; i++)
		{
			dst[i] = indexor.GetPixel(i, j);
		}

		dst += width;
	}

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB5_A1, width, height, 0, GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV, m_pCvtBuffer);
}

template <typename IndexorType>
void CGSH_OpenGL::TexUploader_Psm48(const TEX0& tex0)
{
	unsigned int width	= tex0.GetWidth();
	unsigned int height = tex0.GetHeight();
	uint8* dst			= m_pCvtBuffer;

	IndexorType indexor(m_pRAM, tex0.GetBufPtr(), tex0.nBufWidth);

	for(unsigned int j = 0; j < height; j++)
	{
		for(unsigned int i = 0; i < width; i++)
		{
			dst[i] = indexor.GetPixel(i, j);
		}

		dst += width;
	}

	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, width, height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, m_pCvtBuffer);
}

template <uint32 shiftAmount, uint32 mask>
void CGSH_OpenGL::TexUploader_Psm48H(const TEX0& tex0)
{
	unsigned int width		= tex0.GetWidth();
	unsigned int height		= tex0.GetHeight();
	uint8* dst				= m_pCvtBuffer;

	CGsPixelFormats::CPixelIndexorPSMCT32 indexor(m_pRAM, tex0.GetBufPtr(), tex0.nBufWidth);

	for(unsigned int j = 0; j < height; j++)
	{
		for(unsigned int i = 0; i < width; i++)
		{
			uint32 pixel = indexor.GetPixel(i, j);
			pixel = (pixel >> shiftAmount) & mask;
			dst[i] = static_cast<uint8>(pixel);
		}

		dst += width;
	}

	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, width, height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, m_pCvtBuffer);
}

void CGSH_OpenGL::TexUpdater_Invalid(const TEX0& tex0, unsigned int texX, unsigned int texY, unsigned int texWidth, unsigned int texHeight)
{
	assert(0);
}

void CGSH_OpenGL::TexUpdater_Psm32(const TEX0& tex0, unsigned int texX, unsigned int texY, unsigned int texWidth, unsigned int texHeight)
{
	CGsPixelFormats::CPixelIndexorPSMCT32 indexor(m_pRAM, tex0.GetBufPtr(), tex0.nBufWidth);

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
}

template <typename IndexorType>
void CGSH_OpenGL::TexUpdater_Psm16(const TEX0& tex0, unsigned int texX, unsigned int texY, unsigned int texWidth, unsigned int texHeight)
{
	IndexorType indexor(m_pRAM, tex0.GetBufPtr(), tex0.nBufWidth);

	uint16* dst = reinterpret_cast<uint16*>(m_pCvtBuffer);
	for(unsigned int y = 0; y < texHeight; y++)
	{
		for(unsigned int x = 0; x < texWidth; x++)
		{
			dst[x] = indexor.GetPixel(texX + x, texY + y);
		}

		dst += texWidth;
	}

	glTexSubImage2D(GL_TEXTURE_2D, 0, texX, texY, texWidth, texHeight, GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV, m_pCvtBuffer);
}

template <typename IndexorType>
void CGSH_OpenGL::TexUpdater_Psm48(const TEX0& tex0, unsigned int texX, unsigned int texY, unsigned int texWidth, unsigned int texHeight)
{
	IndexorType indexor(m_pRAM, tex0.GetBufPtr(), tex0.nBufWidth);

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

	glTexSubImage2D(GL_TEXTURE_2D, 0, texX, texY, texWidth, texHeight, GL_ALPHA, GL_UNSIGNED_BYTE, m_pCvtBuffer);
}

template <uint32 shiftAmount, uint32 mask>
void CGSH_OpenGL::TexUpdater_Psm48H(const TEX0& tex0, unsigned int texX, unsigned int texY, unsigned int texWidth, unsigned int texHeight)
{
	CGsPixelFormats::CPixelIndexorPSMCT32 indexor(m_pRAM, tex0.GetBufPtr(), tex0.nBufWidth);

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

	glTexSubImage2D(GL_TEXTURE_2D, 0, texX, texY, texWidth, texHeight, GL_ALPHA, GL_UNSIGNED_BYTE, m_pCvtBuffer);
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
	ClearDirtyPages();
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
	}
	m_live = false;
	ClearDirtyPages();
}

void CGSH_OpenGL::CTexture::InvalidateFromMemorySpace(uint32 start, uint32 size)
{
	if(!m_live) return;

	auto tex0 = make_convertible<TEX0>(m_tex0);

	if(DoMemoryRangesOverlap(start, size, m_start, m_size))
	{
		auto texturePageSize = GetPsmPageSize(tex0.nPsm);

		uint32 pageCountX = (tex0.GetBufWidth() + texturePageSize.first - 1) / texturePageSize.first;
		uint32 pageCountY = (tex0.GetHeight() + texturePageSize.second - 1) / texturePageSize.second;

		//Find the pages that are touched by this transfer
		uint32 pageStart = (start < m_start) ? 0 : ((start - m_start) / CGsPixelFormats::PAGESIZE);
		uint32 pageCount = std::min<uint32>(pageCountX * pageCountY, (size + CGsPixelFormats::PAGESIZE - 1) / CGsPixelFormats::PAGESIZE);

		for(unsigned int i = 0; i < pageCount; i++)
		{
			SetPageDirty(pageStart + i);
		}

		//Wouldn't make sense to go through here and not have at least a dirty page
		assert(HasDirtyPages());
	}
}

bool CGSH_OpenGL::CTexture::IsPageDirty(uint32 pageIndex) const
{
	assert(pageIndex < sizeof(m_dirtyPages) * 8);
	unsigned int dirtyPageSection = pageIndex / (sizeof(m_dirtyPages[0]) * 8);
	unsigned int dirtyPageIndex = pageIndex % (sizeof(m_dirtyPages[0]) * 8);
	return (m_dirtyPages[dirtyPageSection] & (1ULL << dirtyPageIndex)) != 0;
}

void CGSH_OpenGL::CTexture::SetPageDirty(uint32 pageIndex)
{
	assert(pageIndex < sizeof(m_dirtyPages) * 8);
	unsigned int dirtyPageSection = pageIndex / (sizeof(m_dirtyPages[0]) * 8);
	unsigned int dirtyPageIndex = pageIndex % (sizeof(m_dirtyPages[0]) * 8);
	m_dirtyPages[dirtyPageSection] |= (1ULL << dirtyPageIndex);
}

bool CGSH_OpenGL::CTexture::HasDirtyPages() const
{
	DirtyPageHolder dirtyStatus = 0;
	for(unsigned int i = 0; i < MAX_DIRTYPAGES_SECTIONS; i++)
	{
		dirtyStatus |= m_dirtyPages[i];
	}
	return (dirtyStatus != 0);
}

void CGSH_OpenGL::CTexture::ClearDirtyPages()
{
	memset(m_dirtyPages, 0, sizeof(m_dirtyPages));
}

/////////////////////////////////////////////////////////////
// Texture Caching
/////////////////////////////////////////////////////////////

CGSH_OpenGL::CTexture* CGSH_OpenGL::TexCache_Search(const TEX0& tex0)
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
		return texture.get();
	}

	return nullptr;
}

void CGSH_OpenGL::TexCache_Insert(const TEX0& tex0, GLuint textureHandle)
{
	auto texture = *m_textureCache.rbegin();
	texture->Free();

	auto transferPageSize = GetPsmPageSize(tex0.nPsm);
	uint32 pageCountX = (tex0.GetBufWidth() + transferPageSize.first - 1) / transferPageSize.first;
	uint32 pageCountY = (tex0.GetHeight() + transferPageSize.second - 1) / transferPageSize.second;

	texture->m_start		= tex0.GetBufPtr();
	texture->m_size			= pageCountX * pageCountY * CGsPixelFormats::PAGESIZE;
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
