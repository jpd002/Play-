#include "GSH_Direct3D9.h"
#include "PtrMacro.h"
#include "../Log.h"
#include <d3dx9math.h>

using namespace Framework;
using namespace std;
using namespace std::tr1;

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

CGSH_Direct3D9::CGSH_Direct3D9(Win32::CWindow* outputWindow) :
m_pOutputWnd(dynamic_cast<COutputWnd*>(outputWindow)),
m_d3d(NULL),
m_device(NULL),
m_triangleVb(NULL),
m_pCvtBuffer(NULL),
m_nWidth(0),
m_nHeight(0),
m_sceneBegun(false),
m_nTexWidth(0),
m_nTexHeight(0),
m_nTexHandle(NULL)
{

}

CGSH_Direct3D9::~CGSH_Direct3D9()
{

}

CGSHandler::FactoryFunction CGSH_Direct3D9::GetFactoryFunction(Win32::CWindow* pOutputWnd)
{
    return bind(&CGSH_Direct3D9::GSHandlerFactory, pOutputWnd);
}

void CGSH_Direct3D9::ProcessImageTransfer(uint32 nAddress, uint32 nLenght)
{

}

void CGSH_Direct3D9::InitializeImpl()
{
	m_d3d = Direct3DCreate9(D3D_SDK_VERSION);
	SetViewport(512, 384);
	m_device->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
	FlipImpl();

    for(unsigned int i = 0; i < MAXCACHE; i++)
    {
        m_TexCache.push_back(new CTexture());
    }

	m_pCvtBuffer = reinterpret_cast<uint8*>(malloc(CVTBUFFERSIZE));

	m_pCLUT		= malloc(0x400);
	m_pCLUT32	= reinterpret_cast<uint32*>(m_pCLUT);
	m_pCLUT16	= reinterpret_cast<uint16*>(m_pCLUT);
}

void CGSH_Direct3D9::ReleaseImpl()
{
	FREEPTR(m_pCLUT);
	FREEPTR(m_pCvtBuffer);
	TexCache_Flush();
	FREECOM(m_triangleVb);
	FREECOM(m_device);
	FREECOM(m_d3d);
}

