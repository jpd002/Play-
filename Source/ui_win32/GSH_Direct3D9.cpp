#include "GSH_Direct3D9.h"
#include "../Log.h"
#include "../gs/GsPixelFormats.h"
#include <d3dx9math.h>

#pragma comment (lib, "d3d9.lib")
#pragma comment (lib, "d3dx9.lib")

#define D3D_DEBUG_INFO
//#define _WIREFRAME

struct CUSTOMVERTEX 
{
	float x, y, z; 
	DWORD color;
	float u, v;
};

#define CUSTOMFVF (D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1)

CGSH_Direct3D9::CGSH_Direct3D9(Framework::Win32::CWindow* outputWindow) 
: m_outputWnd(dynamic_cast<COutputWnd*>(outputWindow))
, m_cvtBuffer(nullptr)
, m_sceneBegun(false)
, m_currentTextureWidth(0)
, m_currentTextureHeight(0)
{
	memset(&m_renderState, 0, sizeof(m_renderState));
}

CGSH_Direct3D9::~CGSH_Direct3D9()
{

}

Framework::CBitmap CGSH_Direct3D9::GetFramebuffer(uint64 frameReg)
{
	Framework::CBitmap result;
	m_mailBox.SendCall([&] () { GetFramebufferImpl(result, frameReg); }, true );
	return result;
}

Framework::CBitmap CGSH_Direct3D9::GetTexture(uint64 tex0Reg, uint64 tex1Reg, uint64 clamp)
{
	Framework::CBitmap result;
	m_mailBox.SendCall([&] () { GetTextureImpl(result, tex0Reg, tex1Reg, clamp); }, true);
	return result;
}

const CGSH_Direct3D9::VERTEX* CGSH_Direct3D9::GetInputVertices() const
{
	return m_vtxBuffer;
}

CGSHandler::FactoryFunction CGSH_Direct3D9::GetFactoryFunction(Framework::Win32::CWindow* outputWnd)
{
	return [=] () { return new CGSH_Direct3D9(outputWnd); };
}

void CGSH_Direct3D9::ProcessImageTransfer()
{

}

void CGSH_Direct3D9::ProcessClutTransfer(uint32, uint32)
{

}

void CGSH_Direct3D9::ProcessLocalToLocalTransfer()
{

}

void CGSH_Direct3D9::ReadFramebuffer(uint32, uint32, void*)
{

}

bool CGSH_Direct3D9::GetDepthTestingEnabled() const
{
	return m_depthTestingEnabled;
}

void CGSH_Direct3D9::SetDepthTestingEnabled(bool depthTestingEnabled)
{
	m_depthTestingEnabled = depthTestingEnabled;
	m_renderState.isValid = false;
}

bool CGSH_Direct3D9::GetAlphaBlendingEnabled() const
{
	return m_alphaBlendingEnabled;
}

void CGSH_Direct3D9::SetAlphaBlendingEnabled(bool alphaBlendingEnabled)
{
	m_alphaBlendingEnabled = alphaBlendingEnabled;
	m_renderState.isValid = false;
}

bool CGSH_Direct3D9::GetAlphaTestingEnabled() const
{
	return m_alphaTestingEnabled;
}

void CGSH_Direct3D9::SetAlphaTestingEnabled(bool alphaTestingEnabled)
{
	m_alphaTestingEnabled = alphaTestingEnabled;
	m_renderState.isValid = false;
}

void CGSH_Direct3D9::InitializeImpl()
{
	m_d3d = Direct3DPtr(Direct3DCreate9(D3D_SDK_VERSION));
	CreateDevice();

	m_device->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
	PresentBackbuffer();

	for(unsigned int i = 0; i < MAXCACHE; i++)
	{
		m_cachedTextures.push_back(std::make_shared<CCachedTexture>());
	}

	m_cvtBuffer = new uint8[CVTBUFFERSIZE];
}

void CGSH_Direct3D9::ResetImpl()
{
	memset(&m_vtxBuffer, 0, sizeof(m_vtxBuffer));
	m_framebuffers.clear();
	m_depthbuffers.clear();
	CGSHandler::ResetImpl();
}

void CGSH_Direct3D9::ReleaseImpl()
{
	delete [] m_cvtBuffer;
}

CGSH_Direct3D9::FramebufferPtr CGSH_Direct3D9::FindFramebuffer(uint64 frameReg) const
{
	auto frame = make_convertible<FRAME>(frameReg);

	auto framebufferIterator = std::find_if(std::begin(m_framebuffers), std::end(m_framebuffers), 
		[&] (const FramebufferPtr& framebuffer)
		{
			return (framebuffer->m_basePtr == frame.GetBasePtr()) && (framebuffer->m_width == frame.GetWidth());
		}
	);

	return (framebufferIterator != std::end(m_framebuffers)) ? *(framebufferIterator) : FramebufferPtr();
}

void CGSH_Direct3D9::GetFramebufferImpl(Framework::CBitmap& outputBitmap, uint64 frameReg)
{
	auto framebuffer = FindFramebuffer(frameReg);
	if(!framebuffer) return;
	CopyRenderTargetToBitmap(outputBitmap, framebuffer->m_renderTarget, 
		framebuffer->m_width, framebuffer->m_height, framebuffer->m_width, framebuffer->m_height);
}

CGSH_Direct3D9::DepthbufferPtr CGSH_Direct3D9::FindDepthbuffer(uint64 zbufReg, uint64 frameReg) const
{
	auto zbuf = make_convertible<ZBUF>(zbufReg);
	auto frame = make_convertible<FRAME>(frameReg);

	auto depthbufferIterator = std::find_if(std::begin(m_depthbuffers), std::end(m_depthbuffers), 
		[&] (const DepthbufferPtr& depthBuffer)
		{
			return (depthBuffer->m_basePtr == zbuf.GetBasePtr()) && (depthBuffer->m_width == frame.GetWidth());
		}
	);

	return (depthbufferIterator != std::end(m_depthbuffers)) ? *(depthbufferIterator) : DepthbufferPtr();
}

