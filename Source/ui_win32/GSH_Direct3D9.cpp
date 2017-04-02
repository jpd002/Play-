#include "GSH_Direct3D9.h"
#include "../Log.h"
#include "../gs/GsPixelFormats.h"
#include "D3D9TextureUtils.h"
#include <d3dx9math.h>

#pragma comment (lib, "d3d9.lib")
#pragma comment (lib, "d3dx9.lib")

#define D3D_DEBUG_INFO
//#define _WIREFRAME

struct CUSTOMVERTEX
{
	float x, y, z;
	DWORD color;
	float s, t, q;
};

struct PRESENTVERTEX
{
	float x, y, z;
	float u, v;
};

#define PRESENTFVF (D3DFVF_XYZ | D3DFVF_TEX1)

static const PRESENTVERTEX g_presentVertices[] = 
{
	// X   Y  Z  U  V
	{ -1, -1, 0, 0, 1 },
	{  1, -1, 0, 1, 1 },
	{ -1,  1, 0, 0, 0 },
	{  1,  1, 0, 1, 0 }
};

CGSH_Direct3D9::CGSH_Direct3D9(Framework::Win32::CWindow* outputWindow) 
: m_outputWnd(dynamic_cast<COutputWnd*>(outputWindow))
{
	memset(&m_renderState, 0, sizeof(m_renderState));
	m_primitiveMode <<= 0;
}

Framework::CBitmap CGSH_Direct3D9::GetFramebuffer(uint64 frameReg)
{
	Framework::CBitmap result;
	m_mailBox.SendCall([&] () { result = GetFramebufferImpl(frameReg); }, true );
	return result;
}