void CGSH_Direct3D9::BeginScene()
{
	if(!m_sceneBegun)
	{
		m_device->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
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

void CGSH_Direct3D9::FlipImpl()
{
	if(m_device != NULL)
	{
		HRESULT result;

		EndScene();
		result = m_device->Present(NULL, NULL, NULL, NULL);
		assert(result == S_OK);
		BeginScene();
	}
}

void CGSH_Direct3D9::SetReadCircuitMatrix(int nWidth, int nHeight)
{
	//Setup projection matrix
	{
		D3DXMATRIX projMatrix;
		D3DXMatrixOrthoLH(&projMatrix, static_cast<FLOAT>(nWidth), static_cast<FLOAT>(nHeight), 0, 100);
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

void CGSH_Direct3D9::SetViewport(int nWidth, int nHeight)
{
	RECT rc;
	SetRect(&rc, 0, 0, nWidth, nHeight);
	AdjustWindowRect(&rc, GetWindowLong(m_pOutputWnd->m_hWnd, GWL_STYLE), FALSE);
	m_pOutputWnd->SetSize((rc.right - rc.left), (rc.bottom - rc.top));
	ReCreateDevice(nWidth, nHeight);
}

void CGSH_Direct3D9::ReCreateDevice(int nWidth, int nHeight)
{
	FREECOM(m_triangleVb);
	FREECOM(m_device);

    D3DPRESENT_PARAMETERS d3dpp;
	memset(&d3dpp, 0, sizeof(D3DPRESENT_PARAMETERS));
    d3dpp.Windowed			= TRUE;
    d3dpp.SwapEffect		= D3DSWAPEFFECT_DISCARD;
    d3dpp.hDeviceWindow		= m_pOutputWnd->m_hWnd;
    d3dpp.BackBufferFormat	= D3DFMT_X8R8G8B8;
    d3dpp.BackBufferWidth	= nWidth;
    d3dpp.BackBufferHeight	= nHeight;

    m_d3d->CreateDevice(D3DADAPTER_DEFAULT,
                      D3DDEVTYPE_HAL,
                      m_pOutputWnd->m_hWnd,
                      D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                      &d3dpp,
                      &m_device);


#ifdef _WIREFRAME
	m_device->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
#endif
	m_device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	m_device->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
    m_device->SetRenderState(D3DRS_LIGHTING, FALSE);

	m_device->CreateVertexBuffer(3 * sizeof(CUSTOMVERTEX), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, CUSTOMFVF, D3DPOOL_DEFAULT, &m_triangleVb, NULL);

	m_sceneBegun = false;
	BeginScene();
}

CGSHandler* CGSH_Direct3D9::GSHandlerFactory(Win32::CWindow* pParam)
{
	return new CGSH_Direct3D9(pParam);
}

void CGSH_Direct3D9::UpdateViewportImpl()
{
	GSDISPLAY d;

	if(m_nPMODE == 0) return;

	if(m_nPMODE & 0x01)
	{
		DECODE_DISPLAY(m_nDISPLAY1, d);
	}
	else
	{
		DECODE_DISPLAY(m_nDISPLAY2, d);
	}

	unsigned int nW = (d.nW + 1) / (d.nMagX + 1);
	unsigned int nH = (d.nH + 1);

	if(GetCrtIsInterlaced() && GetCrtIsFrameMode())
	{
		nH /= 2;
	}

	if(m_nWidth == nW && m_nHeight == nH)
	{
		return;
	}

	m_nWidth = nW;
	m_nHeight = nH;

	SetViewport(GetCrtWidth(), GetCrtHeight());
	SetReadCircuitMatrix(m_nWidth, m_nHeight);
}

void CGSH_Direct3D9::Prim_Triangle()
{
	float nU1 = 0, nU2 = 0, nU3 = 0;
	float nV1 = 0, nV2 = 0, nV3 = 0;
	float nF1 = 0, nF2 = 0, nF3 = 0;

	GSRGBAQ rgbaq[3];

	XYZ vertex[3];
	vertex[0] <<= m_VtxBuffer[2].nPosition;
	vertex[1] <<= m_VtxBuffer[1].nPosition;
	vertex[2] <<= m_VtxBuffer[0].nPosition;

	float nX1 = vertex[0].GetX(), nX2 = vertex[1].GetX(), nX3 = vertex[2].GetX();
	float nY1 = vertex[0].GetY(), nY2 = vertex[1].GetY(), nY3 = vertex[2].GetY();
	float nZ1 = vertex[0].GetZ(), nZ2 = vertex[1].GetZ(), nZ3 = vertex[2].GetZ();

	DECODE_RGBAQ(m_VtxBuffer[2].nRGBAQ, rgbaq[0]);
	DECODE_RGBAQ(m_VtxBuffer[1].nRGBAQ, rgbaq[1]);
	DECODE_RGBAQ(m_VtxBuffer[0].nRGBAQ, rgbaq[2]);

	nX1 -= m_nPrimOfsX;
	nX2 -= m_nPrimOfsX;
	nX3 -= m_nPrimOfsX;

	nY1 -= m_nPrimOfsY;
	nY2 -= m_nPrimOfsY;
	nY3 -= m_nPrimOfsY;

	//nZ1 = GetZ(nZ1);
	//nZ2 = GetZ(nZ2);
	//nZ3 = GetZ(nZ3);

	if(m_PrimitiveMode.nShading)
	{
		m_device->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);
	}
	else
	{
		m_device->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_FLAT);
	}

	if(m_PrimitiveMode.nAlpha)
	{
		m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	}

	if(m_PrimitiveMode.nFog)
	{
		//glEnable(GL_FOG);

		nF1 = (float)(0xFF - m_VtxBuffer[2].nFog) / 255.0f;
		nF2 = (float)(0xFF - m_VtxBuffer[1].nFog) / 255.0f;
		nF3 = (float)(0xFF - m_VtxBuffer[0].nFog) / 255.0f;
	}
	else
	{
		nF1 = nF2 = nF3 = 0.0;
	}

	if(m_PrimitiveMode.nTexture)
	{
		//Textured triangle

		if(m_PrimitiveMode.nUseUV)
		{
			//glBindTexture(GL_TEXTURE_2D, m_nTexHandle);

			//DECODE_UV(m_VtxBuffer[2].nUV, nU1, nV1);
			//DECODE_UV(m_VtxBuffer[1].nUV, nU2, nV2);
			//DECODE_UV(m_VtxBuffer[0].nUV, nU3, nV3);

			//nU1 /= (double)m_nTexWidth;
			//nU2 /= (double)m_nTexWidth;
			//nU3 /= (double)m_nTexWidth;

			//nV1 /= (double)m_nTexHeight;
			//nV2 /= (double)m_nTexHeight;
			//nV3 /= (double)m_nTexHeight;

			//glBegin(GL_TRIANGLES);
			//{
			//	glColor4ub(MulBy2Clamp(rgbaq[0].nR), MulBy2Clamp(rgbaq[0].nG), MulBy2Clamp(rgbaq[0].nB), MulBy2Clamp(rgbaq[0].nA));
			//	glTexCoord2d(nU1, nV1);
			//	glVertex3d(nX1, nY1, nZ1);

			//	glColor4ub(MulBy2Clamp(rgbaq[1].nR), MulBy2Clamp(rgbaq[1].nG), MulBy2Clamp(rgbaq[1].nB), MulBy2Clamp(rgbaq[1].nA));
			//	glTexCoord2d(nU2, nV2);
			//	glVertex3d(nX2, nY2, nZ2);

			//	glColor4ub(MulBy2Clamp(rgbaq[2].nR), MulBy2Clamp(rgbaq[2].nG), MulBy2Clamp(rgbaq[2].nB), MulBy2Clamp(rgbaq[2].nA));
			//	glTexCoord2d(nU3, nV3);
			//	glVertex3d(nX3, nY3, nZ3);
			//}
			//glEnd();

			//glBindTexture(GL_TEXTURE_2D, NULL);

		}
		else
		{
			m_device->SetTexture(0, m_nTexHandle);

			ST st[3];
			st[0] <<= m_VtxBuffer[2].nST;
			st[1] <<= m_VtxBuffer[1].nST;
			st[2] <<= m_VtxBuffer[0].nST;

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

		//Non Textured Triangle
		//glBegin(GL_TRIANGLES);
		//	
		//	glColor4ub(rgbaq[0].nR, rgbaq[0].nG, rgbaq[0].nB, MulBy2Clamp(rgbaq[0].nA));
		//	glVertex3d(nX1, nY1, nZ1);

		//	glColor4ub(rgbaq[1].nR, rgbaq[1].nG, rgbaq[1].nB, MulBy2Clamp(rgbaq[1].nA));
		//	glVertex3d(nX2, nY2, nZ2);

		//	glColor4ub(rgbaq[2].nR, rgbaq[2].nG, rgbaq[2].nB, MulBy2Clamp(rgbaq[2].nA));
		//	glVertex3d(nX3, nY3, nZ3);

		//glEnd();
	}

	HRESULT result;

    CUSTOMVERTEX vertices[] =
    {
        { nX1,	nY1,	0.5f, D3DCOLOR_XRGB(255, 255, 255), nU1, nV1 },
        { nX2,	nY2,	0.5f, D3DCOLOR_XRGB(255, 255, 255), nU2, nV2 },
        { nX3,	nY3,	0.5f, D3DCOLOR_XRGB(255, 255, 255), nU3, nV3 },
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

	if(m_PrimitiveMode.nFog)
	{
		//glDisable(GL_FOG);
	}

	if(m_PrimitiveMode.nAlpha)
	{
		m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	}
}

void CGSH_Direct3D9::SetRenderingContext(unsigned int nContext)
{
	SetupBlendingFunction(m_nReg[GS_REG_ALPHA_1 + nContext]);
	//SetupTestFunctions(m_nReg[GS_REG_TEST_1 + nContext]);
	//SetupDepthBuffer(m_nReg[GS_REG_ZBUF_1 + nContext]);
	SetupTexture(m_nReg[GS_REG_TEX0_1 + nContext], m_nReg[GS_REG_TEX1_1 + nContext], m_nReg[GS_REG_CLAMP_1 + nContext]);
	
	XYOFFSET offset;
	offset <<= m_nReg[GS_REG_XYOFFSET_1 + nContext];
	m_nPrimOfsX = offset.GetX();
	m_nPrimOfsY = offset.GetY();
	
	if(GetCrtIsInterlaced() && GetCrtIsFrameMode())
	{
		if(m_nCSR & 0x2000)
		{
			m_nPrimOfsY += 0.5;
		}
	}
}

void CGSH_Direct3D9::SetupBlendingFunction(uint64 nData)
{
	if(nData == 0) return;

	ALPHA alpha;
	alpha <<= nData;

	if((alpha.nA == 0) && (alpha.nB == 1) && (alpha.nC == 0) && (alpha.nD == 1))
	{
		m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	}
	else if((alpha.nA == 0) && (alpha.nB == 2) && (alpha.nC == 2) && (alpha.nD == 1) && (alpha.nFix == 0x80))
	{
		m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
		m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
	}
	//else if((alpha.nA == 0) && (alpha.nB == 1) && (alpha.nC == 2) && (alpha.nD == 1) && (alpha.nFix == 0x80))
	//{
	//	glBlendFunc(GL_ONE, GL_ZERO);
	//}
	//else if((alpha.nA == 0) && (alpha.nB == 1) && (alpha.nC == 2) && (alpha.nD == 1))
	//{
	//	//Source alpha value is implied in the formula
	//	//As = FIX / 0x80
	//	if(glBlendColorEXT != NULL)
	//	{
	//		glBlendColorEXT(0.0f, 0.0f, 0.0f, (float)alpha.nFix / 128.0f);
	//		glBlendFunc(GL_CONSTANT_ALPHA_EXT, GL_ONE_MINUS_CONSTANT_ALPHA_EXT);
	//	}
	//}
	//else if((alpha.nA == 1) && (alpha.nB == 0) && (alpha.nC == 2) && (alpha.nD == 2))
	//{
	//	nFunction = GL_FUNC_REVERSE_SUBTRACT_EXT;
	//	if(glBlendColorEXT != NULL)
	//	{
	//		glBlendColorEXT(0.0f, 0.0f, 0.0f, (float)alpha.nFix / 128.0f);
	//		glBlendFunc(GL_CONSTANT_ALPHA_EXT, GL_CONSTANT_ALPHA_EXT);
	//	}
	//}
	//else if((alpha.nA == 0) && (alpha.nB == 2) && (alpha.nC == 0) && (alpha.nD == 1))
	//{
	//	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	//}
	//else if((alpha.nA == 0) && (alpha.nB == 2) && (alpha.nC == 0) && (alpha.nD == 2))
	//{
	//	//Cs * As
	//	glBlendFunc(GL_SRC_ALPHA, GL_ZERO);
	//}
	//else if((alpha.nA == 0) && (alpha.nB == 1) && (alpha.nC == 1) && (alpha.nD == 1))
	//{
	//	//Cs * Ad + Cd * (1 - Ad)
	//	glBlendFunc(GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA);
	//}
	//else if((alpha.nA == 0) && (alpha.nB == 2) && (alpha.nC == 1) && (alpha.nD == 1))
	//{
	//	//Cs * Ad + Cd
	//	glBlendFunc(GL_DST_ALPHA, GL_ONE);
	//}
	else if((alpha.nA == 1) && (alpha.nB == 2) && (alpha.nC == 0) && (alpha.nD == 2))
	{
		//Cd * As
		m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ZERO);
		m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_SRCALPHA);

		m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ZERO);
		m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);

//		glBlendFunc(GL_ZERO, GL_ONE);
	}
	else
	{
		printf("GSH_DirectX9: Unknown color blending formula.\r\n");
	}

	//if(glBlendEquationEXT != NULL)
	//{
	//	glBlendEquationEXT(nFunction);
	//}
}

