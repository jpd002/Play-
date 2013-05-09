#pragma once

#include "../DirectXControl.h"
#include "bitmap/Bitmap.h"

class CPixelBufferView : public CDirectXControl
{
public:
							CPixelBufferView(HWND, const RECT&);
	virtual					~CPixelBufferView();

	void					SetBitmap(const Framework::CBitmap&);

protected:
	virtual void			Refresh() override;

	virtual long			OnSize(unsigned int, unsigned int, unsigned int) override;
	virtual void			OnDeviceReset() override;
	virtual void			OnDeviceResetting() override;

private:
	typedef Framework::Win32::CComPtr<IDirect3DTexture9> TexturePtr;
	typedef Framework::Win32::CComPtr<IDirect3DVertexBuffer9> VertexBufferPtr;
	typedef Framework::Win32::CComPtr<IDirect3DVertexDeclaration9> VertexDeclarationPtr;
	typedef Framework::Win32::CComPtr<ID3DXEffect> EffectPtr;

	struct VERTEX
	{
		float	position[3];
		float	texCoord[2];
	};

	void					CreateResources();
	void					DrawCheckerboard();
	void					DrawPixelBuffer();

	EffectPtr				CreateEffectFromResource(const TCHAR*);
	TexturePtr				CreateTextureFromBitmap(const Framework::CBitmap&);

	TexturePtr				m_pixelBufferTexture;
	Framework::CBitmap		m_pixelBufferBitmap;
	VertexDeclarationPtr	m_quadVertexDecl;
	VertexBufferPtr			m_quadVertexBuffer;
	EffectPtr				m_checkerboardEffect;
	EffectPtr				m_pixelBufferViewEffect;
};