Framework::CBitmap CGSH_Direct3D9::GetTexture(uint64 tex0Reg, uint32 maxMip, uint64 miptbp1Reg, uint64 miptbp2Reg, uint32 mipLevel)
{
	Framework::CBitmap result;
	m_mailBox.SendCall([&] () { result = GetTextureImpl(tex0Reg, maxMip, miptbp1Reg, miptbp2Reg, mipLevel); }, true);
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

void CGSH_Direct3D9::ProcessHostToLocalTransfer()
{
	auto bltBuf = make_convertible<BITBLTBUF>(m_nReg[GS_REG_BITBLTBUF]);
	uint32 transferAddress = bltBuf.GetDstPtr();

	if(m_trxCtx.nDirty)
	{
		//FlushVertexBuffer();
		m_renderState.isValid = false;

		auto trxReg = make_convertible<TRXREG>(m_nReg[GS_REG_TRXREG]);
		auto trxPos = make_convertible<TRXPOS>(m_nReg[GS_REG_TRXPOS]);

		//Find the pages that are touched by this transfer
		auto transferPageSize = CGsPixelFormats::GetPsmPageSize(bltBuf.nDstPsm);

		uint32 pageCountX = (bltBuf.GetDstWidth() + transferPageSize.first - 1) / transferPageSize.first;
		uint32 pageCountY = (trxReg.nRRH + transferPageSize.second - 1) / transferPageSize.second;

		uint32 pageCount = pageCountX * pageCountY;
		uint32 transferSize = pageCount * CGsPixelFormats::PAGESIZE;
		uint32 transferOffset = (trxPos.nDSAY / transferPageSize.second) * pageCountX * CGsPixelFormats::PAGESIZE;

		m_textureCache.InvalidateRange(transferAddress + transferOffset, transferSize);

#if 0
		bool isUpperByteTransfer = (bltBuf.nDstPsm == PSMT8H) || (bltBuf.nDstPsm == PSMT4HL) || (bltBuf.nDstPsm == PSMT4HH);
		for(const auto& framebuffer : m_framebuffers)
		{
			if((framebuffer->m_psm == PSMCT24) && isUpperByteTransfer) continue;
			framebuffer->m_cachedArea.Invalidate(transferAddress + transferOffset, transferSize);
		}
#endif
	}
}

void CGSH_Direct3D9::ProcessLocalToHostTransfer()
{
	//This is constrained to work only with ps2autotest, will be unconstrained later

	auto bltBuf = make_convertible<BITBLTBUF>(m_nReg[GS_REG_BITBLTBUF]);
	auto trxPos = make_convertible<TRXPOS>(m_nReg[GS_REG_TRXPOS]);
	auto trxReg = make_convertible<TRXREG>(m_nReg[GS_REG_TRXREG]);

	if(bltBuf.nSrcPsm != PSMCT32) return;

	uint32 transferAddress = bltBuf.GetSrcPtr();
	if(transferAddress != 0) return;

	if((trxReg.nRRW != 32) || (trxReg.nRRH != 32)) return;
	if((trxPos.nSSAX != 0) || (trxPos.nSSAY != 0)) return;

	auto framebufferIterator = std::find_if(m_framebuffers.begin(), m_framebuffers.end(), 
		[] (const FramebufferPtr& framebuffer)
		{
			return (framebuffer->m_psm == PSMCT32) && (framebuffer->m_basePtr == 0);
		}
	);
	if(framebufferIterator == std::end(m_framebuffers)) return;
	const auto& framebuffer = (*framebufferIterator);

	m_renderState.isValid = false;

	auto renderTarget = framebuffer->m_renderTarget;
	SurfacePtr offscreenSurface, renderTargetSurface;

	HRESULT result = S_OK;

	result = renderTarget->GetSurfaceLevel(0, &renderTargetSurface);
	assert(SUCCEEDED(result));

	result = m_device->CreateOffscreenPlainSurface(framebuffer->m_width, framebuffer->m_height, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &offscreenSurface, nullptr);
	assert(SUCCEEDED(result));

	result = m_device->GetRenderTargetData(renderTargetSurface, offscreenSurface);
	assert(SUCCEEDED(result));

	D3DLOCKED_RECT lockedRect = {};
	result = offscreenSurface->LockRect(&lockedRect, nullptr, D3DLOCK_READONLY);
	assert(SUCCEEDED(result));

	auto pixels = reinterpret_cast<uint32*>(lockedRect.pBits);

	//Write back to RAM
	{
		CGsPixelFormats::CPixelIndexorPSMCT32 indexor(m_pRAM, bltBuf.GetSrcPtr(), bltBuf.nSrcWidth);
		for(uint32 y = trxPos.nSSAY; y < (trxPos.nSSAY + trxReg.nRRH); y++)
		{
			for(uint32 x = trxPos.nSSAX; x < (trxPos.nSSAX + trxReg.nRRH); x++)
			{
				uint32 pixel = pixels[x + (y * lockedRect.Pitch / 4)];
				indexor.SetPixel(x, y, pixel);
			}
		}
	}

	result = offscreenSurface->UnlockRect();
	assert(SUCCEEDED(result));
}

void CGSH_Direct3D9::ProcessLocalToLocalTransfer()
{

}

void CGSH_Direct3D9::ProcessClutTransfer(uint32, uint32)
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

	m_cvtBuffer = new uint8[CVTBUFFERSIZE];

	SetupTextureUpdaters();
}