void CGSH_Direct3D9::GetTextureImpl(Framework::CBitmap& outputBitmap, uint64 tex0Reg, uint64 tex1Reg, uint64 clampReg)
{
	auto tex0 = make_convertible<TEX0>(tex0Reg);
	auto tex1 = make_convertible<TEX1>(tex1Reg);
	auto clamp = make_convertible<CLAMP>(clampReg);
	auto texInfo = LoadTexture(tex0, tex1, clamp);

	if(texInfo.isRenderTarget)
	{
		CopyRenderTargetToBitmap(outputBitmap, texInfo.texture, 
			texInfo.renderTargetWidth, texInfo.renderTargetHeight, tex0.GetWidth(), tex0.GetHeight());
	}
	else
	{
		CopyTextureToBitmap(outputBitmap, texInfo.texture, tex0.GetWidth(), tex0.GetHeight());
	}
}

void CGSH_Direct3D9::CopyTextureToBitmap(Framework::CBitmap& outputBitmap, const TexturePtr& texture, uint32 width, uint32 height)
{
	outputBitmap = Framework::CBitmap(width, height, 32);

	HRESULT result = S_OK;

	D3DLOCKED_RECT lockedRect = {};
	result = texture->LockRect(0, &lockedRect, nullptr, D3DLOCK_READONLY);
	assert(SUCCEEDED(result));

	uint8* srcPtr = reinterpret_cast<uint8*>(lockedRect.pBits);
	uint8* dstPtr = reinterpret_cast<uint8*>(outputBitmap.GetPixels());
	for(unsigned int y = 0; y < height; y++)
	{
		memcpy(dstPtr, srcPtr, width * 4);
		dstPtr += outputBitmap.GetPitch();
		srcPtr += lockedRect.Pitch;
	}

	result = texture->UnlockRect(0);
	assert(SUCCEEDED(result));
}

void CGSH_Direct3D9::CopyRenderTargetToBitmap(Framework::CBitmap& outputBitmap, const TexturePtr& renderTarget, uint32 renderTargetWidth, uint32 renderTargetHeight, uint32 width, uint32 height)
{
	HRESULT result = S_OK;

	SurfacePtr offscreenSurface, renderTargetSurface;

	result = renderTarget->GetSurfaceLevel(0, &renderTargetSurface);
	assert(SUCCEEDED(result));

	result = m_device->CreateOffscreenPlainSurface(renderTargetWidth, renderTargetHeight, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &offscreenSurface, nullptr);
	assert(SUCCEEDED(result));

	result = m_device->GetRenderTargetData(renderTargetSurface, offscreenSurface);
	assert(SUCCEEDED(result));

	outputBitmap = Framework::CBitmap(width, height, 32);

	D3DLOCKED_RECT lockedRect = {};
	result = offscreenSurface->LockRect(&lockedRect, nullptr, D3DLOCK_READONLY);
	assert(SUCCEEDED(result));

	uint8* srcPtr = reinterpret_cast<uint8*>(lockedRect.pBits);
	uint8* dstPtr = reinterpret_cast<uint8*>(outputBitmap.GetPixels());
	uint32 copyWidth = std::min<uint32>(renderTargetWidth, width);
	uint32 copyHeight = std::min<uint32>(renderTargetHeight, height);
	for(unsigned int y = 0; y < copyHeight; y++)
	{
		memcpy(dstPtr, srcPtr, copyWidth * 4);
		dstPtr += outputBitmap.GetPitch();
		srcPtr += lockedRect.Pitch;
	}

	result = offscreenSurface->UnlockRect();
	assert(SUCCEEDED(result));
}

void CGSH_Direct3D9::BeginScene()
{
	if(!m_sceneBegun)
	{
#ifdef _WIREFRAME
		m_device->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 0.0f, 0);
#endif
		HRESULT result = m_device->BeginScene();
		assert(result == S_OK);
		m_sceneBegun = true;
	}
}

void CGSH_Direct3D9::EndScene()
{
	if(m_sceneBegun)
	{
		HRESULT result = m_device->EndScene();
		assert(result == S_OK);
		m_sceneBegun = false;
	}
}

void CGSH_Direct3D9::PresentBackbuffer()
{
	if(!m_device.IsEmpty())
	{
		HRESULT result = S_OK;

		EndScene();
		if(TestDevice())
		{
			result = m_device->Present(NULL, NULL, NULL, NULL);
			assert(SUCCEEDED(result));
		}
		BeginScene();
	}
}

void CGSH_Direct3D9::FlipImpl()
{
	m_renderState.isValid = false;
	PresentBackbuffer();
	CGSHandler::FlipImpl();
}

void CGSH_Direct3D9::SetReadCircuitMatrix(int nWidth, int nHeight)
{
	//Setup projection matrix
	{
		D3DXMATRIX projMatrix;
		D3DXMatrixOrthoLH(&projMatrix, static_cast<FLOAT>(nWidth), static_cast<FLOAT>(nHeight), 1.0f, 0.0f);
		m_device->SetTransform(D3DTS_PROJECTION, &projMatrix);
	}

	//Setup view matrix
	{
		D3DXMATRIX viewMatrix;
		D3DXMatrixLookAtLH(&viewMatrix,
						   &D3DXVECTOR3(0.0f, 0.0f, 1.0f),
						   &D3DXVECTOR3(0.0f, 0.0f, 0.0f),
						   &D3DXVECTOR3(0.0f, -1.0f, 0.0f));

		m_device->SetTransform(D3DTS_VIEW, &viewMatrix);
	}

	//Setup world matrix
	{
		D3DXMATRIX worldMatrix;
		D3DXMatrixTranslation(&worldMatrix, -static_cast<FLOAT>(nWidth) / 2.0f, -static_cast<FLOAT>(nHeight) / 2.0f, 0);
		m_device->SetTransform(D3DTS_WORLD, &worldMatrix);
	}
}

