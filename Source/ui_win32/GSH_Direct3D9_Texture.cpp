#include <assert.h>
#include <zlib.h>
#include <sys/stat.h>
#include <d3dx9.h>
#include "GSH_Direct3D9.h"
#include "../gs/GsPixelFormats.h"

CGSH_Direct3D9::TEXTURE_INFO CGSH_Direct3D9::LoadTexture(const TEX0& tex0, const TEX1& tex1, const CLAMP& clamp)
{
	TEXTURE_INFO result;

	{
		D3DXMATRIX textureMatrix;
		D3DXMatrixIdentity(&textureMatrix);
		D3DXMatrixScaling(&textureMatrix, 1, 1, 1);
		m_device->SetTransform(D3DTS_TEXTURE0, &textureMatrix);
	}

	for(const auto& candidateFramebuffer : m_framebuffers)
	{
		if(candidateFramebuffer->m_basePtr == tex0.GetBufPtr() &&
			candidateFramebuffer->m_width == tex0.GetBufWidth() &&
			candidateFramebuffer->m_canBeUsedAsTexture)
		{
			float scaleRatioX = static_cast<float>(tex0.GetWidth()) / static_cast<float>(candidateFramebuffer->m_width);
			float scaleRatioY = static_cast<float>(tex0.GetHeight()) / static_cast<float>(candidateFramebuffer->m_height);

			{
				D3DXMATRIX textureMatrix;
				D3DXMatrixIdentity(&textureMatrix);
				D3DXMatrixScaling(&textureMatrix, scaleRatioX, scaleRatioY, 1);
				m_device->SetTransform(D3DTS_TEXTURE0, &textureMatrix);
			}

			result.texture = candidateFramebuffer->m_renderTarget;
			result.isRenderTarget = true;
			result.renderTargetWidth = candidateFramebuffer->m_width;
			result.renderTargetHeight = candidateFramebuffer->m_height;

			return result;
		}
	}

	result.texture = TexCache_SearchLive(tex0);
	if(!result.texture.IsEmpty()) return result;

	auto texA = make_convertible<TEXA>(m_nReg[GS_REG_TEXA]);

	uint32 textureChecksum = 0; 
	switch(tex0.nPsm)
	{
	case PSMT8:
		textureChecksum = ConvertTexturePsm8(tex0, texA);
		break;
	case PSMT4:
		textureChecksum = ConvertTexturePsm4(tex0, texA);
		break;
	case PSMT4HL:
		textureChecksum = ConvertTexturePsm4H<24>(tex0, texA);
		break;
	case PSMT4HH:
		textureChecksum = ConvertTexturePsm4H<28>(tex0, texA);
		break;
	case PSMT8H:
		textureChecksum = ConvertTexturePsm8H(tex0, texA);
		break;
	case PSMCT16:
		textureChecksum = ConvertTexturePsmct16(tex0, texA);
		break;
	}

	if(textureChecksum)
	{
		//Check if we don't already have it somewhere...
		result.texture = TexCache_SearchDead(tex0, textureChecksum);
		if(!result.texture.IsEmpty())
		{
			return result;
		}
	}

	uint32 width		= tex0.GetWidth();
	uint32 height		= tex0.GetHeight();

	HRESULT resultCode = S_OK;
	resultCode = m_device->CreateTexture(width, height, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &result.texture, NULL);
	assert(SUCCEEDED(resultCode));

	switch(tex0.nPsm)
	{
	case PSMCT32:
	case PSMCT24:
		TexUploader_Psm32(tex0, texA, result.texture);
		break;
	case PSMT8:
	case PSMT8H:
	case PSMT4:
	case PSMT4HL:
	case PSMT4HH:
	case PSMCT16:
		UploadConversionBuffer(tex0, texA, result.texture);
		break;
	default:
		assert(0);
		break;
	}

	TexCache_Insert(tex0, result.texture, textureChecksum);

	return result;
}

uint32 CGSH_Direct3D9::Color_Ps2ToDx9(uint32 color)
{
	return D3DCOLOR_ARGB(
		(color >> 24) & 0xFF, 
		(color >>  0) & 0xFF,
		(color >>  8) & 0xFF,
		(color >> 16) & 0xFF);
}

uint32 CGSH_Direct3D9::RGBA16ToRGBA32(uint16 nColor)
{
	return (nColor & 0x8000 ? 0xFF000000 : 0) | ((nColor & 0x7C00) << 9) | ((nColor & 0x03E0) << 6) | ((nColor & 0x001F) << 3);
}

