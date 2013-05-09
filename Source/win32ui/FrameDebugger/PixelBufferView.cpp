#include "PixelBufferView.h"
#include <assert.h>
#include <vector>
#include "../resource.h"

CPixelBufferView::CPixelBufferView(HWND parent, const RECT& rect)
: CDirectXControl(parent)
{
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

	D3DXHANDLE screenSizeParameter = m_checkerboardEffect->GetParameterByName(NULL, "g_screenSize");
	assert(screenSizeParameter != NULL);

	RECT clientRect = GetClientRect();
	D3DXVECTOR4 screenSizeValue;
	screenSizeValue.x = static_cast<float>(clientRect.right);
	screenSizeValue.y = static_cast<float>(clientRect.bottom);
	screenSizeValue.z = 0;
	screenSizeValue.w = 0;

	m_checkerboardEffect->SetVector(screenSizeParameter, &screenSizeValue);
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

	{
		D3DXHANDLE screenSizeParameter = m_pixelBufferViewEffect->GetParameterByName(NULL, "g_screenSize");
		assert(screenSizeParameter != NULL);

		RECT clientRect = GetClientRect();
		D3DXVECTOR4 screenSizeValue;
		screenSizeValue.x = static_cast<float>(clientRect.right);
		screenSizeValue.y = static_cast<float>(clientRect.bottom);
		screenSizeValue.z = 0;
		screenSizeValue.w = 0;

		m_pixelBufferViewEffect->SetVector(screenSizeParameter, &screenSizeValue);
	}

	{
		D3DXHANDLE bufferSizeParameter = m_pixelBufferViewEffect->GetParameterByName(NULL, "g_bufferSize");
		assert(bufferSizeParameter != NULL);

		D3DXVECTOR4 bufferSizeValue;
		bufferSizeValue.x = static_cast<float>(m_pixelBufferBitmap.GetWidth());
		bufferSizeValue.y = static_cast<float>(m_pixelBufferBitmap.GetHeight());
		bufferSizeValue.z = 0;
		bufferSizeValue.w = 0;

		m_pixelBufferViewEffect->SetVector(bufferSizeParameter, &bufferSizeValue);
	}

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

long CPixelBufferView::OnSize(unsigned int type, unsigned int x, unsigned int y)
{
	long result = CDirectXControl::OnSize(type, x, y);
	Refresh();
	return result;
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