bool CGSH_Direct3D9::TestDevice()
{
	HRESULT coopLevelResult = m_device->TestCooperativeLevel();
	if(FAILED(coopLevelResult))
	{
		if(coopLevelResult == D3DERR_DEVICELOST)
		{
			return false;
		}
		else if(coopLevelResult == D3DERR_DEVICENOTRESET)
		{
			OnDeviceResetting();
			auto presentParams = CreatePresentParams();
			HRESULT result = m_device->Reset(&presentParams);
			if(FAILED(result))
			{
				assert(0);
				return false;
			}
			OnDeviceReset();
		}
		else
		{
			assert(0);
		}
	}

	return true;
}

D3DPRESENT_PARAMETERS CGSH_Direct3D9::CreatePresentParams()
{
	RECT clientRect = m_outputWnd->GetClientRect();
	unsigned int outputWidth = clientRect.right;
	unsigned int outputHeight = clientRect.bottom;

	D3DPRESENT_PARAMETERS d3dpp;
	memset(&d3dpp, 0, sizeof(D3DPRESENT_PARAMETERS));
	d3dpp.Windowed					= TRUE;
	d3dpp.SwapEffect				= D3DSWAPEFFECT_DISCARD;
	d3dpp.hDeviceWindow				= m_outputWnd->m_hWnd;
	d3dpp.BackBufferFormat			= D3DFMT_X8R8G8B8;
	d3dpp.BackBufferWidth			= outputWidth;
	d3dpp.BackBufferHeight			= outputHeight;
	d3dpp.EnableAutoDepthStencil	= TRUE;
	d3dpp.AutoDepthStencilFormat	= D3DFMT_D24S8;
	return d3dpp;
}

void CGSH_Direct3D9::CreateDevice()
{
	auto presentParams = CreatePresentParams();
	HRESULT result = S_OK;
	result = m_d3d->CreateDevice(D3DADAPTER_DEFAULT,
						D3DDEVTYPE_HAL,
						m_outputWnd->m_hWnd,
						D3DCREATE_SOFTWARE_VERTEXPROCESSING,
						&presentParams,
						&m_device);
	assert(SUCCEEDED(result));

	OnDeviceReset();

	m_sceneBegun = false;
	BeginScene();
}

void CGSH_Direct3D9::OnDeviceReset()
{
	HRESULT result = S_OK;

#ifdef _WIREFRAME
	m_device->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
#endif
	m_device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	m_device->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
	m_device->SetRenderState(D3DRS_LIGHTING, FALSE);
	m_device->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);

	result = m_device->CreateVertexBuffer(3 * sizeof(CUSTOMVERTEX), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, CUSTOMFVF, D3DPOOL_DEFAULT, &m_triangleVb, NULL);
	assert(SUCCEEDED(result));

	result = m_device->CreateVertexBuffer(4 * sizeof(CUSTOMVERTEX), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, CUSTOMFVF, D3DPOOL_DEFAULT, &m_quadVb, NULL);
	assert(SUCCEEDED(result));

	m_renderState.isValid = false;
}

void CGSH_Direct3D9::OnDeviceResetting()
{
	m_triangleVb.Reset();
	m_quadVb.Reset();
	m_framebuffers.clear();
	m_depthbuffers.clear();
	TexCache_Flush();
	m_currentTexture.Reset();
}

uint8 CGSH_Direct3D9::MulBy2Clamp(uint8 nValue)
{
	return (nValue > 0x7F) ? 0xFF : (nValue << 1);
}

float CGSH_Direct3D9::GetZ(float nZ)
{
	if(nZ < 256)
	{
		//The number is small, so scale to a smaller ratio (65536)
		return nZ / 32768.0f;
	}
	else
	{
//		nZ -= m_nMaxZ;
		if(nZ > m_nMaxZ) return 1.0;
//		if(nZ < -m_nMaxZ) return -1.0;
		return nZ / m_nMaxZ;
	}
}

void CGSH_Direct3D9::Prim_Line()
{
	float nU1 = 0, nU2 = 0;
	float nV1 = 0, nV2 = 0;
	float nF1 = 0, nF2 = 0;

	XYZ vertex[2];
	vertex[0] <<= m_vtxBuffer[1].nPosition;
	vertex[1] <<= m_vtxBuffer[0].nPosition;

	float nX1 = vertex[0].GetX(), nX2 = vertex[1].GetX();
	float nY1 = vertex[0].GetY(), nY2 = vertex[1].GetY();
	float nZ1 = vertex[0].GetZ(), nZ2 = vertex[1].GetZ();

	RGBAQ rgbaq[2];
	rgbaq[0] <<= m_vtxBuffer[1].nRGBAQ;
	rgbaq[1] <<= m_vtxBuffer[0].nRGBAQ;

	nX1 -= m_nPrimOfsX;
	nX2 -= m_nPrimOfsX;

	nY1 -= m_nPrimOfsY;
	nY2 -= m_nPrimOfsY;

	nZ1 = GetZ(nZ1);
	nZ2 = GetZ(nZ2);

	if(m_primitiveMode.nShading)
	{
		m_device->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);
	}
	else
	{
		m_device->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_FLAT);
	}

	if(m_primitiveMode.nFog)
	{
		//glEnable(GL_FOG);

		nF1 = (float)(0xFF - m_vtxBuffer[1].nFog) / 255.0f;
		nF2 = (float)(0xFF - m_vtxBuffer[0].nFog) / 255.0f;
	}
	else
	{
		nF1 = nF2 = 0.0;
	}

	if(m_primitiveMode.nTexture)
	{
		m_device->SetTexture(0, m_currentTexture);

		//Textured triangle
		if(m_primitiveMode.nUseUV)
		{
			UV uv[2];
			uv[0] <<= m_vtxBuffer[1].nUV;
			uv[1] <<= m_vtxBuffer[0].nUV;

			nU1 = uv[0].GetU() / static_cast<float>(m_currentTextureWidth);
			nU2 = uv[1].GetU() / static_cast<float>(m_currentTextureWidth);

			nV1 = uv[0].GetV() / static_cast<float>(m_currentTextureHeight);
			nV2 = uv[1].GetV() / static_cast<float>(m_currentTextureHeight);
		}
		else
		{
			ST st[2];
			st[0] <<= m_vtxBuffer[1].nST;
			st[1] <<= m_vtxBuffer[0].nST;

			nU1 = st[0].nS, nU2 = st[1].nS;
			nV1 = st[0].nT, nV2 = st[1].nT;

			nU1 /= rgbaq[0].nQ;
			nU2 /= rgbaq[1].nQ;

			nV1 /= rgbaq[0].nQ;
			nV2 /= rgbaq[1].nQ;
		}
	}
	else
	{
		m_device->SetTexture(0, NULL);
	}

	//Build vertex buffer
	{
		HRESULT result = S_OK;

		DWORD color0 = D3DCOLOR_ARGB(MulBy2Clamp(rgbaq[0].nA),	MulBy2Clamp(rgbaq[0].nR),	MulBy2Clamp(rgbaq[0].nG), MulBy2Clamp(rgbaq[0].nB));
		DWORD color1 = D3DCOLOR_ARGB(MulBy2Clamp(rgbaq[1].nA),	MulBy2Clamp(rgbaq[1].nR),	MulBy2Clamp(rgbaq[1].nG), MulBy2Clamp(rgbaq[1].nB));

		CUSTOMVERTEX vertices[] =
		{
			{	nX1,	nY1,	nZ1,	color0,		nU1,	nV1 },
			{	nX2,	nY2,	nZ2,	color1,		nU2,	nV2 },
		};

		uint8* buffer = NULL;
		result = m_triangleVb->Lock(0, sizeof(CUSTOMVERTEX) * 3, reinterpret_cast<void**>(&buffer), D3DLOCK_DISCARD);
		assert(result == S_OK);
		{
			memcpy(buffer, vertices, sizeof(vertices));
		}
		result = m_triangleVb->Unlock();
		assert(result == S_OK);

		// select which vertex format we are using
		result = m_device->SetFVF(CUSTOMFVF);
		assert(result == S_OK);

		// select the vertex buffer to display
		result = m_device->SetStreamSource(0, m_triangleVb, 0, sizeof(CUSTOMVERTEX));
		assert(result == S_OK);

		// copy the vertex buffer to the back buffer
		result = m_device->DrawPrimitive(D3DPT_LINELIST, 0, 1);
		assert(result == S_OK);
	}

	if(m_primitiveMode.nFog)
	{
		//glDisable(GL_FOG);
	}
}

