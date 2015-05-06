#pragma once

#include "../DirectXControl.h"
#include "bitmap/Bitmap.h"
#include "PixelBufferViewOverlay.h"
#include <memory>

class CPixelBufferView : public CDirectXControl
{
public:
							CPixelBufferView(HWND, const RECT&);
	virtual					~CPixelBufferView();

	void					SetBitmap(const Framework::CBitmap&);
	void					FitBitmap();

protected:
	virtual void			Refresh() override;

	virtual long			OnCommand(unsigned short, unsigned short, HWND) override;
	virtual long			OnSize(unsigned int, unsigned int, unsigned int) override;

	virtual long			OnLeftButtonDown(int, int) override;
	virtual long			OnLeftButtonUp(int, int) override;

	virtual long			OnMouseMove(WPARAM, int, int) override;

	virtual long			OnMouseWheel(int, int, short) override;

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

	void					OnSaveBitmap();

	void					CreateResources();
	void					DrawCheckerboard();
	void					DrawPixelBuffer();

	EffectPtr				CreateEffectFromResource(const TCHAR*);
	TexturePtr				CreateTextureFromBitmap(const Framework::CBitmap&);
	void					SetEffectVector(EffectPtr&, const char*, float, float, float, float);

	TexturePtr				m_pixelBufferTexture;
	Framework::CBitmap		m_pixelBufferBitmap;
	VertexDeclarationPtr	m_quadVertexDecl;
	VertexBufferPtr			m_quadVertexBuffer;
	EffectPtr				m_checkerboardEffect;
	EffectPtr				m_pixelBufferViewEffect;

	std::unique_ptr<CPixelBufferViewOverlay>	m_overlay;

	float					m_panX;
	float					m_panY;
	float					m_zoomFactor;

	bool					m_dragging;
	int						m_dragBaseX;
	int						m_dragBaseY;

	float					m_panXDragBase;
	float					m_panYDragBase;
};