void CGSH_Direct3D9::ResetImpl()
{
	memset(&m_vtxBuffer, 0, sizeof(m_vtxBuffer));
	m_framebuffers.clear();
	m_depthbuffers.clear();
	m_textureCache.Flush();
	m_renderState.isValid = false;
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

Framework::CBitmap CGSH_Direct3D9::GetFramebufferImpl(uint64 frameReg)
{
	auto framebuffer = FindFramebuffer(frameReg);
	if(!framebuffer)
	{
		return Framework::CBitmap();
	}
	else
	{
		return CreateBitmapFromRenderTarget(framebuffer->m_renderTarget,
			framebuffer->m_width, framebuffer->m_height, framebuffer->m_width, framebuffer->m_height);
	}
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

Framework::CBitmap CGSH_Direct3D9::GetTextureImpl(uint64 tex0Reg, uint32 maxMip, uint64 miptbp1Reg, uint64 miptbp2Reg, uint32 mipLevel)
{
	auto tex0 = make_convertible<TEX0>(tex0Reg);
	auto miptbp1 = make_convertible<MIPTBP1>(miptbp1Reg);
	auto miptbp2 = make_convertible<MIPTBP2>(miptbp2Reg);
	auto texInfo = LoadTexture(tex0, maxMip, miptbp1, miptbp2);

	if(texInfo.isRenderTarget)
	{
		return CreateBitmapFromRenderTarget(texInfo.texture, 
			texInfo.renderTargetWidth, texInfo.renderTargetHeight, tex0.GetWidth(), tex0.GetHeight());
	}
	else
	{
		return CreateBitmapFromTexture(texInfo.texture, tex0.GetWidth(), tex0.GetHeight(), tex0.nPsm, mipLevel);
	}
}

Framework::CBitmap CGSH_Direct3D9::CreateBitmapFromTexture(const TexturePtr& texture, uint32 width, uint32 height, uint8 format, uint32 mipLevel)
{
	width = std::max<uint32>(width >> mipLevel, 1);
	height = std::max<uint32>(height >> mipLevel, 1);

	uint32 bitsPerPixel = 
		[format]()
		{
			switch(format)
			{
			case PSMCT16:
				return 16;
			case PSMT4:
			case PSMT8:
			case PSMT8H:
			case PSMT4HL:
				return 8;
			default:
				assert(false);
			case PSMCT32:
				return 32;
			}
		}();

	auto bitmap = Framework::CBitmap(width, height, bitsPerPixel);

	switch(bitsPerPixel)
	{
	case 8:
		CopyTextureToBitmap<uint8>(bitmap, texture, mipLevel);
		break;
	case 16:
		CopyTextureToBitmap<uint16>(bitmap, texture, mipLevel);
		break;
	case 32:
		CopyTextureToBitmap<uint32>(bitmap, texture, mipLevel);
		break;
	default:
		assert(false);
		break;
	}

	return bitmap;
}

Framework::CBitmap CGSH_Direct3D9::CreateBitmapFromRenderTarget(const TexturePtr& renderTarget, uint32 renderTargetWidth, uint32 renderTargetHeight, uint32 width, uint32 height)
{
	HRESULT result = S_OK;

	SurfacePtr offscreenSurface, renderTargetSurface;

	result = renderTarget->GetSurfaceLevel(0, &renderTargetSurface);
	assert(SUCCEEDED(result));

	result = m_device->CreateOffscreenPlainSurface(renderTargetWidth, renderTargetHeight, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &offscreenSurface, nullptr);
	assert(SUCCEEDED(result));

	result = m_device->GetRenderTargetData(renderTargetSurface, offscreenSurface);
	assert(SUCCEEDED(result));

	auto bitmap = Framework::CBitmap(width, height, 32);

	D3DLOCKED_RECT lockedRect = {};
	result = offscreenSurface->LockRect(&lockedRect, nullptr, D3DLOCK_READONLY);
	assert(SUCCEEDED(result));

	auto srcPtr = reinterpret_cast<uint8*>(lockedRect.pBits);
	auto dstPtr = reinterpret_cast<uint8*>(bitmap.GetPixels());
	uint32 copyWidth = std::min<uint32>(renderTargetWidth, width);
	uint32 copyHeight = std::min<uint32>(renderTargetHeight, height);
	for(unsigned int y = 0; y < copyHeight; y++)
	{
		memcpy(dstPtr, srcPtr, copyWidth * 4);
		dstPtr += bitmap.GetPitch();
		srcPtr += lockedRect.Pitch;
	}

	result = offscreenSurface->UnlockRect();
	assert(SUCCEEDED(result));

	return bitmap;
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

unsigned int CGSH_Direct3D9::GetCurrentReadCircuit()
{
	//assert((m_nPMODE & 0x3) != 0x03);
	if(m_nPMODE & 0x1) return 0;
	if(m_nPMODE & 0x2) return 1;
	//Getting here is bad
	return 0;
}

void CGSH_Direct3D9::FlipImpl()
{
	DrawActiveFramebuffer();
	PresentBackbuffer();
	CGSHandler::FlipImpl();
}

void CGSH_Direct3D9::DrawActiveFramebuffer()
{
	HRESULT result = S_OK;

	m_renderState.isValid = false;
	DISPLAY d;
	DISPFB fb;
	{
		std::lock_guard<std::recursive_mutex> registerMutexLock(m_registerMutex);
		unsigned int readCircuit = GetCurrentReadCircuit();
		switch(readCircuit)
		{
		case 0:
			d <<= m_nDISPLAY1.value.q;
			fb <<= m_nDISPFB1.value.q;
			break;
		case 1:
			d <<= m_nDISPLAY2.value.q;
			fb <<= m_nDISPFB2.value.q;
			break;
		}
	}

	unsigned int dispWidth = (d.nW + 1) / (d.nMagX + 1);
	unsigned int dispHeight = (d.nH + 1);

	bool halfHeight = GetCrtIsInterlaced() && GetCrtIsFrameMode();
	if(halfHeight) dispHeight /= 2;

	FramebufferPtr framebuffer;
	for(const auto& candidateFramebuffer : m_framebuffers)
	{
		if(
			(candidateFramebuffer->m_basePtr == fb.GetBufPtr()) &&
			//(GetFramebufferBitDepth(candidateFramebuffer->m_psm) == GetFramebufferBitDepth(fb.nPSM)) &&
			(candidateFramebuffer->m_width == fb.GetBufWidth())
			)
		{
			//We have a winner
			framebuffer = candidateFramebuffer;
			break;
		}
	}

	//TODO: Create new framebuffer if none is found
	//TODO: Commit dirty framebuffer pages to video memory
	//TODO: Resolve multisample

	{
		SurfacePtr backbuffer;
		result = m_device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &backbuffer);
		assert(SUCCEEDED(result));

		result = m_device->SetRenderTarget(0, backbuffer);
		assert(SUCCEEDED(result));
	}

	m_device->SetVertexDeclaration(nullptr);
	m_device->SetVertexShader(nullptr);
	m_device->SetPixelShader(nullptr);

	D3DVIEWPORT9 viewport = {};
	viewport.X = 0;
	viewport.Y = 0;
	viewport.Width  = m_presentationParams.windowWidth;
	viewport.Height = m_presentationParams.windowHeight;
	result = m_device->SetViewport(&viewport);
	assert(SUCCEEDED(result));

	result = m_device->Clear(1, nullptr, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 0, 0);
	assert(SUCCEEDED(result));

	D3DXMATRIX identityMatrix = {};
	D3DXMatrixIdentity(&identityMatrix);
	m_device->SetTransform(D3DTS_PROJECTION, &identityMatrix);
	m_device->SetTransform(D3DTS_VIEW, &identityMatrix);
	m_device->SetTransform(D3DTS_WORLD, &identityMatrix);

	if(framebuffer)
	{
		float u1 = static_cast<float>(dispWidth) / static_cast<float>(framebuffer->m_width);
		float v1 = static_cast<float>(dispHeight) / static_cast<float>(framebuffer->m_height);

		D3DXMATRIX textureMatrix = {};
		D3DXMatrixScaling(&textureMatrix, u1, v1, 1);
		m_device->SetTransform(D3DTS_TEXTURE0, &textureMatrix);

		result = m_device->SetStreamSource(0, m_presentVb, 0, sizeof(PRESENTVERTEX));
		assert(SUCCEEDED(result));

		result = m_device->SetFVF(PRESENTFVF);
		assert(SUCCEEDED(result));

		result = m_device->SetTexture(0, framebuffer->m_renderTarget);
		assert(SUCCEEDED(result));

		result = m_device->SetTexture(1, nullptr);
		assert(SUCCEEDED(result));

		result = m_device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
		assert(SUCCEEDED(result));
	}
}

void CGSH_Direct3D9::SetReadCircuitMatrix(int width, int height)
{
	//Setup projection matrix
	D3DXMATRIX projMatrix;
	D3DXMatrixOrthoLH(&projMatrix, static_cast<FLOAT>(width), static_cast<FLOAT>(height), 1.0f, 0.0f);

	//Setup view matrix
	D3DXMATRIX viewMatrix;
	D3DXMatrixLookAtLH(&viewMatrix,
						&D3DXVECTOR3(0.0f, 0.0f, 1.0f),
						&D3DXVECTOR3(0.0f, 0.0f, 0.0f),
						&D3DXVECTOR3(0.0f, -1.0f, 0.0f));

	//Setup world matrix
	D3DXMATRIX worldMatrix;
	D3DXMatrixTranslation(&worldMatrix, -static_cast<FLOAT>(width) / 2.0f, -static_cast<FLOAT>(height) / 2.0f, 0);

	D3DXMATRIX tempMatrix;
	D3DXMatrixMultiply(&tempMatrix, &worldMatrix, &viewMatrix);
	D3DXMatrixMultiply(reinterpret_cast<D3DXMATRIX*>(&m_projMatrix), &tempMatrix, &projMatrix);
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

	auto clientRect = m_outputWnd->GetClientRect();
	bool sizeChanged = 
		(clientRect.Width() != m_deviceWindowWidth) ||
		(clientRect.Height() != m_deviceWindowHeight);
	if(sizeChanged)
	{
		OnDeviceResetting();
		auto presentParams = CreatePresentParams();
		HRESULT result = m_device->Reset(&presentParams);
		if(FAILED(result))
		{
			assert(0);
		}
		OnDeviceReset();
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

	result = m_device->CreateVertexBuffer(4 * sizeof(CUSTOMVERTEX), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, &m_drawVb, nullptr);
	assert(SUCCEEDED(result));

	result = m_device->CreateVertexBuffer(4 * sizeof(PRESENTVERTEX), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, PRESENTFVF, D3DPOOL_DEFAULT, &m_presentVb, nullptr);
	assert(SUCCEEDED(result));

	static const D3DVERTEXELEMENT9 vertexElements[] =
	{
		{ 0, offsetof(CUSTOMVERTEX, x),     D3DDECLTYPE_FLOAT3,   0, D3DDECLUSAGE_POSITION, 0 },
		{ 0, offsetof(CUSTOMVERTEX, s),     D3DDECLTYPE_FLOAT3,   0, D3DDECLUSAGE_TEXCOORD, 0 },
		{ 0, offsetof(CUSTOMVERTEX, color), D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_TEXCOORD, 1 },
		D3DDECL_END()
	};

	result = m_device->CreateVertexDeclaration(vertexElements, &m_vertexDeclaration);
	assert(SUCCEEDED(result));

	{
		uint8* buffer = nullptr;
		result = m_presentVb->Lock(0, sizeof(g_presentVertices), reinterpret_cast<void**>(&buffer), D3DLOCK_DISCARD);
		assert(SUCCEEDED(result));

		memcpy(buffer, g_presentVertices, sizeof(g_presentVertices));

		result = m_presentVb->Unlock();
		assert(SUCCEEDED(result));
	}

	auto clientRect = m_outputWnd->GetClientRect();
	m_deviceWindowWidth = clientRect.Width();
	m_deviceWindowHeight = clientRect.Height();

	result = m_device->CreateTexture(256, 1, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &m_clutTexture, nullptr);
	assert(SUCCEEDED(result));

	m_renderState.isValid = false;
}

void CGSH_Direct3D9::OnDeviceResetting()
{
	m_drawVb.Reset();
	m_presentVb.Reset();
	m_vertexDeclaration.Reset();
	m_framebuffers.clear();
	m_depthbuffers.clear();
	m_textureCache.Flush();
	m_clutTexture.Reset();
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
	float nQ1 = 1, nQ2 = 1;
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

			nQ1 = rgbaq[0].nQ; nQ2 = rgbaq[1].nQ;
		}
	}

	//Build vertex buffer
	{
		HRESULT result = S_OK;

		DWORD color0 = D3DCOLOR_ARGB(rgbaq[0].nA, rgbaq[0].nR, rgbaq[0].nG, rgbaq[0].nB);
		DWORD color1 = D3DCOLOR_ARGB(rgbaq[1].nA, rgbaq[1].nR, rgbaq[1].nG, rgbaq[1].nB);

		CUSTOMVERTEX vertices[] =
		{
			{ nX1, nY1, nZ1, color0, nU1, nV1, nQ1 },
			{ nX2, nY2, nZ2, color1, nU2, nV2, nQ2 },
		};

		uint8* buffer = nullptr;
		result = m_drawVb->Lock(0, sizeof(CUSTOMVERTEX) * 3, reinterpret_cast<void**>(&buffer), D3DLOCK_DISCARD);
		assert(SUCCEEDED(result));
		{
			memcpy(buffer, vertices, sizeof(vertices));
		}
		result = m_drawVb->Unlock();
		assert(SUCCEEDED(result));

		// select which vertex format we are using
		result = m_device->SetVertexDeclaration(m_vertexDeclaration);
		assert(SUCCEEDED(result));

		// select the vertex buffer to display
		result = m_device->SetStreamSource(0, m_drawVb, 0, sizeof(CUSTOMVERTEX));
		assert(SUCCEEDED(result));

		// copy the vertex buffer to the back buffer
		result = m_device->DrawPrimitive(D3DPT_LINELIST, 0, 1);
		assert(SUCCEEDED(result));
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
	float nQ1 = 1, nQ2 = 1, nQ3 = 1;
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
			nQ1 = rgbaq[0].nQ; nQ2 = rgbaq[1].nQ; nQ3 = rgbaq[2].nQ;
		}
	}

	//Build vertex buffer
	{
		HRESULT result = S_OK;

		DWORD color0 = D3DCOLOR_ARGB(rgbaq[0].nA, rgbaq[0].nR, rgbaq[0].nG, rgbaq[0].nB);
		DWORD color1 = D3DCOLOR_ARGB(rgbaq[1].nA, rgbaq[1].nR, rgbaq[1].nG, rgbaq[1].nB);
		DWORD color2 = D3DCOLOR_ARGB(rgbaq[2].nA, rgbaq[2].nR, rgbaq[2].nG, rgbaq[2].nB);

		CUSTOMVERTEX vertices[] =
		{
			{ nX1, nY1, nZ1, color0, nU1, nV1, nQ1 },
			{ nX2, nY2, nZ2, color1, nU2, nV2, nQ2 },
			{ nX3, nY3, nZ3, color2, nU3, nV3, nQ3 },
		};

		uint8* buffer = nullptr;
		result = m_drawVb->Lock(0, sizeof(CUSTOMVERTEX) * 3, reinterpret_cast<void**>(&buffer), D3DLOCK_DISCARD);
		assert(SUCCEEDED(result));
		{
			memcpy(buffer, vertices, sizeof(vertices));
		}
		result = m_drawVb->Unlock();
		assert(SUCCEEDED(result));

		// select which vertex format we are using
		result = m_device->SetVertexDeclaration(m_vertexDeclaration);
		assert(SUCCEEDED(result));

		// select the vertex buffer to display
		result = m_device->SetStreamSource(0, m_drawVb, 0, sizeof(CUSTOMVERTEX));
		assert(SUCCEEDED(result));

		// copy the vertex buffer to the back buffer
		result = m_device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1);
		assert(SUCCEEDED(result));
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

	{
		HRESULT result;

		DWORD color1 = D3DCOLOR_ARGB(rgbaq[1].nA, rgbaq[1].nR, rgbaq[1].nG, rgbaq[1].nB);

		CUSTOMVERTEX vertices[] =
		{
			{ nX1, nY2, nZ, color1, nU1, nV2, 1 },
			{ nX1, nY1, nZ, color1, nU1, nV1, 1 },
			{ nX2, nY2, nZ, color1, nU2, nV2, 1 },
			{ nX2, nY1, nZ, color1, nU2, nV1, 1 },
		};

		uint8* buffer = nullptr;
		result = m_drawVb->Lock(0, sizeof(CUSTOMVERTEX) * 4, reinterpret_cast<void**>(&buffer), D3DLOCK_DISCARD);
		assert(SUCCEEDED(result));
		{
			memcpy(buffer, vertices, sizeof(vertices));
		}
		result = m_drawVb->Unlock();
		assert(SUCCEEDED(result));

		// select which vertex format we are using
		result = m_device->SetVertexDeclaration(m_vertexDeclaration);
		assert(SUCCEEDED(result));

		// select the vertex buffer to display
		result = m_device->SetStreamSource(0, m_drawVb, 0, sizeof(CUSTOMVERTEX));
		assert(SUCCEEDED(result));

		// copy the vertex buffer to the back buffer
		result = m_device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
		assert(SUCCEEDED(result));
	}
}