void CGSH_Direct3D9::Prim_Triangle()
{
	float nU1 = 0, nU2 = 0, nU3 = 0;
	float nV1 = 0, nV2 = 0, nV3 = 0;
	float nF1 = 0, nF2 = 0, nF3 = 0;

	XYZ vertex[3];
	vertex[0] <<= m_vtxBuffer[2].nPosition;
	vertex[1] <<= m_vtxBuffer[1].nPosition;
	vertex[2] <<= m_vtxBuffer[0].nPosition;

	float nX1 = vertex[0].GetX(), nX2 = vertex[1].GetX(), nX3 = vertex[2].GetX();
	float nY1 = vertex[0].GetY(), nY2 = vertex[1].GetY(), nY3 = vertex[2].GetY();
	float nZ1 = vertex[0].GetZ(), nZ2 = vertex[1].GetZ(), nZ3 = vertex[2].GetZ();

	RGBAQ rgbaq[3];
	rgbaq[0] <<= m_vtxBuffer[2].nRGBAQ;
	rgbaq[1] <<= m_vtxBuffer[1].nRGBAQ;
	rgbaq[2] <<= m_vtxBuffer[0].nRGBAQ;

	nX1 -= m_nPrimOfsX;
	nX2 -= m_nPrimOfsX;
	nX3 -= m_nPrimOfsX;

	nY1 -= m_nPrimOfsY;
	nY2 -= m_nPrimOfsY;
	nY3 -= m_nPrimOfsY;

	nZ1 = GetZ(nZ1);
	nZ2 = GetZ(nZ2);
	nZ3 = GetZ(nZ3);

	if(m_primitiveMode.nShading)
	{
		m_device->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);
	}
	else
	{
		m_device->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_FLAT);
	}

	if(m_primitiveMode.nFog)
	{
		//glEnable(GL_FOG);

		nF1 = (float)(0xFF - m_vtxBuffer[2].nFog) / 255.0f;
		nF2 = (float)(0xFF - m_vtxBuffer[1].nFog) / 255.0f;
		nF3 = (float)(0xFF - m_vtxBuffer[0].nFog) / 255.0f;
	}
	else
	{
		nF1 = nF2 = nF3 = 0.0;
	}

	if(m_primitiveMode.nTexture)
	{
		m_device->SetTexture(0, m_currentTexture);

		//Textured triangle
		if(m_primitiveMode.nUseUV)
		{
			UV uv[3];
			uv[0] <<= m_vtxBuffer[2].nUV;
			uv[1] <<= m_vtxBuffer[1].nUV;
			uv[2] <<= m_vtxBuffer[0].nUV;

			nU1 = uv[0].GetU() / static_cast<float>(m_currentTextureWidth);
			nU2 = uv[1].GetU() / static_cast<float>(m_currentTextureWidth);
			nU3 = uv[2].GetU() / static_cast<float>(m_currentTextureWidth);

			nV1 = uv[0].GetV() / static_cast<float>(m_currentTextureHeight);
			nV2 = uv[1].GetV() / static_cast<float>(m_currentTextureHeight);
			nV3 = uv[2].GetV() / static_cast<float>(m_currentTextureHeight);
		}
		else
		{
			ST st[3];
			st[0] <<= m_vtxBuffer[2].nST;
			st[1] <<= m_vtxBuffer[1].nST;
			st[2] <<= m_vtxBuffer[0].nST;

			nU1 = st[0].nS, nU2 = st[1].nS, nU3 = st[2].nS;
			nV1 = st[0].nT, nV2 = st[1].nT, nV3 = st[2].nT;

			nU1 /= rgbaq[0].nQ;
			nU2 /= rgbaq[1].nQ;
			nU3 /= rgbaq[2].nQ;

			nV1 /= rgbaq[0].nQ;
			nV2 /= rgbaq[1].nQ;
			nV3 /= rgbaq[2].nQ;
		}
	}
	else
	{
		m_device->SetTexture(0, NULL);
	}

	//Build vertex buffer
	{
		HRESULT result = S_OK;

		DWORD color0 = D3DCOLOR_ARGB(MulBy2Clamp(rgbaq[0].nA),	MulBy2Clamp(rgbaq[0].nR),	MulBy2Clamp(rgbaq[0].nG), MulBy2Clamp(rgbaq[0].nB));
		DWORD color1 = D3DCOLOR_ARGB(MulBy2Clamp(rgbaq[1].nA),	MulBy2Clamp(rgbaq[1].nR),	MulBy2Clamp(rgbaq[1].nG), MulBy2Clamp(rgbaq[1].nB));
		DWORD color2 = D3DCOLOR_ARGB(MulBy2Clamp(rgbaq[2].nA),	MulBy2Clamp(rgbaq[2].nR),	MulBy2Clamp(rgbaq[2].nG), MulBy2Clamp(rgbaq[2].nB));

		CUSTOMVERTEX vertices[] =
		{
			{	nX1,	nY1,	nZ1,	color0,		nU1,	nV1 },
			{	nX2,	nY2,	nZ2,	color1,		nU2,	nV2 },
			{	nX3,	nY3,	nZ3,	color2,		nU3,	nV3 },
		};

		uint8* buffer = NULL;
		result = m_triangleVb->Lock(0, sizeof(CUSTOMVERTEX) * 3, reinterpret_cast<void**>(&buffer), D3DLOCK_DISCARD);
		assert(result == S_OK);
		{
			memcpy(buffer, vertices, sizeof(vertices));
		}
		result = m_triangleVb->Unlock();
		assert(result == S_OK);

		// select which vertex format we are using
		result = m_device->SetFVF(CUSTOMFVF);
		assert(result == S_OK);

		// select the vertex buffer to display
		result = m_device->SetStreamSource(0, m_triangleVb, 0, sizeof(CUSTOMVERTEX));
		assert(result == S_OK);

		// copy the vertex buffer to the back buffer
		result = m_device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1);
		assert(result == S_OK);
	}

	if(m_primitiveMode.nFog)
	{
		//glDisable(GL_FOG);
	}
}

