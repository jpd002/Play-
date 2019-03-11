#include "PixelBufferView.h"
#include <assert.h>
#include <vector>
#include <algorithm>
#include <D3Dcompiler.h>
#include "../resource.h"
#include "../D3D9TextureUtils.h"
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

	m_checkerboardVertexShader = CreateVertexShaderFromResource(MAKEINTRESOURCE(IDR_CHECKERBOARD_VERTEXSHADER));
	m_checkerboardPixelShader = CreatePixelShaderFromResource(MAKEINTRESOURCE(IDR_CHECKERBOARD_PIXELSHADER));
	m_pixelBufferViewVertexShader = CreateVertexShaderFromResource(MAKEINTRESOURCE(IDR_PIXELBUFFERVIEW_VERTEXSHADER));
	m_pixelBufferViewPixelShader = CreatePixelShaderFromResource(MAKEINTRESOURCE(IDR_PIXELBUFFERVIEW_PIXELSHADER));
	SetSizePosition(rect);
}

void CPixelBufferView::SetPixelBuffers(PixelBufferArray pixelBuffers)
{
	m_pixelBuffers = std::move(pixelBuffers);
	//Update buffer titles
	{
		CPixelBufferViewOverlay::StringList titles;
		for(const auto& pixelBuffer : m_pixelBuffers)
		{
			titles.push_back(pixelBuffer.first);
		}
		//Save previously selected index
		int selectedIndex = m_overlay->GetSelectedPixelBufferIndex();
		auto titleCount = titles.size();
		m_overlay->SetPixelBufferTitles(std::move(titles));
		//Restore selected index
		if((selectedIndex != -1) && (selectedIndex < titleCount))
		{
			m_overlay->SetSelectedPixelBufferIndex(selectedIndex);
		}
	}
	CreateSelectedPixelBufferTexture();
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
	HRESULT result = S_OK;

	m_device->SetVertexDeclaration(m_quadVertexDecl);
	m_device->SetStreamSource(0, m_quadVertexBuffer, 0, sizeof(VERTEX));

	RECT clientRect = GetClientRect();

	float screenSizeVector[4] =
	    {
	        static_cast<float>(clientRect.right),
	        static_cast<float>(clientRect.bottom),
	        0,
	        0};

	m_device->SetVertexShader(m_checkerboardVertexShader);
	m_device->SetPixelShader(m_checkerboardPixelShader);
	m_device->SetVertexShaderConstantF(0, screenSizeVector, 1);

	result = m_device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
	assert(SUCCEEDED(result));
}

void CPixelBufferView::DrawPixelBuffer()
{
	auto pixelBuffer = GetSelectedPixelBuffer();
	if(!pixelBuffer) return;
	const auto& pixelBufferBitmap = pixelBuffer->second;

	HRESULT result = S_OK;

	m_device->SetVertexDeclaration(m_quadVertexDecl);
	m_device->SetStreamSource(0, m_quadVertexBuffer, 0, sizeof(VERTEX));

	RECT clientRect = GetClientRect();

	float screenSizeVector[4] =
	    {
	        static_cast<float>(clientRect.right),
	        static_cast<float>(clientRect.bottom),
	        0,
	        0};

	float bufferSizeVector[4] =
	    {
	        static_cast<float>(pixelBufferBitmap.GetWidth()),
	        static_cast<float>(pixelBufferBitmap.GetHeight()),
	        0,
	        0};

	float panOffsetVector[4] = {m_panX, m_panY, 0, 0};
	float zoomFactorVector[4] = {m_zoomFactor, 0, 0, 0};

	m_device->SetVertexShader(m_pixelBufferViewVertexShader);
	m_device->SetPixelShader(m_pixelBufferViewPixelShader);
	m_device->SetVertexShaderConstantF(0, screenSizeVector, 1);
	m_device->SetVertexShaderConstantF(1, bufferSizeVector, 1);
	m_device->SetVertexShaderConstantF(2, panOffsetVector, 1);
	m_device->SetVertexShaderConstantF(3, zoomFactorVector, 1);

	m_device->SetTexture(0, m_pixelBufferTexture);

	result = m_device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
	assert(SUCCEEDED(result));
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
		case CPixelBufferViewOverlay::COMMAND_PIXELBUFFER_CHANGED:
			CreateSelectedPixelBufferTexture();
			Refresh();
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
		POINT mousePoint = {x, y};
		ScreenToClient(m_hWnd, &mousePoint);
		float relPosX = static_cast<float>(mousePoint.x) / static_cast<float>(clientRect.Right());
		float relPosY = static_cast<float>(mousePoint.y) / static_cast<float>(clientRect.Bottom());
		relPosX = std::max(relPosX, 0.f);
		relPosX = std::min(relPosX, 1.f);
		relPosY = std::max(relPosY, 0.f);
		relPosY = std::min(relPosY, 1.f);

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
	CreateSelectedPixelBufferTexture();
}