void CGSH_Direct3D9::FlattenClut(const TEX0& tex0, uint32* dstClut)
{
	unsigned int entryCount = CGsPixelFormats::IsPsmIDTEX4(tex0.nPsm) ? 16 : 256;

	if(CGsPixelFormats::IsPsmIDTEX4(tex0.nPsm))
	{
		uint32 clutOffset = tex0.nCSA * 16;

		if(tex0.nCPSM == PSMCT32)
		{
//			assert(tex0.nCSA < 16);

			for(unsigned int i = 0; i < 16; i++)
			{
				uint32 color = 
					(static_cast<uint16>(m_pCLUT[i + clutOffset + 0x000])) | 
					(static_cast<uint16>(m_pCLUT[i + clutOffset + 0x100]) << 16);
				uint32 alpha = MulBy2Clamp(color >> 24);
				color &= ~0xFF000000;
				color |= alpha << 24;
				dstClut[i] = Color_Ps2ToDx9(color);
			}
		}
		else
		{
			assert(tex0.nCSA < 32);

			for(unsigned int i = 0; i < 16; i++)
			{
				uint32 color = RGBA16ToRGBA32(m_pCLUT[i + clutOffset]);
				dstClut[i] = Color_Ps2ToDx9(color);
			}
		}
	}
	else if(CGsPixelFormats::IsPsmIDTEX8(tex0.nPsm))
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
				dstClut[i] = Color_Ps2ToDx9(color);
			}
		}
		else
		{
			for(unsigned int i = 0; i < 256; i++)
			{
				uint32 color = RGBA16ToRGBA32(m_pCLUT[i]);
				dstClut[i] = Color_Ps2ToDx9(color);
			}
		}
	}
}

void CGSH_Direct3D9::TexUploader_Psm32(const TEX0& tex0, const TEXA& texA, TexturePtr texture)
{
	uint32 nPointer			= tex0.GetBufPtr();
	unsigned int nWidth		= tex0.GetWidth();
	unsigned int nHeight	= tex0.GetHeight();
//	unsigned int nDstPitch	= nWidth;
//	uint32* pDst			= reinterpret_cast<uint32*>(m_pCvtBuffer);

	HRESULT result;
	D3DLOCKED_RECT rect;
	result = texture->LockRect(0, &rect, NULL, 0);

	uint32* pDst			= reinterpret_cast<uint32*>(rect.pBits);
	unsigned int nDstPitch	= rect.Pitch / 4;

	CGsPixelFormats::CPixelIndexorPSMCT32 Indexor(m_pRAM, nPointer, tex0.nBufWidth);

	for(unsigned int j = 0; j < nHeight; j++)
	{
		for(unsigned int i = 0; i < nWidth; i++)
		{
			pDst[i] = Color_Ps2ToDx9(Indexor.GetPixel(i, j));
		}

		pDst += nDstPitch;
	}

	result = texture->UnlockRect(0);
	assert(result == S_OK);
}

uint32 CGSH_Direct3D9::ConvertTexturePsm8(const TEX0& tex0, const TEXA& texA)
{
	unsigned int width		= tex0.GetWidth();
	unsigned int height		= tex0.GetHeight();
	unsigned int dstPitch	= width;
	uint32* dst				= reinterpret_cast<uint32*>(m_cvtBuffer);
	uint32 checksum			= crc32(0, NULL, 0);

	uint32 clut[256];
	FlattenClut(tex0, clut);

	CGsPixelFormats::CPixelIndexorPSMT8 indexor(m_pRAM, tex0.GetBufPtr(), tex0.nBufWidth);

	uint32 bufWidthBytes = tex0.nBufWidth * 64;

	for(unsigned int j = 0; j < height; j++)
	{
		for(unsigned int i = 0; i < width; i++)
		{
			if(i <= bufWidthBytes)
			{
				uint32 pixel = indexor.GetPixel(i, j);
				dst[i] = clut[pixel];
			}
			else
			{
				dst[i] = 0;
			}
		}

		checksum = crc32(checksum, reinterpret_cast<Bytef*>(dst), sizeof(uint32) * width);
		dst += dstPitch;
	}

	return checksum;
}

uint32 CGSH_Direct3D9::ConvertTexturePsm8H(const TEX0& tex0, const TEXA& texA)
{
	unsigned int width		= tex0.GetWidth();
	unsigned int height		= tex0.GetHeight();
	unsigned int dstPitch	= width;
	uint32* dst				= reinterpret_cast<uint32*>(m_cvtBuffer);
	uint32 checksum			= crc32(0, NULL, 0);

	uint32 clut[256];
	FlattenClut(tex0, clut);

	CGsPixelFormats::CPixelIndexorPSMCT32 indexor(m_pRAM, tex0.GetBufPtr(), tex0.nBufWidth);

	uint32 bufWidthBytes = tex0.nBufWidth * 64;

	for(unsigned int j = 0; j < height; j++)
	{
		for(unsigned int i = 0; i < width; i++)
		{
			if(i <= bufWidthBytes)
			{
				uint32 pixel = indexor.GetPixel(i, j);
				pixel >>= 24;
				dst[i] = clut[pixel];
			}
			else
			{
				dst[i] = 0;
			}
		}

		checksum = crc32(checksum, reinterpret_cast<Bytef*>(dst), sizeof(uint32) * width);
		dst += dstPitch;
	}

	return checksum;
}

