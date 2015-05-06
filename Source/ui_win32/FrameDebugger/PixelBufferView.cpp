#include "PixelBufferView.h"
#include <assert.h>
#include <vector>
#include "../resource.h"
#include "win32/FileDialog.h"
#include "bitmap/BMP.h"
#include "StdStreamUtils.h"
#include "string_format.h"

CPixelBufferView::CPixelBufferView(HWND parent, const RECT& rect)
: CDirectXControl(parent)
, m_zoomFactor(1)
, m_panX(0)
, m_panY(0)
, m_dragging(false)
, m_dragBaseX(0)
, m_dragBaseY(0)
, m_panXDragBase(0)
, m_panYDragBase(0)
{
	m_overlay = std::make_unique<CPixelBufferViewOverlay>(m_hWnd);

	m_checkerboardEffect = CreateEffectFromResource(MAKEINTRESOURCE(IDR_CHECKERBOARD_SHADER));
	m_pixelBufferViewEffect = CreateEffectFromResource(MAKEINTRESOURCE(IDR_PIXELBUFFERVIEW_SHADER));
	SetSizePosition(rect);
}

CPixelBufferView::~CPixelBufferView()
{

}

void CPixelBufferView::SetBitmap(const Framework::CBitmap& bitmap)
{
	m_pixelBufferBitmap = bitmap;
	m_pixelBufferTexture = CreateTextureFromBitmap(bitmap);
	Refresh();
}

void CPixelBufferView::Refresh()
{
	if(m_device.IsEmpty()) return;
	if(!TestDevice()) return;

	D3DCOLOR backgroundColor = D3DCOLOR_XRGB(0x00, 0x00, 0x00);

	m_device->Clear(0, NULL, D3DCLEAR_TARGET, backgroundColor, 1.0f, 0);
	m_device->BeginScene();

	DrawCheckerboard();
	DrawPixelBuffer();

	m_device->EndScene();
	m_device->Present(NULL, NULL, NULL, NULL);
}

void CPixelBufferView::DrawCheckerboard()
{
	m_device->SetVertexDeclaration(m_quadVertexDecl);
	m_device->SetStreamSource(0, m_quadVertexBuffer, 0, sizeof(VERTEX));

	RECT clientRect = GetClientRect();
	SetEffectVector(m_checkerboardEffect, "g_screenSize", 
		static_cast<float>(clientRect.right), static_cast<float>(clientRect.bottom), 0, 0);

	m_checkerboardEffect->CommitChanges();

	UINT passCount = 0;
	m_checkerboardEffect->Begin(&passCount, D3DXFX_DONOTSAVESTATE);
	for(unsigned int i = 0; i < passCount; i++)
	{
		m_checkerboardEffect->BeginPass(i);

		m_device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);

		m_checkerboardEffect->EndPass();
	}
	m_checkerboardEffect->End();
}

void CPixelBufferView::DrawPixelBuffer()
{
	if(m_pixelBufferBitmap.IsEmpty()) return;

	m_device->SetVertexDeclaration(m_quadVertexDecl);
	m_device->SetStreamSource(0, m_quadVertexBuffer, 0, sizeof(VERTEX));

	RECT clientRect = GetClientRect();

	SetEffectVector(m_pixelBufferViewEffect, "g_screenSize", 
		static_cast<float>(clientRect.right), static_cast<float>(clientRect.bottom), 0, 0);

	SetEffectVector(m_pixelBufferViewEffect, "g_bufferSize",
		static_cast<float>(m_pixelBufferBitmap.GetWidth()), static_cast<float>(m_pixelBufferBitmap.GetHeight()), 0, 0);

	SetEffectVector(m_pixelBufferViewEffect, "g_panOffset", m_panX, m_panY, 0, 0);
	SetEffectVector(m_pixelBufferViewEffect, "g_zoomFactor", m_zoomFactor, 0, 0, 0);

	D3DXHANDLE textureParameter = m_pixelBufferViewEffect->GetParameterByName(NULL, "g_bufferTexture");
	m_pixelBufferViewEffect->SetTexture(textureParameter, m_pixelBufferTexture);

	m_pixelBufferViewEffect->CommitChanges();

	UINT passCount = 0;
	m_pixelBufferViewEffect->Begin(&passCount, D3DXFX_DONOTSAVESTATE);
	for(unsigned int i = 0; i < passCount; i++)
	{
		m_pixelBufferViewEffect->BeginPass(i);

		m_device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);

		m_pixelBufferViewEffect->EndPass();
	}
	m_pixelBufferViewEffect->End();
}

long CPixelBufferView::OnCommand(unsigned short id, unsigned short cmd, HWND hwndFrom)
{
	if(CWindow::IsCommandSource(m_overlay.get(), hwndFrom))
	{
		switch(cmd)
		{
		case CPixelBufferViewOverlay::COMMAND_SAVE:
			OnSaveBitmap();
			break;
		case CPixelBufferViewOverlay::COMMAND_FIT:
			FitBitmap();
			break;
		}
	}
	return TRUE;
}