void CPixelBufferView::OnDeviceResetting()
{
	m_quadVertexBuffer.Reset();
	m_quadVertexDecl.Reset();
	m_pixelBufferTexture.Reset();
}

const CPixelBufferView::PixelBuffer* CPixelBufferView::GetSelectedPixelBuffer()
{
	if(m_pixelBuffers.empty()) return nullptr;

	int selectedPixelBufferIndex = m_overlay->GetSelectedPixelBufferIndex();
	if(selectedPixelBufferIndex < 0) return nullptr;

	assert(selectedPixelBufferIndex < m_pixelBuffers.size());
	if(selectedPixelBufferIndex >= m_pixelBuffers.size()) return nullptr;

	return &m_pixelBuffers[selectedPixelBufferIndex];
}

void CPixelBufferView::CreateSelectedPixelBufferTexture()
{
	m_pixelBufferTexture.Reset();

	auto pixelBuffer = GetSelectedPixelBuffer();
	if(!pixelBuffer) return;

	m_pixelBufferTexture = CreateTextureFromBitmap(pixelBuffer->second);
}

void CPixelBufferView::OnSaveBitmap()
{
	auto pixelBuffer = GetSelectedPixelBuffer();
	if(!pixelBuffer) return;
	auto pixelBufferBitmap = pixelBuffer->second;

	auto bpp = pixelBufferBitmap.GetBitsPerPixel();
	if((bpp == 24) || (bpp == 32))
	{
		pixelBufferBitmap = ConvertBGRToRGB(std::move(pixelBufferBitmap));
	}

	Framework::Win32::CFileDialog openFileDialog;
	openFileDialog.m_OFN.lpstrFilter = _T("Windows Bitmap Files (*.bmp)\0*.bmp");
	int result = openFileDialog.SummonSave(m_hWnd);
	if(result == IDOK)
	{
		try
		{
			auto outputStream = Framework::CreateOutputStdStream(std::tstring(openFileDialog.m_OFN.lpstrFile));
			Framework::CBMP::WriteBitmap(pixelBufferBitmap, outputStream);
		}
		catch(const std::exception& exception)
		{
			auto message = string_format("Failed to save buffer to file:\r\n\r\n%s", exception.what());
			MessageBoxA(m_hWnd, message.c_str(), nullptr, MB_ICONHAND);
		}
	}
}

Framework::CBitmap CPixelBufferView::ConvertBGRToRGB(Framework::CBitmap bitmap)
{
	for(int32 y = 0; y < bitmap.GetHeight(); y++)
	{
		for(int32 x = 0; x < bitmap.GetWidth(); x++)
		{
			auto color = bitmap.GetPixel(x, y);
			std::swap(color.r, color.b);
			bitmap.SetPixel(x, y, color);
		}
	}
	return std::move(bitmap);
}