void CGSH_Direct3D9::SetupTexture(uint64 nTex0, uint64 nTex1, uint64 nClamp)
{
	TEX0 tex0;
	TEX1 tex1;
	CLAMP clamp;

	if(nTex0 == 0)
	{
		m_nTexHandle = 0;
		return;
	}

	tex0 <<= nTex0;
	tex1 <<= nTex1;
	clamp <<= nClamp;

	m_nTexWidth		= tex0.GetWidth();
	m_nTexHeight	= tex0.GetHeight();
	m_nTexHandle	= LoadTexture(&tex0, &tex1, &clamp);

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

	//if(m_nForceBilinearTextures)
	//{
	//	nMagFilter = D3DTEXF_LINEAR;
	//	nMinFilter = D3DTEXF_LINEAR;
	//}

	//switch(pClamp->nWMS)
	//{
	//case 0:
	//case 2:
	//case 3:
	//	nWrapS = GL_REPEAT;
	//	break;
	//case 1:
	//	nWrapS = GL_CLAMP_TO_EDGE;
	//	break;
	//}

	//switch(pClamp->nWMT)
	//{
	//case 0:
	//case 2:
	//case 3:
	//	nWrapT = GL_REPEAT;
	//	break;
	//case 1:
	//	nWrapT = GL_CLAMP_TO_EDGE;
	//	break;
	//}

	m_device->SetSamplerState(0, D3DSAMP_MINFILTER, nMinFilter);
	m_device->SetSamplerState(0, D3DSAMP_MAGFILTER, nMagFilter);
	m_device->SetSamplerState(0, D3DSAMP_MIPFILTER, nMipFilter);
}