uint32 CGSH_Direct3D9::ConvertTexturePsmct16(const TEX0& tex0, const TEXA& texA)
{
	unsigned int width = tex0.GetWidth();
	unsigned int height = tex0.GetHeight();
	unsigned int dstPitch = width;
	uint32* dst = reinterpret_cast<uint32*>(m_cvtBuffer);
	uint32 checksum = crc32(0, NULL, 0);

	uint8 nTA0, nTA1;
	if (tex0.nColorComp == 1)
	{
		nTA0 = texA.nTA0;
		nTA1 = texA.nTA1;
	}
	else
	{
		nTA0 = 0;
		nTA1 = 0;
	}

	CGsPixelFormats::CPixelIndexorPSMCT16S indexor(m_pRAM, tex0.GetBufPtr(), tex0.nBufWidth);

	for (unsigned int j = 0; j < height; j++)
	{
		for (unsigned int i = 0; i < width; i++)
		{
			uint16 pixel = indexor.GetPixel(i, j);
			uint8 nB = (pixel >> 0) & 0x1F;
			uint8 nG = (pixel >> 5) & 0x1F;
			uint8 nR = (pixel >> 10) & 0x1F;
			uint8 nA = (pixel >> 15) & 0x01;

			nB <<= 3;
			nG <<= 3;
			nR <<= 3;
			nA = (nA == 0) ? nTA0 : nTA1;

			*(((uint8*)&dst[i]) + 0) = nB;
			*(((uint8*)&dst[i]) + 1) = nG;
			*(((uint8*)&dst[i]) + 2) = nR;
			*(((uint8*)&dst[i]) + 3) = nA;
		}

		checksum = crc32(checksum, reinterpret_cast<Bytef*>(dst), sizeof(uint32)* width);
		dst += dstPitch;
	}

	return checksum;
}

uint32 CGSH_Direct3D9::ConvertTexturePsm4(const TEX0& tex0, const TEXA& texA)
{
	unsigned int width		= tex0.GetWidth();
	unsigned int height		= tex0.GetHeight();
	unsigned int dstPitch	= width;
	uint32* dst				= reinterpret_cast<uint32*>(m_cvtBuffer);
	uint32 checksum			= crc32(0, NULL, 0);

	uint32 clut[256];
	FlattenClut(tex0, clut);

	CGsPixelFormats::CPixelIndexorPSMT4 indexor(m_pRAM, tex0.GetBufPtr(), tex0.nBufWidth);

	for(unsigned int j = 0; j < height; j++)
	{
		for(unsigned int i = 0; i < width; i++)
		{
			uint32 pixel = indexor.GetPixel(i, j);
			dst[i] = clut[pixel];
		}

		checksum = crc32(checksum, reinterpret_cast<Bytef*>(dst), sizeof(uint32) * width);
		dst += dstPitch;
	}

	return checksum;
}

template <uint32 shiftAmount>
uint32 CGSH_Direct3D9::ConvertTexturePsm4H(const TEX0& tex0, const TEXA& texA)
{
	unsigned int width		= tex0.GetWidth();
	unsigned int height		= tex0.GetHeight();
	unsigned int dstPitch	= width;
	uint32* dst				= reinterpret_cast<uint32*>(m_cvtBuffer);
	uint32 checksum			= crc32(0, NULL, 0);

	uint32 clut[256];
	FlattenClut(tex0, clut);

	CGsPixelFormats::CPixelIndexorPSMCT32 indexor(m_pRAM, tex0.GetBufPtr(), tex0.nBufWidth);

	for(unsigned int j = 0; j < height; j++)
	{
		for(unsigned int i = 0; i < width; i++)
		{
			uint32 pixel = indexor.GetPixel(i, j);
			pixel >>= shiftAmount;
			pixel &= 0x0F;
			dst[i] = clut[pixel];
		}

		checksum = crc32(checksum, reinterpret_cast<Bytef*>(dst), sizeof(uint32) * width);
		dst += dstPitch;
	}

	return checksum;
}