void CGSH_Direct3D9::Prim_Sprite()
{
	float nU1 = 0, nU2 = 0;
	float nV1 = 0, nV2 = 0;
	float nF1 = 0, nF2 = 0;

	XYZ vertex[2];
	vertex[0] <<= m_vtxBuffer[1].nPosition;
	vertex[1] <<= m_vtxBuffer[0].nPosition;

	float nX1 = vertex[0].GetX(), nY1 = vertex[0].GetY();
	float nX2 = vertex[1].GetX(), nY2 = vertex[1].GetY(), nZ = vertex[1].GetZ();

	RGBAQ rgbaq[2];
	rgbaq[0] <<= m_vtxBuffer[1].nRGBAQ;
	rgbaq[1] <<= m_vtxBuffer[0].nRGBAQ;

	nX1 -= m_nPrimOfsX;
	nX2 -= m_nPrimOfsX;

	nY1 -= m_nPrimOfsY;
	nY2 -= m_nPrimOfsY;

	nZ = GetZ(nZ);

	if(m_primitiveMode.nTexture)
	{
		m_device->SetTexture(0, m_currentTexture);

		//Textured triangle
		if(m_primitiveMode.nUseUV)
		{
			UV uv[2];
			uv[0] <<= m_vtxBuffer[1].nUV;
			uv[1] <<= m_vtxBuffer[0].nUV;

			nU1 = uv[0].GetU() / static_cast<float>(m_currentTextureWidth);
			nU2 = uv[1].GetU() / static_cast<float>(m_currentTextureWidth);

			nV1 = uv[0].GetV() / static_cast<float>(m_currentTextureHeight);
			nV2 = uv[1].GetV() / static_cast<float>(m_currentTextureHeight);
		}
		else
		{
			//TODO
		}
	}
	else
	{
		m_device->SetTexture(0, NULL);
	}

	{
		HRESULT result;

		DWORD color0 = D3DCOLOR_ARGB(MulBy2Clamp(rgbaq[0].nA),	MulBy2Clamp(rgbaq[0].nR),	MulBy2Clamp(rgbaq[0].nG), MulBy2Clamp(rgbaq[0].nB));
		DWORD color1 = D3DCOLOR_ARGB(MulBy2Clamp(rgbaq[1].nA),	MulBy2Clamp(rgbaq[1].nR),	MulBy2Clamp(rgbaq[1].nG), MulBy2Clamp(rgbaq[1].nB));

		CUSTOMVERTEX vertices[] =
		{
			{	nX1,	nY2,	nZ,		color0,		nU1,	nV2 },
			{	nX1,	nY1,	nZ,		color0,		nU1,	nV1 },
			{	nX2,	nY2,	nZ,		color1,		nU2,	nV2 },
			{	nX2,	nY1,	nZ,		color1,		nU2,	nV1 },
		};

		uint8* buffer = NULL;
		result = m_quadVb->Lock(0, sizeof(CUSTOMVERTEX) * 4, reinterpret_cast<void**>(&buffer), D3DLOCK_DISCARD);
		assert(SUCCEEDED(result));
		{
			memcpy(buffer, vertices, sizeof(vertices));
		}
		result = m_quadVb->Unlock();
		assert(SUCCEEDED(result));

		// select which vertex format we are using
		result = m_device->SetFVF(CUSTOMFVF);
		assert(SUCCEEDED(result));

		// select the vertex buffer to display
		result = m_device->SetStreamSource(0, m_quadVb, 0, sizeof(CUSTOMVERTEX));
		assert(SUCCEEDED(result));

		// copy the vertex buffer to the back buffer
		result = m_device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
		assert(SUCCEEDED(result));
	}
}