void CGSH_Direct3D9::WriteRegisterImpl(uint8 nRegister, uint64 nData)
{
	CGSHandler::WriteRegisterImpl(nRegister, nData);

    switch(nRegister)
	{
	case GS_REG_PRIM:
		m_nPrimitiveType = (unsigned int)(nData & 0x07);
		switch(m_nPrimitiveType)
		{
		case 0:
			//Point
			m_nVtxCount = 1;
			break;
		case 1:
			//Line
			m_nVtxCount = 2;
			break;
		case 2:
			//Line strip
			m_nVtxCount = 2;
			break;
		case 3:
			//Triangle
			m_nVtxCount = 3;
			break;
		case 4:
			//Triangle Strip
			m_nVtxCount = 3;
			break;
		case 5:
			//Triangle Fan
			m_nVtxCount = 3;
			break;
		case 6:
			//Sprite (rectangle)
			m_nVtxCount = 2;
			break;
		default:
			printf("GS: Unhandled primitive type (%i) encountered.\r\n", m_nPrimitiveType);
			break;
		}
		break;

	case GS_REG_XYZ2:
	case GS_REG_XYZ3:
	case GS_REG_XYZF2:
	case GS_REG_XYZF3:
		VertexKick(nRegister, nData);
		break;

	//case GS_REG_TEX2_1:
	//case GS_REG_TEX2_2:
	//	{
	//		unsigned int nContext;
	//		const uint64 nMask = 0xFFFFFFE003F00000ULL;
	//		GSTEX0 Tex0;

	//		nContext = nRegister - GS_REG_TEX2_1;

	//		Tex0 = *(GSTEX0*)&m_nReg[GS_REG_TEX0_1 + nContext];
	//		if(Tex0.nCLD == 1 && Tex0.nCPSM == 0)
	//		{
	//			ReadCLUT8(&Tex0);
	//		}

	//		m_nReg[GS_REG_TEX0_1 + nContext] &= ~nMask;
	//		m_nReg[GS_REG_TEX0_1 + nContext] |= nData & nMask;
	//	}
	//	break;

	//case GS_REG_FOGCOL:
	//	SetupFogColor();
	//	break;
	}
}