long CPixelBufferView::OnSize(unsigned int type, unsigned int x, unsigned int y)
{
	long result = CDirectXControl::OnSize(type, x, y);
	Refresh();
	return result;
}

long CPixelBufferView::OnLeftButtonDown(int x, int y)
{
	SetCapture(m_hWnd);
	m_dragBaseX = x;
	m_dragBaseY = y;
	m_panXDragBase = m_panX;
	m_panYDragBase = m_panY;
	m_dragging = true;
	return CDirectXControl::OnLeftButtonDown(x, y);
}

long CPixelBufferView::OnLeftButtonUp(int x, int y)
{
	m_dragging = false;
	ReleaseCapture();
	return CDirectXControl::OnLeftButtonUp(x, y);
}

long CPixelBufferView::OnMouseMove(WPARAM wparam, int x, int y)
{
	if(m_dragging)
	{
		RECT clientRect = GetClientRect();
		m_panX = m_panXDragBase + (static_cast<float>(x - m_dragBaseX) / static_cast<float>(clientRect.right / 2)) / m_zoomFactor;
		m_panY = m_panYDragBase - (static_cast<float>(y - m_dragBaseY) / static_cast<float>(clientRect.bottom / 2)) / m_zoomFactor;
		Refresh();
	}
	return CDirectXControl::OnMouseMove(wparam, x, y);
}

long CPixelBufferView::OnMouseWheel(int x, int y, short z)
{
	float newZoom = 0;
	z /= WHEEL_DELTA;
	if(z <= -1)
	{
		newZoom = m_zoomFactor / 2;
	}
	else if(z >= 1)
	{
		newZoom = m_zoomFactor * 2;
	}

	if(newZoom != 0)
	{
		auto clientRect = GetClientRect();
		POINT mousePoint = { x, y };
		ScreenToClient(m_hWnd, &mousePoint);
		float relPosX = static_cast<float>(mousePoint.x) / static_cast<float>(clientRect.Right());
		float relPosY = static_cast<float>(mousePoint.y) / static_cast<float>(clientRect.Bottom());
		relPosX = std::max(relPosX, 0.f); relPosX = std::min(relPosX, 1.f);
		relPosY = std::max(relPosY, 0.f); relPosY = std::min(relPosY, 1.f);

		relPosX = (relPosX - 0.5f) * 2;
		relPosY = (relPosY - 0.5f) * 2;

		float panModX = (1 - newZoom / m_zoomFactor) * relPosX;
		float panModY = (1 - newZoom / m_zoomFactor) * relPosY;
		m_panX += panModX;
		m_panY -= panModY;

		m_zoomFactor = newZoom;
		Refresh();
	}

	return TRUE;
}

void CPixelBufferView::OnDeviceReset()
{
	CreateResources();
	m_pixelBufferTexture = CreateTextureFromBitmap(m_pixelBufferBitmap);
	if(!m_checkerboardEffect.IsEmpty()) m_checkerboardEffect->OnResetDevice();
	if(!m_pixelBufferViewEffect.IsEmpty()) m_pixelBufferViewEffect->OnResetDevice();
}

void CPixelBufferView::OnDeviceResetting()
{
	m_quadVertexBuffer.Reset();
	m_quadVertexDecl.Reset();
	m_pixelBufferTexture.Reset();
	if(!m_checkerboardEffect.IsEmpty()) m_checkerboardEffect->OnLostDevice();
	if(!m_pixelBufferViewEffect.IsEmpty()) m_pixelBufferViewEffect->OnLostDevice();
}

void CPixelBufferView::OnSaveBitmap()
{
	if(!m_pixelBufferBitmap.IsEmpty())
	{
		Framework::Win32::CFileDialog openFileDialog;
		openFileDialog.m_OFN.lpstrFilter = _T("Windows Bitmap Files (*.bmp)\0*.bmp");
		int result = openFileDialog.SummonSave(m_hWnd);
		if(result == IDOK)
		{
			try
			{
				auto outputStream = Framework::CreateOutputStdStream(std::tstring(openFileDialog.m_OFN.lpstrFile));
				Framework::CBMP::WriteBitmap(m_pixelBufferBitmap, outputStream);
			}
			catch(const std::exception& exception)
			{
				auto message = string_format("Failed to save buffer to file:\r\n\r\n%s", exception.what());
				MessageBoxA(m_hWnd, message.c_str(), nullptr, MB_ICONHAND);
			}
		}
	}
}

void CPixelBufferView::FitBitmap()
{
	if(m_pixelBufferBitmap.IsEmpty()) return;

	Framework::Win32::CRect clientRect = GetClientRect();
	unsigned int marginSize = 50;
	float normalizedSizeX = static_cast<float>(m_pixelBufferBitmap.GetWidth()) / static_cast<float>(clientRect.Right() - marginSize);
	float normalizedSizeY = static_cast<float>(m_pixelBufferBitmap.GetHeight()) / static_cast<float>(clientRect.Bottom() - marginSize);

	float normalizedSize = std::max<float>(normalizedSizeX, normalizedSizeY);

	m_zoomFactor = 1 / normalizedSize;
	m_panX = 0;
	m_panY = 0;

	Refresh();
}

