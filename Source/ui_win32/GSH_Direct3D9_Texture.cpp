#include <assert.h>
#include <zlib.h>
#include <sys/stat.h>
#include <d3dx9.h>
#include "GSH_Direct3D9.h"
#include "../gs/GsPixelFormats.h"

void CGSH_Direct3D9::SetupTextureUpdaters()
{
	for(unsigned int i = 0; i < PSM_MAX; i++)
	{
		m_textureUpdater[i] = &CGSH_Direct3D9::TexUpdater_Invalid;
	}

	m_textureUpdater[PSMCT32]  = &CGSH_Direct3D9::TexUpdater_Psm32;
	m_textureUpdater[PSMCT24]  = &CGSH_Direct3D9::TexUpdater_Psm32;
	m_textureUpdater[PSMCT16]  = &CGSH_Direct3D9::TexUpdater_Psm16<CGsPixelFormats::CPixelIndexorPSMCT16>;
	m_textureUpdater[PSMCT16S] = &CGSH_Direct3D9::TexUpdater_Psm16<CGsPixelFormats::CPixelIndexorPSMCT16S>;
	m_textureUpdater[PSMT8]    = &CGSH_Direct3D9::TexUpdater_Psm48<CGsPixelFormats::CPixelIndexorPSMT8>;
	m_textureUpdater[PSMT4]    = &CGSH_Direct3D9::TexUpdater_Psm48<CGsPixelFormats::CPixelIndexorPSMT4>;
	m_textureUpdater[PSMT8H]   = &CGSH_Direct3D9::TexUpdater_Psm48H<24, 0xFF>;
	m_textureUpdater[PSMT4HL]  = &CGSH_Direct3D9::TexUpdater_Psm48H<24, 0x0F>;
	m_textureUpdater[PSMT4HH]  = &CGSH_Direct3D9::TexUpdater_Psm48H<28, 0x0F>;
}