void CGSH_Direct3D9::FillShaderCapsFromTexture(SHADERCAPS& shaderCaps, const uint64& tex0Reg)
{
	auto tex0 = make_convertible<TEX0>(tex0Reg);

	shaderCaps.texSourceMode = TEXTURE_SOURCE_MODE_STD;

	if(CGsPixelFormats::IsPsmIDTEX(tex0.nPsm))
	{
		shaderCaps.texSourceMode = CGsPixelFormats::IsPsmIDTEX4(tex0.nPsm) ? TEXTURE_SOURCE_MODE_IDX4 : TEXTURE_SOURCE_MODE_IDX8;
	}

	shaderCaps.texFunction = tex0.nFunction;
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
	uint64 miptbp1Reg = m_nReg[GS_REG_MIPTBP1_1 + context];
	uint64 miptbp2Reg = m_nReg[GS_REG_MIPTBP2_1 + context];
	uint64 clampReg = m_nReg[GS_REG_CLAMP_1 + context];
	uint64 scissorReg = m_nReg[GS_REG_SCISSOR_1 + context];

	//Get shader caps
	auto shaderCaps = make_convertible<SHADERCAPS>(0);
	FillShaderCapsFromTexture(shaderCaps, tex0Reg);

	if(!prim.nTexture)
	{
		shaderCaps.texSourceMode = TEXTURE_SOURCE_MODE_NONE;
	}

	{
		auto shaderIterator = m_vertexShaders.find(shaderCaps);
		if(shaderIterator == std::end(m_vertexShaders))
		{
			auto shader = CreateVertexShader(shaderCaps);

			m_vertexShaders.insert(std::make_pair(shaderCaps, shader));
			shaderIterator = m_vertexShaders.find(shaderCaps);
		}
		m_device->SetVertexShader(shaderIterator->second);
	}

	{
		auto shaderIterator = m_pixelShaders.find(shaderCaps);
		if(shaderIterator == std::end(m_pixelShaders))
		{
			auto shader = CreatePixelShader(shaderCaps);

			m_pixelShaders.insert(std::make_pair(shaderCaps, shader));
			shaderIterator = m_pixelShaders.find(shaderCaps);
		}
		m_device->SetPixelShader(shaderIterator->second);
	}

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
		(m_renderState.frameReg != frameReg) ||
		(m_renderState.scissorReg != scissorReg))
	{
		SetupFramebuffer(frameReg, scissorReg);
	}

	if(!m_renderState.isValid ||
		(m_renderState.tex0Reg != tex0Reg) ||
		(m_renderState.tex1Reg != tex1Reg) ||
		(m_renderState.clampReg != clampReg))
	{
		SetupTexture(tex0Reg, tex1Reg, miptbp1Reg, miptbp2Reg, clampReg);
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
	m_renderState.scissorReg = scissorReg;

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

	if((alpha.nA == alpha.nB) && (alpha.nD == ALPHABLEND_ABD_CS))
	{
		//ab*0 (when a == b) - Cs
		m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
		m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
	}
	else if((alpha.nA == alpha.nB) && (alpha.nD == ALPHABLEND_ABD_CD))
	{
		//ab*1 (when a == b) - Cd
		m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ZERO);
		m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
	}
	else if((alpha.nA == alpha.nB) && (alpha.nD == ALPHABLEND_ABD_ZERO))
	{
		//ab*2 (when a == b) - Zero
		m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ZERO);
		m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
	}
	else if((alpha.nA == 0) && (alpha.nB == 1) && (alpha.nC == 0) && (alpha.nD == 1))
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

		uint8 fix = alpha.nFix;
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
	else if((alpha.nA == ALPHABLEND_ABD_CS) && (alpha.nB == ALPHABLEND_ABD_ZERO) && (alpha.nC == ALPHABLEND_C_AS) && (alpha.nD == ALPHABLEND_ABD_ZERO))
	{
		//0202 - Cs * As
		m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
	}
	else if((alpha.nA == 1) && (alpha.nB == 0) && (alpha.nC == 0) && (alpha.nD == 0))
	{
		//(Cd - Cs) * As + Cs
		m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_INVSRCALPHA);
		m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_SRCALPHA);
	}
	else if((alpha.nA == ALPHABLEND_ABD_CD) && (alpha.nB == ALPHABLEND_ABD_CS) && (alpha.nC == ALPHABLEND_C_AD) && (alpha.nD == ALPHABLEND_ABD_CS))
	{
		//1010 -> Cs * (1 - Ad) + Cd * Ad
		m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_INVDESTALPHA);
		m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_DESTALPHA);
	}
	else if((alpha.nA == 1) && (alpha.nB == 2) && (alpha.nC == 0) && (alpha.nD == 2))
	{
		//Cd * As
		m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ZERO);
		m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_SRCALPHA);

		m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ZERO);
		m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
	}
	else if((alpha.nA == ALPHABLEND_ABD_ZERO) && (alpha.nB == ALPHABLEND_ABD_CD) && (alpha.nC == ALPHABLEND_C_AS) && (alpha.nD == ALPHABLEND_ABD_CD))
	{
		//2101 -> Cd * (1 - As)
		m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ZERO);
		m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	}
	else if((alpha.nA == 2) && (alpha.nB == 1) && (alpha.nC == 2) && (alpha.nD == 1))
	{
		//(0 - Cd) * FIX + Cd 
		//		-> 0 * Cs + (1 - FIX) * Cd

		uint8 fix = alpha.nFix;
		m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ZERO);
		m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVBLENDFACTOR);
		m_device->SetRenderState(D3DRS_BLENDFACTOR, D3DCOLOR_ARGB(fix, fix, fix, fix));
	}
	else
	{
		assert(0);
		//Default blending
		m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
		m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
	}
}