void CGSH_Direct3D9::VertexKick(uint8 nRegister, uint64 nValue)
{
	if(m_nVtxCount == 0) return;

	bool nDrawingKick = (nRegister == GS_REG_XYZ2) || (nRegister == GS_REG_XYZF2);
	bool nFog = (nRegister == GS_REG_XYZF2) || (nRegister == GS_REG_XYZF3);

	if(nFog)
	{
		m_VtxBuffer[m_nVtxCount - 1].nPosition	= nValue & 0x00FFFFFFFFFFFFFFULL;
		m_VtxBuffer[m_nVtxCount - 1].nRGBAQ		= m_nReg[GS_REG_RGBAQ];
		m_VtxBuffer[m_nVtxCount - 1].nUV		= m_nReg[GS_REG_UV];
		m_VtxBuffer[m_nVtxCount - 1].nST		= m_nReg[GS_REG_ST];
		m_VtxBuffer[m_nVtxCount - 1].nFog		= (uint8)(nValue >> 56);
	}
	else
	{
		m_VtxBuffer[m_nVtxCount - 1].nPosition	= nValue;
		m_VtxBuffer[m_nVtxCount - 1].nRGBAQ		= m_nReg[GS_REG_RGBAQ];
		m_VtxBuffer[m_nVtxCount - 1].nUV		= m_nReg[GS_REG_UV];
		m_VtxBuffer[m_nVtxCount - 1].nST		= m_nReg[GS_REG_ST];
		m_VtxBuffer[m_nVtxCount - 1].nFog		= (uint8)(m_nReg[GS_REG_FOG] >> 56);
	}

	m_nVtxCount--;

	if(m_nVtxCount == 0)
	{
		{
			if((m_nReg[GS_REG_PRMODECONT] & 1) != 0)
			{
				m_PrimitiveMode <<= m_nReg[GS_REG_PRIM];
			}
			else
			{
				m_PrimitiveMode <<= m_nReg[GS_REG_PRMODE];
			}

			SetRenderingContext(m_PrimitiveMode.nContext);

			switch(m_nPrimitiveType)
			{
			case 0:
				//if(nDrawingKick) Prim_Point();
				break;
			case 1:
				//if(nDrawingKick) Prim_Line();
				break;
			case 2:
				//if(nDrawingKick) Prim_Line();
				memcpy(&m_VtxBuffer[1], &m_VtxBuffer[0], sizeof(VERTEX));
				m_nVtxCount = 1;
				break;
			case 3:
				if(nDrawingKick) Prim_Triangle();
				m_nVtxCount = 3;
				break;
			case 4:
				if(nDrawingKick) Prim_Triangle();
				memcpy(&m_VtxBuffer[2], &m_VtxBuffer[1], sizeof(VERTEX));
				memcpy(&m_VtxBuffer[1], &m_VtxBuffer[0], sizeof(VERTEX));
				m_nVtxCount = 1;
				break;
			case 5:
				if(nDrawingKick) Prim_Triangle();
				memcpy(&m_VtxBuffer[1], &m_VtxBuffer[0], sizeof(VERTEX));
				m_nVtxCount = 1;
				break;
			case 6:
				//if(nDrawingKick) Prim_Sprite();
				m_nVtxCount = 2;
				break;
			}
		}
	}
}