CGSH_Direct3D9::TEXTURE_INFO CGSH_Direct3D9::LoadTexture(const TEX0& tex0, uint32 maxMip, const MIPTBP1& miptbp1, const MIPTBP2& miptbp2)
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

	HRESULT resultCode = S_OK;

	auto texture = m_textureCache.Search(tex0);
	if(!texture)
	{
		uint32 width  = tex0.GetWidth();
		uint32 height = tex0.GetHeight();

		D3DFORMAT textureFormat = D3DFMT_A8R8G8B8;
		switch(tex0.nPsm)
		{
		case PSMCT32:
			textureFormat = D3DFMT_A8R8G8B8;
			break;
		case PSMCT24:
			textureFormat = D3DFMT_X8R8G8B8;
			break;
		case PSMCT16:
		case PSMCT16S:
			textureFormat = D3DFMT_A1R5G5B5;
			break;
		case PSMT8:
		case PSMT4:
		case PSMT8H:
		case PSMT4HL:
		case PSMT4HH:
			textureFormat = D3DFMT_L8;
			break;
		default:
			assert(false);
			break;
		}

		{
			TexturePtr textureHandle;
			resultCode = m_device->CreateTexture(width, height, 1 + maxMip, D3DUSAGE_DYNAMIC, textureFormat, D3DPOOL_DEFAULT, &textureHandle, NULL);
			assert(SUCCEEDED(resultCode));
			m_textureCache.Insert(tex0, std::move(textureHandle));
		}

		texture = m_textureCache.Search(tex0);
		texture->m_cachedArea.Invalidate(0, RAMSIZE);
	}

	auto& cachedArea = texture->m_cachedArea;

	if(cachedArea.HasDirtyPages())
	{
		D3DLOCKED_RECT lockedRect;
		resultCode = texture->m_textureHandle->LockRect(0, &lockedRect, nullptr, 0);
		assert(SUCCEEDED(resultCode));

		auto texturePageSize = CGsPixelFormats::GetPsmPageSize(tex0.nPsm);
		auto areaRect = cachedArea.GetAreaPageRect();

		for(unsigned int dirtyPageIndex = 0; dirtyPageIndex < CGsCachedArea::MAX_DIRTYPAGES; dirtyPageIndex++)
		{
			if(!cachedArea.IsPageDirty(dirtyPageIndex)) continue;

			uint32 pageX = dirtyPageIndex % areaRect.width;
			uint32 pageY = dirtyPageIndex / areaRect.width;
			uint32 texX = pageX * texturePageSize.first;
			uint32 texY = pageY * texturePageSize.second;
			uint32 texWidth = texturePageSize.first;
			uint32 texHeight = texturePageSize.second;
			if(texX >= tex0.GetWidth()) continue;
			if(texY >= tex0.GetHeight()) continue;
			if((texX + texWidth) > tex0.GetWidth())
			{
				texWidth = tex0.GetWidth() - texX;
			}
			if((texY + texHeight) > tex0.GetHeight())
			{
				texHeight = tex0.GetHeight() - texY;
			}
			((this)->*(m_textureUpdater[tex0.nPsm]))(&lockedRect, tex0.GetBufPtr(), tex0.nBufWidth, texX, texY, texWidth, texHeight);
		}

		cachedArea.ClearDirtyPages();

		resultCode = texture->m_textureHandle->UnlockRect(0);
		assert(SUCCEEDED(resultCode));

		//Update mipmap levels
		for(uint32 mipLevel = 1; mipLevel <= maxMip; mipLevel++)
		{
			uint32 mipLevelWidth = std::max<uint32>(tex0.GetWidth() >> mipLevel, 1);
			uint32 mipLevelHeight = std::max<uint32>(tex0.GetHeight() >> mipLevel, 1);

			uint32 mipLevelBufferPointer = 0;
			uint32 mipLevelBufferWidth = 0;
			switch(mipLevel)
			{
			case 1: mipLevelBufferPointer = miptbp1.GetTbp1(); mipLevelBufferWidth = miptbp1.GetTbw1(); break;
			case 2: mipLevelBufferPointer = miptbp1.GetTbp2(); mipLevelBufferWidth = miptbp1.GetTbw2(); break;
			case 3: mipLevelBufferPointer = miptbp1.GetTbp3(); mipLevelBufferWidth = miptbp1.GetTbw3(); break;
			case 4: mipLevelBufferPointer = miptbp2.GetTbp4(); mipLevelBufferWidth = miptbp2.GetTbw4(); break;
			case 5: mipLevelBufferPointer = miptbp2.GetTbp5(); mipLevelBufferWidth = miptbp2.GetTbw5(); break;
			case 6: mipLevelBufferPointer = miptbp2.GetTbp6(); mipLevelBufferWidth = miptbp2.GetTbw6(); break;
			}

			if(mipLevelBufferWidth == 0) break;

			resultCode = texture->m_textureHandle->LockRect(mipLevel, &lockedRect, nullptr, 0);
			assert(SUCCEEDED(resultCode));

			((this)->*(m_textureUpdater[tex0.nPsm]))(&lockedRect, mipLevelBufferPointer, mipLevelBufferWidth / 64, 0, 0, mipLevelWidth, mipLevelHeight);

			resultCode = texture->m_textureHandle->UnlockRect(mipLevel);
			assert(SUCCEEDED(resultCode));
		}
	}

	result.texture = texture->m_textureHandle;
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

void CGSH_Direct3D9::TexUpdater_Invalid(D3DLOCKED_RECT*, uint32, uint32, unsigned int, unsigned int, unsigned int, unsigned int)
{
	assert(false);
}

void CGSH_Direct3D9::TexUpdater_Psm32(D3DLOCKED_RECT* lockedRect, uint32 bufPtr, uint32 bufWidth, unsigned int texX, unsigned int texY, unsigned int texWidth, unsigned int texHeight)
{
	CGsPixelFormats::CPixelIndexorPSMCT32 indexor(m_pRAM, bufPtr, bufWidth);

	auto dstPitch = lockedRect->Pitch / 4;
	auto dst = reinterpret_cast<uint32*>(lockedRect->pBits);
	dst += texX + (texY * dstPitch);

	for(unsigned int y = 0; y < texHeight; y++)
	{
		for(unsigned int x = 0; x < texWidth; x++)
		{
			uint32 color = indexor.GetPixel(texX + x, texY + y);
			dst[x] = Color_Ps2ToDx9(color);
		}

		dst += dstPitch;
	}
}