void CGSH_Direct3D9::UploadConversionBuffer(const TEX0& tex0, const TEXA& texA, TexturePtr texture)
{
	unsigned int width	= tex0.GetWidth();
	unsigned int height	= tex0.GetHeight();

	HRESULT result;
	D3DLOCKED_RECT rect;
	result = texture->LockRect(0, &rect, NULL, 0);
	assert(result == S_OK);

	uint8* srcPtr = reinterpret_cast<uint8*>(m_cvtBuffer);
	uint8* dstPtr = reinterpret_cast<uint8*>(rect.pBits);
	for(unsigned int i = 0; i < height; i++)
	{
		memcpy(dstPtr, srcPtr, width * sizeof(uint32));
		srcPtr += width * sizeof(uint32);
		dstPtr += rect.Pitch;
	}

	result = texture->UnlockRect(0);
	assert(result == S_OK);
}

/////////////////////////////////////////////////////////////
// Texture
/////////////////////////////////////////////////////////////

CGSH_Direct3D9::CCachedTexture::CCachedTexture() 
: m_nStart(0)
, m_nSize(0)
, m_nCLUTAddress(0)
, m_nTex0(0)
, m_nTexClut(0)
, m_nIsCSM2(false)
, m_texture(NULL)
, m_checksum(0)
, m_live(false)
{

}

CGSH_Direct3D9::CCachedTexture::~CCachedTexture()
{
	Free();
}

void CGSH_Direct3D9::CCachedTexture::Free()
{
	m_texture.Reset();
	m_live = false;
}

void CGSH_Direct3D9::CCachedTexture::InvalidateFromMemorySpace(uint32 nStart, uint32 nSize)
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

CGSH_Direct3D9::TexturePtr CGSH_Direct3D9::TexCache_SearchLive(const TEX0& tex0)
{
	for(auto textureIterator(m_cachedTextures.begin());
		textureIterator != m_cachedTextures.end(); textureIterator++)
	{
		auto texture = *textureIterator;
		if(!texture->m_live) continue;
		//if(!texture.IsValid()) continue;
		//if(m_TexCache[i].m_nStart != pTex0->GetBufPtr()) continue;
		//if(m_TexCache[i].m_nCLUTAddress != pTex0->GetCLUTPtr()) continue;
		if(static_cast<uint64>(tex0) != texture->m_nTex0) continue;
//		if(texture->m_nIsCSM2)
//		{
//			if(((*(uint64*)GetTexClut()) & 0x3FFFFF) != texture->m_nTexClut) continue;
//		}
		m_cachedTextures.erase(textureIterator);
		m_cachedTextures.push_front(texture);
		return texture->m_texture;
	}

	return TexturePtr();
}

CGSH_Direct3D9::TexturePtr CGSH_Direct3D9::TexCache_SearchDead(const TEX0& tex0, uint32 checksum)
{
	for(auto textureIterator(m_cachedTextures.begin());
		textureIterator != m_cachedTextures.end(); textureIterator++)
	{
		auto& texture = *textureIterator;
		if(texture->m_texture.IsEmpty()) continue;
		if(texture->m_checksum != checksum) continue;
		if(texture->m_nTex0 != *reinterpret_cast<const uint64*>(&tex0)) continue;
		texture->m_live = true;
		m_cachedTextures.erase(textureIterator);
		m_cachedTextures.push_front(texture);
		return texture->m_texture;
	}

	return TexturePtr();
}

void CGSH_Direct3D9::TexCache_Insert(const TEX0& tex0, const TexturePtr& texture, uint32 checksum)
{
	auto cachedTexture = *m_cachedTextures.rbegin();
	cachedTexture->Free();

	cachedTexture->m_nStart			= tex0.GetBufPtr();
	cachedTexture->m_nSize			= tex0.GetBufWidth() * tex0.GetHeight() * CGsPixelFormats::GetPsmPixelSize(tex0.nPsm) / 8;
//	cachedTexture->m_nCLUTAddress	= pTex0->GetCLUTPtr();
	cachedTexture->m_nTex0			= static_cast<const uint64>(tex0);
//	cachedTexture->m_nTexClut		= (*(uint64*)GetTexClut()) & 0x3FFFFF;
	cachedTexture->m_nTexClut		= 0;
	cachedTexture->m_nIsCSM2		= tex0.nCSM == 1;
	cachedTexture->m_texture		= texture;
	cachedTexture->m_checksum		= checksum;
	cachedTexture->m_live			= true;

	m_cachedTextures.pop_back();
	m_cachedTextures.push_front(cachedTexture);
}

void CGSH_Direct3D9::TexCache_InvalidateTextures(uint32 start, uint32 size)
{
	std::for_each(std::begin(m_cachedTextures), std::end(m_cachedTextures), [&] (CachedTexturePtr& texture) { texture->InvalidateFromMemorySpace(start, size); });
}

void CGSH_Direct3D9::TexCache_Flush()
{
	std::for_each(std::begin(m_cachedTextures), std::end(m_cachedTextures), [] (CachedTexturePtr& texture) { texture->Free(); });
}