void CGSH_Direct3D9::SetRenderingContext(uint64 primReg)
{
	auto prim = make_convertible<PRMODE>(primReg);

	unsigned int context = prim.nContext;

	uint64 testReg = m_nReg[GS_REG_TEST_1 + context];
	uint64 frameReg = m_nReg[GS_REG_FRAME_1 + context];
	uint64 alphaReg = m_nReg[GS_REG_ALPHA_1 + context];
	uint64 zbufReg = m_nReg[GS_REG_ZBUF_1 + context];
	uint64 tex0Reg = m_nReg[GS_REG_TEX0_1 + context];
	uint64 tex1Reg = m_nReg[GS_REG_TEX1_1 + context];
	uint64 clampReg = m_nReg[GS_REG_CLAMP_1 + context];

	if(!m_renderState.isValid ||
		(m_renderState.primReg != primReg))
	{
		m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, ((prim.nAlpha != 0) && m_alphaBlendingEnabled) ? TRUE : FALSE);
	}

	if(!m_renderState.isValid ||
		(m_renderState.alphaReg != alphaReg))
	{
		SetupBlendingFunction(alphaReg);
	}

	if(!m_renderState.isValid ||
		(m_renderState.testReg != testReg))
	{
		SetupTestFunctions(testReg);
	}

	if(!m_renderState.isValid ||
		(m_renderState.zbufReg != zbufReg) ||
		(m_renderState.frameReg != frameReg))
	{
		SetupDepthBuffer(zbufReg, frameReg);
	}

	if(!m_renderState.isValid ||
		(m_renderState.frameReg != frameReg))
	{
		SetupFramebuffer(frameReg);
	}

	if(!m_renderState.isValid ||
		(m_renderState.tex0Reg != tex0Reg) ||
		(m_renderState.tex1Reg != tex1Reg) ||
		(m_renderState.clampReg != clampReg))
	{
		SetupTexture(tex0Reg, tex1Reg, clampReg);
	}

	m_renderState.isValid = true;
	m_renderState.primReg = primReg;
	m_renderState.alphaReg = alphaReg;
	m_renderState.testReg = testReg;
	m_renderState.zbufReg = zbufReg;
	m_renderState.frameReg = frameReg;
	m_renderState.tex0Reg = tex0Reg;
	m_renderState.tex1Reg = tex1Reg;
	m_renderState.clampReg = clampReg;

	auto offset = make_convertible<XYOFFSET>(m_nReg[GS_REG_XYOFFSET_1 + context]);
	m_nPrimOfsX = offset.GetX();
	m_nPrimOfsY = offset.GetY();
	
	if(GetCrtIsInterlaced() && GetCrtIsFrameMode())
	{
		if(m_nCSR & CSR_FIELD)
		{
			m_nPrimOfsY += 0.5;
		}
	}
}

void CGSH_Direct3D9::SetupBlendingFunction(uint64 alphaReg)
{
	auto alpha = make_convertible<ALPHA>(alphaReg);

	m_device->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, D3DZB_TRUE);
	m_device->SetRenderState(D3DRS_SRCBLENDALPHA, D3DBLEND_ONE);
	m_device->SetRenderState(D3DRS_DESTBLENDALPHA, D3DBLEND_ZERO);

	if((alpha.nA == 0) && (alpha.nB == 1) && (alpha.nC == 0) && (alpha.nD == 1))
	{
		m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	}
	else if((alpha.nA == 0) && (alpha.nB == 1) && (alpha.nC == 1) && (alpha.nD == 1))
	{
		m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_DESTALPHA);
		m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVDESTALPHA);
	}
	else if((alpha.nA == 0) && (alpha.nB == 1) && (alpha.nC == 2) && (alpha.nD == 1))
	{
		//(Cs - Cd) * FIX + Cd
		//		-> FIX * Cs + (1 - FIX) * Cd

		uint8 fix = MulBy2Clamp(alpha.nFix);
		m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_BLENDFACTOR);
		m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVBLENDFACTOR);
		m_device->SetRenderState(D3DRS_BLENDFACTOR, D3DCOLOR_ARGB(fix, fix, fix, fix));
	}
	else if((alpha.nA == 0) && (alpha.nB == 2) && (alpha.nC == 2) && (alpha.nD == 1) && (alpha.nFix == 0x80))
	{
		m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
		m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
	}
	else if((alpha.nA == 0) && (alpha.nB == 2) && (alpha.nC == 0) && (alpha.nD == 1))
	{
		m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
	}
	else if((alpha.nA == 1) && (alpha.nB == 0) && (alpha.nC == 0) && (alpha.nD == 0))
	{
		//(Cd - Cs) * As + Cs
		m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_INVSRCALPHA);
		m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_SRCALPHA);
	}
	else if((alpha.nA == 1) && (alpha.nB == 2) && (alpha.nC == 0) && (alpha.nD == 2))
	{
		//Cd * As
		m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ZERO);
		m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_SRCALPHA);

		m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ZERO);
		m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
	}
	else if((alpha.nA == 2) && (alpha.nB == 1) && (alpha.nC == 2) && (alpha.nD == 1))
	{
		//(0 - Cd) * FIX + Cd 
		//		-> 0 * Cs + (1 - FIX) * Cd

		uint8 fix = MulBy2Clamp(alpha.nFix);
		m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ZERO);
		m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVBLENDFACTOR);
		m_device->SetRenderState(D3DRS_BLENDFACTOR, D3DCOLOR_ARGB(fix, fix, fix, fix));
	}
}