template <typename IndexorType>
void CGSH_Direct3D9::TexUpdater_Psm16(D3DLOCKED_RECT* lockedRect, uint32 bufPtr, uint32 bufWidth, unsigned int texX, unsigned int texY, unsigned int texWidth, unsigned int texHeight)
{
	IndexorType indexor(m_pRAM, bufPtr, bufWidth);

	auto dstPitch = lockedRect->Pitch / 2;
	auto dst = reinterpret_cast<uint16*>(lockedRect->pBits);
	dst += texX + (texY * dstPitch);
	
	for(unsigned int y = 0; y < texHeight; y++)
	{
		for(unsigned int x = 0; x < texWidth; x++)
		{
			auto pixel = indexor.GetPixel(texX + x, texY + y);
			auto cvtPixel =
				(((pixel & 0x001F) >> 0)  << 10)  | //R
				(((pixel & 0x03E0) >> 5)  <<  5)  | //G
				(((pixel & 0x7C00) >> 10) <<  0)  | //B
				(((pixel & 0x8000) >> 15) << 15);   //A
			dst[x] = cvtPixel;
		}

		dst += dstPitch;
	}
}

template <typename IndexorType>
void CGSH_Direct3D9::TexUpdater_Psm48(D3DLOCKED_RECT* lockedRect, uint32 bufPtr, uint32 bufWidth, unsigned int texX, unsigned int texY, unsigned int texWidth, unsigned int texHeight)
{
	IndexorType indexor(m_pRAM, bufPtr, bufWidth);

	auto dstPitch = lockedRect->Pitch;
	auto dst = reinterpret_cast<uint8*>(lockedRect->pBits);
	dst += texX + (texY * dstPitch);

	for(unsigned int y = 0; y < texHeight; y++)
	{
		for(unsigned int x = 0; x < texWidth; x++)
		{
			uint8 pixel = indexor.GetPixel(texX + x, texY + y);
			dst[x] = pixel;
		}

		dst += dstPitch;
	}
}

template <uint32 shiftAmount, uint32 mask>
void CGSH_Direct3D9::TexUpdater_Psm48H(D3DLOCKED_RECT* lockedRect, uint32 bufPtr, uint32 bufWidth, unsigned int texX, unsigned int texY, unsigned int texWidth, unsigned int texHeight)
{
	CGsPixelFormats::CPixelIndexorPSMCT32 indexor(m_pRAM, bufPtr, bufWidth);

	auto dstPitch = lockedRect->Pitch;
	auto dst = reinterpret_cast<uint8*>(lockedRect->pBits);
	dst += texX + (texY * dstPitch);

	for(unsigned int y = 0; y < texHeight; y++)
	{
		for(unsigned int x = 0; x < texWidth; x++)
		{
			uint32 pixel = indexor.GetPixel(texX + x, texY + y);
			pixel = (pixel >> shiftAmount) & mask;
			dst[x] = static_cast<uint8>(pixel);
		}

		dst += dstPitch;
	}
}

//------------------------------------------------------------------------
//CLUT Stuff
//------------------------------------------------------------------------

CGSH_Direct3D9::TexturePtr CGSH_Direct3D9::GetClutTexture(const TEX0& tex0)
{
	HRESULT result = S_OK;

	std::array<uint32, 256> convertedClut;
	MakeLinearCLUT(tex0, convertedClut);
	
	unsigned int entryCount = CGsPixelFormats::IsPsmIDTEX4(tex0.nPsm) ? 16 : 256;
	auto clutTexture = CGsPixelFormats::IsPsmIDTEX4(tex0.nPsm) ? m_clutTexture4 : m_clutTexture8;

	{
		D3DLOCKED_RECT rect;
		result = clutTexture->LockRect(0, &rect, NULL, D3DLOCK_DISCARD);
		assert(SUCCEEDED(result));

		auto dst = reinterpret_cast<uint32*>(rect.pBits);
		unsigned int nDstPitch = rect.Pitch / 4;

		for(uint32 i = 0; i < entryCount; i++)
		{
			dst[i] = Color_Ps2ToDx9(convertedClut[i]);
		}

		result = clutTexture->UnlockRect(0);
		assert(SUCCEEDED(result));
	}

	return clutTexture;
}