void CGSH_Direct3D9::SetupTestFunctions(uint64 testReg)
{
	auto tst = make_convertible<TEST>(testReg);

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

void CGSH_Direct3D9::SetupTexture(uint64 tex0Reg, uint64 tex1Reg, uint64 miptbp1Reg, uint64 miptbp2Reg, uint64 clampReg)
{
	if(tex0Reg == 0)
	{
		return;
	}

	auto tex0 = make_convertible<TEX0>(tex0Reg);
	auto tex1 = make_convertible<TEX1>(tex1Reg);
	auto miptbp1 = make_convertible<MIPTBP1>(miptbp1Reg);
	auto miptbp2 = make_convertible<MIPTBP2>(miptbp2Reg);
	auto clamp = make_convertible<CLAMP>(clampReg);

	auto texInfo = LoadTexture(tex0, tex1.nMaxMip, miptbp1, miptbp2);

	m_currentTextureWidth	= tex0.GetWidth();
	m_currentTextureHeight	= tex0.GetHeight();

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

	if(CGsPixelFormats::IsPsmIDTEX(tex0.nPsm))
	{
		//Make sure point sampling is used, filtering will be done in shader
		if(nMinFilter == D3DTEXF_LINEAR) nMinFilter = D3DTEXF_POINT;
		if(nMagFilter == D3DTEXF_LINEAR) nMagFilter = D3DTEXF_POINT;
		if(nMipFilter == D3DTEXF_LINEAR) nMipFilter = D3DTEXF_POINT;

		auto clutTexture = GetClutTexture(tex0);

		m_device->SetTexture(1, m_clutTexture);
		m_device->SetSamplerState(1, D3DSAMP_MINFILTER, D3DTEXF_POINT);
		m_device->SetSamplerState(1, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
		m_device->SetSamplerState(1, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
		m_device->SetSamplerState(1, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
		m_device->SetSamplerState(1, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
	}

	m_device->SetTexture(0, texInfo.texture);
	m_device->SetSamplerState(0, D3DSAMP_MINFILTER, nMinFilter);
	m_device->SetSamplerState(0, D3DSAMP_MAGFILTER, nMagFilter);
	m_device->SetSamplerState(0, D3DSAMP_MIPFILTER, nMipFilter);
}

void CGSH_Direct3D9::SetupFramebuffer(uint64 frameReg, uint64 scissorReg)
{
	if(frameReg == 0) return;

	auto frame = make_convertible<FRAME>(frameReg);
	auto scissor = make_convertible<SCISSOR>(scissorReg);

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

	bool newFramebuffer = false;
	auto framebuffer = FindFramebuffer(frameReg);
	if(!framebuffer)
	{
		framebuffer = FramebufferPtr(new CFramebuffer(m_device, frame.GetBasePtr(), frame.GetWidth(), 1024, frame.nPsm));
		m_framebuffers.push_back(framebuffer);
		newFramebuffer = true;
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

	if(newFramebuffer)
	{
		//TODO: Get actual contents from GS RAM
		m_device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
		m_device->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0f, 0);
	}

	RECT scissorRect = {};
	scissorRect.left = scissor.scax0;
	scissorRect.top = scissor.scay0;
	scissorRect.right = scissor.scax1 + 1;
	scissorRect.bottom = scissor.scay1 + 1;
	m_device->SetScissorRect(&scissorRect);
	m_device->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);

	SetReadCircuitMatrix(projWidth, projHeight);

	m_device->SetVertexShaderConstantF(0, reinterpret_cast<float*>(&m_projMatrix), 4);
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
				m_vtxCount = 1;
				break;
			case PRIM_LINE:
				if(drawingKick) Prim_Line();
				m_vtxCount = 2;
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
	
	result = D3DXCreateTexture(device, width, height, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &m_renderTarget);
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