void CGSH_Direct3D9::SetupTestFunctions(uint64 nData)
{
	TEST tst;
	tst <<= nData;

	if(tst.nAlphaEnabled)
	{
		static const D3DCMPFUNC g_alphaTestFunc[ALPHA_TEST_MAX] =
		{
			D3DCMP_NEVER,
			D3DCMP_ALWAYS,
			D3DCMP_LESS,
			D3DCMP_LESSEQUAL,
			D3DCMP_EQUAL,
			D3DCMP_GREATEREQUAL,
			D3DCMP_GREATER,
			D3DCMP_NOTEQUAL
		};

		//If alpha test is set to always fail but don't keep fragment info, we need to set
		//proper masks at in other places
		if(tst.nAlphaMethod == ALPHA_TEST_NEVER && tst.nAlphaFail != ALPHA_TEST_FAIL_KEEP)
		{
			m_device->SetRenderState(D3DRS_ALPHATESTENABLE, D3DZB_FALSE);
		}
		else
		{
			m_device->SetRenderState(D3DRS_ALPHAFUNC, g_alphaTestFunc[tst.nAlphaMethod]);
			m_device->SetRenderState(D3DRS_ALPHAREF, tst.nAlphaRef);
			m_device->SetRenderState(D3DRS_ALPHATESTENABLE, m_alphaTestingEnabled ? D3DZB_TRUE : D3DZB_FALSE);
		}
	}
	else
	{
		m_device->SetRenderState(D3DRS_ALPHATESTENABLE, D3DZB_FALSE);
	}

	if(tst.nDepthEnabled)
	{
		unsigned int depthFunc = D3DCMP_NEVER;

		switch(tst.nDepthMethod)
		{
		case 0:
			depthFunc = D3DCMP_NEVER;
			break;
		case 1:
			depthFunc = D3DCMP_ALWAYS;
			break;
		case 2:
			depthFunc = D3DCMP_GREATEREQUAL;
			break;
		case 3:
			depthFunc = D3DCMP_GREATER;
			break;
		}

		m_device->SetRenderState(D3DRS_ZENABLE, m_depthTestingEnabled ? D3DZB_TRUE : D3DZB_FALSE);
		m_device->SetRenderState(D3DRS_ZFUNC, depthFunc);
	}
	else
	{
		m_device->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
	}
}

void CGSH_Direct3D9::SetupTexture(uint64 tex0Reg, uint64 tex1Reg, uint64 clampReg)
{
	if(tex0Reg == 0)
	{
		m_currentTexture.Reset();
		return;
	}

	auto tex0 = make_convertible<TEX0>(tex0Reg);
	auto tex1 = make_convertible<TEX1>(tex1Reg);
	auto clamp = make_convertible<CLAMP>(clampReg);

	auto texInfo = LoadTexture(tex0, tex1, clamp);

	m_currentTextureWidth	= tex0.GetWidth();
	m_currentTextureHeight	= tex0.GetHeight();
	m_currentTexture		= texInfo.texture;

	int nMagFilter = D3DTEXF_NONE, nMinFilter = D3DTEXF_NONE, nMipFilter = D3DTEXF_NONE;
	int nWrapS = 0, nWrapT = 0;

	if(tex1.nMagFilter == 0)
	{
		nMagFilter = D3DTEXF_POINT;
	}
	else
	{
		nMagFilter = D3DTEXF_LINEAR;
	}

	switch(tex1.nMinFilter)
	{
	case 0:
		nMinFilter = D3DTEXF_POINT;
		break;
	case 1:
		nMinFilter = D3DTEXF_LINEAR;
		break;
	case 5:
		nMinFilter = D3DTEXF_LINEAR;
		nMipFilter = D3DTEXF_LINEAR;
		break;
	default:
		assert(0);
		break;
	}

	m_device->SetSamplerState(0, D3DSAMP_MINFILTER, nMinFilter);
	m_device->SetSamplerState(0, D3DSAMP_MAGFILTER, nMagFilter);
	m_device->SetSamplerState(0, D3DSAMP_MIPFILTER, nMipFilter);
}

void CGSH_Direct3D9::SetupFramebuffer(uint64 frameReg)
{
	if(frameReg == 0) return;

	auto frame = make_convertible<FRAME>(frameReg);

	{
		bool r = (frame.nMask & 0x000000FF) == 0;
		bool g = (frame.nMask & 0x0000FF00) == 0;
		bool b = (frame.nMask & 0x00FF0000) == 0;
		bool a = (frame.nMask & 0xFF000000) == 0;
		UINT colorMask = 
			(r ? D3DCOLORWRITEENABLE_RED : 0) |
			(g ? D3DCOLORWRITEENABLE_GREEN : 0) |
			(b ? D3DCOLORWRITEENABLE_BLUE : 0) |
			(a ? D3DCOLORWRITEENABLE_ALPHA : 0);
		m_device->SetRenderState(D3DRS_COLORWRITEENABLE, colorMask);
	}

	auto framebuffer = FindFramebuffer(frameReg);
	if(!framebuffer)
	{
		framebuffer = FramebufferPtr(new CFramebuffer(m_device, frame.GetBasePtr(), frame.GetWidth(), 1024, frame.nPsm));
		m_framebuffers.push_back(framebuffer);
	}

	//Any framebuffer selected at this point can be used as a texture
	framebuffer->m_canBeUsedAsTexture = true;

	float projWidth = static_cast<float>(framebuffer->m_width);
	float projHeight = static_cast<float>(framebuffer->m_height);

	HRESULT result = S_OK;
	Framework::Win32::CComPtr<IDirect3DSurface9> renderSurface;
	result = framebuffer->m_renderTarget->GetSurfaceLevel(0, &renderSurface);
	assert(SUCCEEDED(result));

	result = m_device->SetRenderTarget(0, renderSurface);
	assert(SUCCEEDED(result));

	SetReadCircuitMatrix(projWidth, projHeight);
}

void CGSH_Direct3D9::SetupDepthBuffer(uint64 zbufReg, uint64 frameReg)
{
	auto frame = make_convertible<FRAME>(frameReg);
	auto zbuf = make_convertible<ZBUF>(zbufReg);

	auto depthbuffer = FindDepthbuffer(zbufReg, frameReg);
	if(!depthbuffer)
	{
		depthbuffer = DepthbufferPtr(new CDepthbuffer(m_device, zbuf.GetBasePtr(), frame.GetWidth(), 1024, zbuf.nPsm));
		m_depthbuffers.push_back(depthbuffer);
	}

	switch(CGsPixelFormats::GetPsmPixelSize(zbuf.nPsm))
	{
	case 16:
		m_nMaxZ = 32768.0f;
		break;
	case 24:
		m_nMaxZ = 8388608.0f;
		break;
	default:
	case 32:
		m_nMaxZ = 2147483647.0f;
		break;
	}

	HRESULT result = m_device->SetDepthStencilSurface(depthbuffer->m_depthSurface);
	assert(SUCCEEDED(result));

	m_device->SetRenderState(D3DRS_ZWRITEENABLE, (zbuf.nMask == 0) ? TRUE : FALSE);
}