void CPixelBufferView::FitBitmap()
{
	auto pixelBuffer = GetSelectedPixelBuffer();
	if(!pixelBuffer) return;
	const auto& pixelBufferBitmap = pixelBuffer->second;

	Framework::Win32::CRect clientRect = GetClientRect();
	unsigned int marginSize = 50;
	float normalizedSizeX = static_cast<float>(pixelBufferBitmap.GetWidth()) / static_cast<float>(clientRect.Right() - marginSize);
	float normalizedSizeY = static_cast<float>(pixelBufferBitmap.GetHeight()) / static_cast<float>(clientRect.Bottom() - marginSize);

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
	        {-1, -1, 0, 0, 1},
	        {-1, 1, 0, 0, 0},
	        {1, -1, 0, 1, 1},
	        {1, 1, 0, 1, 0},
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
			element.Offset = offsetof(VERTEX, position);
			element.Type = D3DDECLTYPE_FLOAT3;
			element.Usage = D3DDECLUSAGE_POSITION;
			vertexElements.push_back(element);
		}

		{
			D3DVERTEXELEMENT9 element = {};
			element.Offset = offsetof(VERTEX, texCoord);
			element.Type = D3DDECLTYPE_FLOAT2;
			element.Usage = D3DDECLUSAGE_TEXCOORD;
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

CPixelBufferView::VertexShaderPtr CPixelBufferView::CreateVertexShaderFromResource(const TCHAR* resourceName)
{
	HRESULT result = S_OK;

	auto shaderResourceInfo = FindResource(GetModuleHandle(nullptr), resourceName, _T("TEXTFILE"));
	assert(shaderResourceInfo != nullptr);

	auto shaderResourceHandle = LoadResource(GetModuleHandle(nullptr), shaderResourceInfo);
	auto shaderResourceSize = SizeofResource(GetModuleHandle(nullptr), shaderResourceInfo);

	auto shaderResource = reinterpret_cast<const char*>(LockResource(shaderResourceHandle));

	UINT compileFlags = 0;
#ifdef _DEBUG
	compileFlags |= D3DCOMPILE_DEBUG;
#endif

	Framework::Win32::CComPtr<ID3DBlob> shaderBinary;
	Framework::Win32::CComPtr<ID3DBlob> compileErrors;
	result = D3DCompile(shaderResource, shaderResourceSize, "vs", nullptr, nullptr, "main",
	                    "vs_3_0", compileFlags, 0, &shaderBinary, &compileErrors);
	assert(SUCCEEDED(result));

	VertexShaderPtr shader;
	result = m_device->CreateVertexShader(reinterpret_cast<DWORD*>(shaderBinary->GetBufferPointer()), &shader);
	assert(SUCCEEDED(result));

	return shader;
}

CPixelBufferView::PixelShaderPtr CPixelBufferView::CreatePixelShaderFromResource(const TCHAR* resourceName)
{
	HRESULT result = S_OK;

	auto shaderResourceInfo = FindResource(GetModuleHandle(nullptr), resourceName, _T("TEXTFILE"));
	assert(shaderResourceInfo != nullptr);

	auto shaderResourceHandle = LoadResource(GetModuleHandle(nullptr), shaderResourceInfo);
	auto shaderResourceSize = SizeofResource(GetModuleHandle(nullptr), shaderResourceInfo);

	auto shaderResource = reinterpret_cast<const char*>(LockResource(shaderResourceHandle));

	UINT compileFlags = 0;
#ifdef _DEBUG
	compileFlags |= D3DCOMPILE_DEBUG;
#endif

	Framework::Win32::CComPtr<ID3DBlob> shaderBinary;
	Framework::Win32::CComPtr<ID3DBlob> compileErrors;
	result = D3DCompile(shaderResource, shaderResourceSize, "ps", nullptr, nullptr, "main",
	                    "ps_3_0", compileFlags, 0, &shaderBinary, &compileErrors);
	assert(SUCCEEDED(result));

	PixelShaderPtr shader;
	result = m_device->CreatePixelShader(reinterpret_cast<DWORD*>(shaderBinary->GetBufferPointer()), &shader);
	assert(SUCCEEDED(result));

	return shader;
}

CPixelBufferView::TexturePtr CPixelBufferView::CreateTextureFromBitmap(const Framework::CBitmap& bitmap)
{
	TexturePtr texture;

	if(!bitmap.IsEmpty())
	{
		D3DFORMAT textureFormat =
		    [bitmap]() {
			    switch(bitmap.GetBitsPerPixel())
			    {
			    case 8:
				    return D3DFMT_L8;
			    case 16:
				    return D3DFMT_A1R5G5B5;
			    case 32:
			    default:
				    return D3DFMT_A8R8G8B8;
			    }
		    }();

		HRESULT result = S_OK;
		result = m_device->CreateTexture(bitmap.GetWidth(), bitmap.GetHeight(), 1, D3DUSAGE_DYNAMIC, textureFormat, D3DPOOL_DEFAULT, &texture, nullptr);
		assert(SUCCEEDED(result));

		switch(bitmap.GetBitsPerPixel())
		{
		case 8:
			CopyBitmapToTexture<uint8>(texture, 0, bitmap);
			break;
		case 16:
			CopyBitmapToTexture<uint16>(texture, 0, bitmap);
			break;
		case 32:
			CopyBitmapToTexture<uint32>(texture, 0, bitmap);
			break;
		default:
			assert(false);
			break;
		}
	}

	return texture;
}
