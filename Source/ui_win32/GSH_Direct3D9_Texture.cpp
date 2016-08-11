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

	m_textureUpdater[PSMCT32] = &CGSH_Direct3D9::TexUpdater_Psm32;
	m_textureUpdater[PSMT8]   = &CGSH_Direct3D9::TexUpdater_Psm48<CGsPixelFormats::CPixelIndexorPSMT8>;
	m_textureUpdater[PSMT4]   = &CGSH_Direct3D9::TexUpdater_Psm48<CGsPixelFormats::CPixelIndexorPSMT4>;
}

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
		case PSMT8:
		case PSMT4:
			textureFormat = D3DFMT_L8;
			break;
		default:
			assert(false);
			break;
		}

		resultCode = m_device->CreateTexture(width, height, 1, D3DUSAGE_DYNAMIC, textureFormat, D3DPOOL_DEFAULT, &result.texture, NULL);
		assert(SUCCEEDED(resultCode));

		m_textureCache.Insert(tex0, result.texture);
		texture = m_textureCache.Search(tex0);
		assert(result.texture == texture->m_textureHandle);

		texture->m_cachedArea.Invalidate(0, RAMSIZE);
	}

	auto& cachedArea = texture->m_cachedArea;

	if(cachedArea.HasDirtyPages())
	{
		D3DLOCKED_RECT lockedRect;
		resultCode = texture->m_textureHandle->LockRect(0, &lockedRect, nullptr, 0);
		assert(SUCCEEDED(resultCode));

		auto texturePageSize = CGsPixelFormats::GetPsmPageSize(tex0.nPsm);
		auto pageRect = cachedArea.GetPageRect();

		for(unsigned int dirtyPageIndex = 0; dirtyPageIndex < CGsCachedArea::MAX_DIRTYPAGES; dirtyPageIndex++)
		{
			if(!cachedArea.IsPageDirty(dirtyPageIndex)) continue;

			uint32 pageX = dirtyPageIndex % pageRect.first;
			uint32 pageY = dirtyPageIndex / pageRect.first;
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