void CPixelBufferView::CreateResources()
{
	static const VERTEX g_quadVertexBufferVertices[4] =
	{
		{ -1, -1, 0, 0, 1 },
		{ -1,  1, 0, 0, 0 },
		{  1, -1, 0, 1, 1 },
		{  1,  1, 0, 1, 0 },
	};

	HRESULT result = S_OK;
	
	result = m_device->CreateVertexBuffer(sizeof(g_quadVertexBufferVertices), D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, &m_quadVertexBuffer, nullptr);
	assert(SUCCEEDED(result));

	{
		void* vertexBufferData(nullptr);
		result = m_quadVertexBuffer->Lock(0, 0, &vertexBufferData, 0);
		assert(SUCCEEDED(result));

		memcpy(vertexBufferData, g_quadVertexBufferVertices, sizeof(g_quadVertexBufferVertices));

		result = m_quadVertexBuffer->Unlock();
		assert(SUCCEEDED(result));
	}

	{
		std::vector<D3DVERTEXELEMENT9> vertexElements;
		
		{
			D3DVERTEXELEMENT9 element = {};
			element.Offset	= offsetof(VERTEX, position);
			element.Type	= D3DDECLTYPE_FLOAT3;
			element.Usage	= D3DDECLUSAGE_POSITION;
			vertexElements.push_back(element);
		}

		{
			D3DVERTEXELEMENT9 element = {};
			element.Offset	= offsetof(VERTEX, texCoord);
			element.Type	= D3DDECLTYPE_FLOAT2;
			element.Usage	= D3DDECLUSAGE_TEXCOORD;
			vertexElements.push_back(element);
		}

		{
			D3DVERTEXELEMENT9 element = D3DDECL_END();
			vertexElements.push_back(element);
		}

		result = m_device->CreateVertexDeclaration(vertexElements.data(), &m_quadVertexDecl);
		assert(SUCCEEDED(result));
	}
}

CPixelBufferView::EffectPtr CPixelBufferView::CreateEffectFromResource(const TCHAR* resourceName)
{
	HRESULT result = S_OK;

	HRSRC shaderResourceInfo = FindResource(GetModuleHandle(nullptr), resourceName, _T("TEXTFILE"));
	assert(shaderResourceInfo != nullptr);

	HGLOBAL shaderResourceHandle = LoadResource(GetModuleHandle(nullptr), shaderResourceInfo);
	DWORD shaderResourceSize = SizeofResource(GetModuleHandle(nullptr), shaderResourceInfo);

	const char* shaderData = reinterpret_cast<const char*>(LockResource(shaderResourceHandle));

	EffectPtr effect;
	Framework::Win32::CComPtr<ID3DXBuffer> errors;
	result = D3DXCreateEffect(m_device, shaderData, shaderResourceSize, nullptr, nullptr, 0, nullptr, &effect, &errors);
	if(!errors.IsEmpty())
	{
		std::string errorText(reinterpret_cast<const char*>(errors->GetBufferPointer()), reinterpret_cast<const char*>(errors->GetBufferPointer()) + errors->GetBufferSize());
		OutputDebugStringA("Failed to compile shader:\r\n");
		OutputDebugStringA(errorText.c_str());
	}
	assert(SUCCEEDED(result));

	UnlockResource(shaderResourceHandle);

	return effect;
}

CPixelBufferView::TexturePtr CPixelBufferView::CreateTextureFromBitmap(const Framework::CBitmap& bitmap)
{
	TexturePtr texture;

	if(!bitmap.IsEmpty())
	{
		HRESULT result = S_OK;
		result = m_device->CreateTexture(bitmap.GetWidth(), bitmap.GetHeight(), 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &texture, nullptr);
		assert(SUCCEEDED(result));

		D3DLOCKED_RECT lockedRect = {};

		result = texture->LockRect(0, &lockedRect, nullptr, 0);
		assert(SUCCEEDED(result));
		
		uint32* dstPtr = reinterpret_cast<uint32*>(lockedRect.pBits);
		uint32* srcPtr = reinterpret_cast<uint32*>(bitmap.GetPixels());
		for(unsigned int y = 0; y < bitmap.GetHeight(); y++)
		{
			memcpy(dstPtr, srcPtr, bitmap.GetWidth() * sizeof(uint32));
			dstPtr += lockedRect.Pitch / sizeof(uint32);
			srcPtr += bitmap.GetPitch() / sizeof(uint32);
		}

		result = texture->UnlockRect(0);
		assert(SUCCEEDED(result));
	}

	return texture;
}

void CPixelBufferView::SetEffectVector(EffectPtr& effect, const char* parameterName, float x, float y, float z, float w)
{
	D3DXHANDLE parameter = effect->GetParameterByName(NULL, parameterName);
	assert(parameter != NULL);

	D3DXVECTOR4 value;
	value.x = x;
	value.y = y;
	value.z = z;
	value.w = w;

	effect->SetVector(parameter, &value);
}
