#pragma once

#include <d3d9.h>
#include "bitmap\Bitmap.h"

template <typename PixelType>
static void CopyTextureToBitmap(Framework::CBitmap& bitmap, IDirect3DTexture9* texture, uint32 mipLevel)
{
	HRESULT result = S_OK;

	D3DLOCKED_RECT lockedRect = {};
	result = texture->LockRect(mipLevel, &lockedRect, nullptr, D3DLOCK_READONLY);
	assert(SUCCEEDED(result));

	auto srcPtr = reinterpret_cast<uint8*>(lockedRect.pBits);
	auto dstPtr = reinterpret_cast<uint8*>(bitmap.GetPixels());
	for(unsigned int y = 0; y < bitmap.GetHeight(); y++)
	{
		memcpy(dstPtr, srcPtr, bitmap.GetWidth() * sizeof(PixelType));
		dstPtr += bitmap.GetPitch();
		srcPtr += lockedRect.Pitch;
	}

	result = texture->UnlockRect(mipLevel);
	assert(SUCCEEDED(result));
}

template <typename PixelType>
static void CopyBitmapToTexture(IDirect3DTexture9* texture, uint32 mipLevel, const Framework::CBitmap& bitmap)
{
	HRESULT result = S_OK;

	D3DLOCKED_RECT lockedRect = {};
	result = texture->LockRect(mipLevel, &lockedRect, nullptr, 0);
	assert(SUCCEEDED(result));

	auto srcPtr = reinterpret_cast<uint8*>(bitmap.GetPixels());
	auto dstPtr = reinterpret_cast<uint8*>(lockedRect.pBits);
	for(unsigned int y = 0; y < bitmap.GetHeight(); y++)
	{
		memcpy(dstPtr, srcPtr, bitmap.GetWidth() * sizeof(PixelType));
		dstPtr += lockedRect.Pitch;
		srcPtr += bitmap.GetPitch();
	}

	result = texture->UnlockRect(mipLevel);
	assert(SUCCEEDED(result));
}