void CGSH_Direct3D9::WriteRegisterImpl(uint8 nRegister, uint64 nData)
{
	CGSHandler::WriteRegisterImpl(nRegister, nData);

	switch(nRegister)
	{
	case GS_REG_PRIM:
		m_primitiveType = static_cast<unsigned int>(nData & 0x07);
		switch(m_primitiveType)
		{
		case PRIM_POINT:
			m_vtxCount = 1;
			break;
		case PRIM_LINE:
		case PRIM_LINESTRIP:
			m_vtxCount = 2;
			break;
		case PRIM_TRIANGLE:
		case PRIM_TRIANGLESTRIP:
		case PRIM_TRIANGLEFAN:
			m_vtxCount = 3;
			break;
		case PRIM_SPRITE:
			m_vtxCount = 2;
			break;
		}
		break;

	case GS_REG_XYZ2:
	case GS_REG_XYZ3:
	case GS_REG_XYZF2:
	case GS_REG_XYZF3:
		VertexKick(nRegister, nData);
		break;

	//case GS_REG_FOGCOL:
	//	SetupFogColor();
	//	break;
	}
}

void CGSH_Direct3D9::VertexKick(uint8 nRegister, uint64 nValue)
{
	if(m_vtxCount == 0) return;

	bool drawingKick = (nRegister == GS_REG_XYZ2) || (nRegister == GS_REG_XYZF2);
	bool fog = (nRegister == GS_REG_XYZF2) || (nRegister == GS_REG_XYZF3);

	if(fog)
	{
		m_vtxBuffer[m_vtxCount - 1].nPosition	= nValue & 0x00FFFFFFFFFFFFFFULL;
		m_vtxBuffer[m_vtxCount - 1].nRGBAQ		= m_nReg[GS_REG_RGBAQ];
		m_vtxBuffer[m_vtxCount - 1].nUV			= m_nReg[GS_REG_UV];
		m_vtxBuffer[m_vtxCount - 1].nST			= m_nReg[GS_REG_ST];
		m_vtxBuffer[m_vtxCount - 1].nFog		= (uint8)(nValue >> 56);
	}
	else
	{
		m_vtxBuffer[m_vtxCount - 1].nPosition	= nValue;
		m_vtxBuffer[m_vtxCount - 1].nRGBAQ		= m_nReg[GS_REG_RGBAQ];
		m_vtxBuffer[m_vtxCount - 1].nUV			= m_nReg[GS_REG_UV];
		m_vtxBuffer[m_vtxCount - 1].nST			= m_nReg[GS_REG_ST];
		m_vtxBuffer[m_vtxCount - 1].nFog		= (uint8)(m_nReg[GS_REG_FOG] >> 56);
	}

	m_vtxCount--;

	if(m_vtxCount == 0)
	{
		{
			if((m_nReg[GS_REG_PRMODECONT] & 1) != 0)
			{
				m_primitiveMode <<= m_nReg[GS_REG_PRIM];
			}
			else
			{
				m_primitiveMode <<= m_nReg[GS_REG_PRMODE];
			}

			SetRenderingContext(m_primitiveMode);

			switch(m_primitiveType)
			{
			case PRIM_POINT:
				//if(drawingKick) Prim_Point();
				break;
			case PRIM_LINE:
				if(drawingKick) Prim_Line();
				break;
			case PRIM_LINESTRIP:
				if(drawingKick) Prim_Line();
				memcpy(&m_vtxBuffer[1], &m_vtxBuffer[0], sizeof(VERTEX));
				m_vtxCount = 1;
				break;
			case PRIM_TRIANGLE:
				if(drawingKick) Prim_Triangle();
				m_vtxCount = 3;
				break;
			case PRIM_TRIANGLESTRIP:
				if(drawingKick) Prim_Triangle();
				memcpy(&m_vtxBuffer[2], &m_vtxBuffer[1], sizeof(VERTEX));
				memcpy(&m_vtxBuffer[1], &m_vtxBuffer[0], sizeof(VERTEX));
				m_vtxCount = 1;
				break;
			case PRIM_TRIANGLEFAN:
				if(drawingKick) Prim_Triangle();
				memcpy(&m_vtxBuffer[1], &m_vtxBuffer[0], sizeof(VERTEX));
				m_vtxCount = 1;
				break;
			case PRIM_SPRITE:
				if(drawingKick) Prim_Sprite();
				m_vtxCount = 2;
				break;
			}
		}
	}
}

/////////////////////////////////////////////////////////////
// Framebuffer
/////////////////////////////////////////////////////////////

CGSH_Direct3D9::CFramebuffer::CFramebuffer(DevicePtr& device, uint32 basePtr, uint32 width, uint32 height, uint32 psm)
: m_basePtr(basePtr)
, m_width(width)
, m_height(height)
, m_psm(psm)
, m_canBeUsedAsTexture(false)
{
	HRESULT result = S_OK;
	
	result = D3DXCreateTexture(device, width, height, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8B8G8R8, D3DPOOL_DEFAULT, &m_renderTarget);
	assert(SUCCEEDED(result));
}

CGSH_Direct3D9::CFramebuffer::~CFramebuffer()
{

}

/////////////////////////////////////////////////////////////
// Depthbuffer
/////////////////////////////////////////////////////////////

CGSH_Direct3D9::CDepthbuffer::CDepthbuffer(DevicePtr& device, uint32 basePtr, uint32 width, uint32 height, uint32 psm)
: m_basePtr(basePtr)
, m_width(width)
, m_height(height)
, m_psm(psm)
{
	HRESULT result = S_OK;
	
	result = device->CreateDepthStencilSurface(width, height, D3DFMT_D24S8, D3DMULTISAMPLE_NONE, 0, FALSE, &m_depthSurface, nullptr);
	assert(SUCCEEDED(result));
}

CGSH_Direct3D9::CDepthbuffer::~CDepthbuffer()
{

}
