#include <stdio.h>
#include <assert.h>
#include <math.h>
#include "AppConfig.h"
#include "GSH_OpenGL.h"
#include "PtrMacro.h"
#include "Log.h"

using namespace Framework;
using namespace std::tr1;

//#define _WIREFRAME

CGSH_OpenGL::CGSH_OpenGL() 
: m_pProgram(NULL)
, m_pVertShader(NULL)
, m_pFragShader(NULL)
, m_pCLUT(NULL)
, m_pCLUT16(NULL)
, m_pCLUT32(NULL)
, m_pCvtBuffer(NULL)
//, m_currentPixelBuffer(0)
{
	CAppConfig::GetInstance().RegisterPreferenceBoolean(PREF_CGSH_OPENGL_LINEASQUADS, false);
	CAppConfig::GetInstance().RegisterPreferenceBoolean(PREF_CGSH_OPENGL_FORCEBILINEARTEXTURES, false);
    CAppConfig::GetInstance().RegisterPreferenceBoolean(PREF_CGSH_OPENGL_FIXSMALLZVALUES, false);

	LoadSettings();
}

CGSH_OpenGL::~CGSH_OpenGL()
{
    FREEPTR(m_pCLUT);
    FREEPTR(m_pCvtBuffer);
}

void CGSH_OpenGL::InitializeImpl()
{
	InitializeRC();

	m_nVtxCount = 0;
	m_nWidth = -1;
	m_nHeight = -1;

    //
	//m_nTexCacheIndex = 0;
    for(unsigned int i = 0; i < MAXCACHE; i++)
    {
        m_TexCache.push_back(new CTexture());
    }

	m_nMaxZ = 32768.0;
}

void CGSH_OpenGL::ReleaseImpl()
{
    TexCache_Flush();
    for(TextureList::iterator textureIterator(m_TexCache.begin());
        textureIterator != m_TexCache.end(); textureIterator++)
    {
        CTexture* texture = *textureIterator;
        delete texture;
    }
//    glDeleteBuffers(MAX_PIXEL_BUFFERS, m_pixelBuffers);
}

void CGSH_OpenGL::FlipImpl()
{
	PresentBackbuffer();
	CGSHandler::FlipImpl();
#ifdef _WIREFRAME
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif
}

void CGSH_OpenGL::LoadState(CZipArchiveReader& archive)
{
	CGSHandler::LoadState(archive);

	m_mailBox.SendCall(bind(&CGSH_OpenGL::TexCache_InvalidateTextures, this, 0, RAMSIZE));
}

void CGSH_OpenGL::LoadSettings()
{
	CGSHandler::LoadSettings();

	m_nLinesAsQuads				= CAppConfig::GetInstance().GetPreferenceBoolean(PREF_CGSH_OPENGL_LINEASQUADS);
	m_nForceBilinearTextures	= CAppConfig::GetInstance().GetPreferenceBoolean(PREF_CGSH_OPENGL_FORCEBILINEARTEXTURES);
    m_fixSmallZValues           = CAppConfig::GetInstance().GetPreferenceBoolean(PREF_CGSH_OPENGL_FIXSMALLZVALUES);
}

void CGSH_OpenGL::InitializeRC()
{
#ifdef WIN32
	glewInit();
#endif

	//Initialize basic stuff
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(0.0f);
	glEnable(GL_TEXTURE_2D);

	//Initialize fog
	glFogi(GL_FOG_MODE, GL_LINEAR);
	glFogf(GL_FOG_START, 0.0f);
	glFogf(GL_FOG_END, 1.0f);
	glHint(GL_FOG_HINT, GL_NICEST);
	glFogi(GL_FOG_COORDINATE_SOURCE_EXT, GL_FOG_COORDINATE_EXT);

//REMOVE
//	if(glColorTableEXT == NULL)
//	{
		m_pTexUploader_Psm8 = &CGSH_OpenGL::TexUploader_Psm8_Cvt;
//	}
//	else
//	{
//		m_pTexUploader_Psm8 = &CGSH_OpenGL::TexUploader_Psm8_Hw;
//	}

	VerifyRGBA5551Support();
	if(!m_nIsRGBA5551Supported)
	{
		m_pTexUploader_Psm16 = &CGSH_OpenGL::TexUploader_Psm16_Cvt;
	}
	else
	{
		m_pTexUploader_Psm16 = &CGSH_OpenGL::TexUploader_Psm16_Hw;
	}

    DELETEPTR(m_pProgram);
    DELETEPTR(m_pVertShader);
    DELETEPTR(m_pFragShader);

	//Create shaders/program
	if((glCreateProgram != NULL) && (glCreateShader != NULL))
	{
        m_pProgram		= new OpenGl::CProgram();
		m_pVertShader	= new OpenGl::CShader(GL_VERTEX_SHADER);
		m_pFragShader	= new OpenGl::CShader(GL_FRAGMENT_SHADER);

		LoadShaderSource(m_pVertShader, SHADER_VERTEX);
		LoadShaderSource(m_pFragShader, SHADER_FRAGMENT);

		m_pVertShader->Compile();
		m_pFragShader->Compile();

		m_pProgram->AttachShader((*m_pVertShader));
		m_pProgram->AttachShader((*m_pFragShader));

		m_pProgram->Link();
	}

	m_pCvtBuffer = (uint8*)malloc(CVTBUFFERSIZE);

	m_pCLUT		= malloc(0x400);
	m_pCLUT32	= (uint32*)m_pCLUT;
	m_pCLUT16	= (uint16*)m_pCLUT;
	m_nCBP0		= 0;
	m_nCBP1		= 0;

	SetViewport(512, 384);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

//    glGenBuffers(MAX_PIXEL_BUFFERS, m_pixelBuffers);
//    for(unsigned int i = 0; i < MAX_PIXEL_BUFFERS; i++)
//    {
//        glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, m_pixelBuffers[i]);
//        glBufferData(GL_PIXEL_UNPACK_BUFFER_ARB, PIXEL_BUFFER_SIZE, 0, GL_DYNAMIC_DRAW_ARB);
//    }
//    glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);

#ifdef _WIREFRAME
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDisable(GL_TEXTURE_2D);
#endif

    PresentBackbuffer();
}

void CGSH_OpenGL::VerifyRGBA5551Support()
{
	unsigned int nTexture;

	m_nIsRGBA5551Supported = true;

	glGenTextures(1, &nTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB5_A1, 32, 32, 0, GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV, m_pRAM);
	if(glGetError() == GL_INVALID_ENUM)
	{
		m_nIsRGBA5551Supported = false;
	}
	glDeleteTextures(1, &nTexture);
}

void CGSH_OpenGL::LinearZOrtho(float nLeft, float nRight, float nBottom, float nTop)
{
	float nMatrix[16];

	nMatrix[ 0] = 2.0f / (nRight - nLeft);
	nMatrix[ 1] = 0;
	nMatrix[ 2] = 0;
	nMatrix[ 3] = 0;

	nMatrix[ 4] = 0;
	nMatrix[ 5] = 2.0f / (nTop - nBottom);
	nMatrix[ 6] = 0;
	nMatrix[ 7] = 0;

	nMatrix[ 8] = 0;
	nMatrix[ 9] = 0;
	nMatrix[10] = 1;
	nMatrix[11] = 0;

	nMatrix[12] = - (nRight + nLeft) / (nRight - nLeft);
	nMatrix[13] = - (nTop + nBottom) / (nTop - nBottom);
	nMatrix[14] = 0;
	nMatrix[15] = 1;

	glMultMatrixf(nMatrix);
}

void CGSH_OpenGL::UpdateViewportImpl()
{
	if(m_nPMODE == 0) return;

	DISPLAY d;
	if(m_nPMODE & 0x01)
	{
		d <<= m_nDISPLAY1;
	}
	else
	{
		d <<= m_nDISPLAY2;
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

	//wglMakeCurrent(m_hDC, m_hRC);
	{
		SetViewport(GetCrtWidth(), GetCrtHeight());
		SetReadCircuitMatrix(m_nWidth, m_nHeight);
	}
	//wglMakeCurrent(NULL, NULL);
}

unsigned int CGSH_OpenGL::GetCurrentReadCircuit()
{
	assert((m_nPMODE & 0x3) != 0x03);
	if(m_nPMODE & 0x1) return 0;
	if(m_nPMODE & 0x2) return 1;
	//Getting here is bad
	return 0;
}

void CGSH_OpenGL::SetViewport(int nWidth, int nHeight)
{
	glViewport(0, 0, nWidth, nHeight);
}

void CGSH_OpenGL::SetReadCircuitMatrix(int nWidth, int nHeight)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	LinearZOrtho(0, static_cast<float>(nWidth), static_cast<float>(nHeight), 0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

uint32 CGSH_OpenGL::RGBA16ToRGBA32(uint16 nColor)
{
	return (nColor & 0x8000 ? 0xFF000000 : 0) | ((nColor & 0x7C00) << 9) | ((nColor & 0x03E0) << 6) | ((nColor & 0x001F) << 3);
}

uint8 CGSH_OpenGL::MulBy2Clamp(uint8 nValue)
{
	return (nValue > 0x7F) ? 0xFF : (nValue << 1);
}

float CGSH_OpenGL::GetZ(float nZ)
{
	if(nZ == 0)
	{
		return -1;
	}
	
	if(m_fixSmallZValues && (nZ < 256))
	{
		//The number is small, so scale to a smaller ratio (65536)
		return (nZ - 32768.0f) / 32768.0f;
	}
	else
	{
		nZ -= m_nMaxZ;
		if(nZ > m_nMaxZ) return 1.0;
		if(nZ < -m_nMaxZ) return -1.0;
		return nZ / m_nMaxZ;
	}
}

unsigned int CGSH_OpenGL::GetNextPowerOf2(unsigned int nNumber)
{
	return 1 << ((int)(log((double)(nNumber - 1)) / log(2.0)) + 1);
}

/////////////////////////////////////////////////////////////
// Context Unpacking
/////////////////////////////////////////////////////////////

void CGSH_OpenGL::SetRenderingContext(unsigned int nContext)
{
	SetupBlendingFunction(m_nReg[GS_REG_ALPHA_1 + nContext]);
	SetupTestFunctions(m_nReg[GS_REG_TEST_1 + nContext]);
	SetupDepthBuffer(m_nReg[GS_REG_ZBUF_1 + nContext]);
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

void CGSH_OpenGL::SetupBlendingFunction(uint64 nData)
{
	if(nData == 0) return;

	int nFunction = GL_FUNC_ADD_EXT;

	ALPHA alpha;
	alpha <<= nData;

	if((alpha.nA == 0) && (alpha.nB == 1) && (alpha.nC == 0) && (alpha.nD == 1))
	{
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	else if((alpha.nA == 0) && (alpha.nB == 2) && (alpha.nC == 2) && (alpha.nD == 1) && (alpha.nFix == 0x80))
	{
		glBlendFunc(GL_ONE, GL_ONE);
	}
	else if((alpha.nA == 0) && (alpha.nB == 1) && (alpha.nC == 2) && (alpha.nD == 1) && (alpha.nFix == 0x80))
	{
		glBlendFunc(GL_ONE, GL_ZERO);
	}
	else if((alpha.nA == 0) && (alpha.nB == 1) && (alpha.nC == 2) && (alpha.nD == 1))
	{
		//Source alpha value is implied in the formula
		//As = FIX / 0x80
		if(glBlendColorEXT != NULL)
		{
			glBlendColorEXT(0.0f, 0.0f, 0.0f, (float)alpha.nFix / 128.0f);
			glBlendFunc(GL_CONSTANT_ALPHA_EXT, GL_ONE_MINUS_CONSTANT_ALPHA_EXT);
		}
	}
	else if((alpha.nA == 1) && (alpha.nB == 0) && (alpha.nC == 2) && (alpha.nD == 2))
	{
		nFunction = GL_FUNC_REVERSE_SUBTRACT_EXT;
		if(glBlendColorEXT != NULL)
		{
			glBlendColorEXT(0.0f, 0.0f, 0.0f, (float)alpha.nFix / 128.0f);
			glBlendFunc(GL_CONSTANT_ALPHA_EXT, GL_CONSTANT_ALPHA_EXT);
		}
	}
	else if((alpha.nA == 0) && (alpha.nB == 2) && (alpha.nC == 0) && (alpha.nD == 1))
	{
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	}
	else if((alpha.nA == 0) && (alpha.nB == 2) && (alpha.nC == 0) && (alpha.nD == 2))
	{
		//Cs * As
		glBlendFunc(GL_SRC_ALPHA, GL_ZERO);
	}
	else if((alpha.nA == 0) && (alpha.nB == 1) && (alpha.nC == 1) && (alpha.nD == 1))
	{
		//Cs * Ad + Cd * (1 - Ad)
		glBlendFunc(GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA);
	}
	else if((alpha.nA == 0) && (alpha.nB == 2) && (alpha.nC == 1) && (alpha.nD == 1))
	{
		//Cs * Ad + Cd
		glBlendFunc(GL_DST_ALPHA, GL_ONE);
	}
	else if((alpha.nA == 1) && (alpha.nB == 2) && (alpha.nC == 0) && (alpha.nD == 2))
	{
		//Cd * As
		//glBlendFunc(GL_ZERO, GL_SRC_ALPHA);
		//REMOVE
		glBlendFunc(GL_ZERO, GL_ONE);
		//REMOVE
	}
	else
	{
		printf("GSH_OpenGL: Unknown color blending formula.\r\n");
	}

	if(glBlendEquationEXT != NULL)
	{
		glBlendEquationEXT(nFunction);
	}
}

void CGSH_OpenGL::SetupTestFunctions(uint64 nData)
{
	TEST tst;
    tst <<= nData;

	if(tst.nAlphaEnabled)
	{
	    unsigned int nFunc = GL_NEVER;
		switch(tst.nAlphaMethod)
		{
		case 0:
			nFunc = GL_NEVER;
			break;
		case 1:
			nFunc = GL_ALWAYS;
			break;
		case 2:
			nFunc = GL_LESS;
			break;
        case 5:
            nFunc = GL_GEQUAL;
            break;
		case 6:
			nFunc = GL_GREATER;
			break;
		case 7:
			nFunc = GL_NOTEQUAL;
			break;
		default:
			assert(0);
			break;
		}

		float nValue = (float)tst.nAlphaRef / 255.0f;
		glAlphaFunc(nFunc, nValue);

		glEnable(GL_ALPHA_TEST);

	}
	else
	{
		glDisable(GL_ALPHA_TEST);
	}

	if(tst.nDepthEnabled)
	{
	    unsigned int nFunc = GL_NEVER;

		switch(tst.nDepthMethod)
		{
		case 0:
			nFunc = GL_NEVER;
			break;
		case 1:
			nFunc = GL_ALWAYS;
			break;
		case 2:
			nFunc = GL_GEQUAL;
			break;
		case 3:
			nFunc = GL_GREATER;
			break;
		}

		glDepthFunc(nFunc);

		glEnable(GL_DEPTH_TEST);
	}
	else
	{
		glDisable(GL_DEPTH_TEST);
	}
}

void CGSH_OpenGL::SetupDepthBuffer(uint64 nData)
{
	ZBUF zbuf;
	zbuf <<= nData;

	switch(GetPsmPixelSize(zbuf.nPsm))
	{
	case 16:
		m_nMaxZ = 32768.0f;
		break;
	default:
	case 32:
		m_nMaxZ = 2147483647.0f;
		break;
	}

	glDepthMask(zbuf.nMask ? GL_FALSE : GL_TRUE);
}

void CGSH_OpenGL::SetupTexture(uint64 nTex0, uint64 nTex1, uint64 nClamp)
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
}

void CGSH_OpenGL::SetupFogColor()
{
	float nColor[4];

	//wglMakeCurrent(m_hDC, m_hRC);

	FOGCOL* pColor = GetFogCol();
	nColor[0] = (float)pColor->nFCR / 255.0f;
	nColor[1] = (float)pColor->nFCG / 255.0f;
	nColor[2] = (float)pColor->nFCB / 255.0f;
	nColor[3] = 0.0f;

	glFogfv(GL_FOG_COLOR, nColor);
}

/////////////////////////////////////////////////////////////
// Individual Primitives Implementations
/////////////////////////////////////////////////////////////

void CGSH_OpenGL::Prim_Point()
{
	XYZ xyz;
	xyz <<= m_VtxBuffer[0].nPosition;

	float nX = xyz.GetX();	float nY = xyz.GetY();	float nZ = xyz.GetZ();

	nX -= m_nPrimOfsX;
	nY -= m_nPrimOfsY;

	if(!m_PrimitiveMode.nUseUV && !m_PrimitiveMode.nTexture)
	{
		RGBAQ rgbaq;
		rgbaq <<= m_VtxBuffer[0].nRGBAQ;

		glBegin(GL_QUADS);

			glColor4ub(rgbaq.nR, rgbaq.nG, rgbaq.nB, rgbaq.nA);

			glVertex2f(nX + 0, nY + 0);
			glVertex2f(nX + 1, nY + 0);
			glVertex2f(nX + 1, nY + 1);
			glVertex2f(nX + 0, nY + 1);

		glEnd();

	}
	else
	{
		//Yay for textured points!
		assert(0);
	}
}

void CGSH_OpenGL::Prim_Line()
{
	XYZ xyz[2];
	xyz[0] <<= m_VtxBuffer[1].nPosition;
	xyz[1] <<= m_VtxBuffer[0].nPosition;

	float nX1 = xyz[0].GetX();	float nY1 = xyz[0].GetY();	float nZ1 = xyz[0].GetZ();
	float nX2 = xyz[1].GetX();	float nY2 = xyz[1].GetY();	float nZ2 = xyz[1].GetZ();

	nX1 -= m_nPrimOfsX;
	nX2 -= m_nPrimOfsX;

	nY1 -= m_nPrimOfsY;
	nY2 -= m_nPrimOfsY;

	if(m_PrimitiveMode.nAntiAliasing)
	{
		glEnable(GL_BLEND);
	}

	if(!m_PrimitiveMode.nUseUV && !m_PrimitiveMode.nTexture)
	{
		RGBAQ rgbaq[2];
		rgbaq[0] <<= m_VtxBuffer[1].nRGBAQ;
		rgbaq[1] <<= m_VtxBuffer[0].nRGBAQ;

		if(m_nLinesAsQuads)
		{
			glBegin(GL_QUADS);

				glColor4ub(rgbaq[0].nR, rgbaq[0].nG, rgbaq[0].nB, rgbaq[0].nA);
		
				glVertex2f(nX1 + 0, nY1 + 0);
				glVertex2f(nX1 + 1, nY1 + 1);

				glColor4ub(rgbaq[1].nR, rgbaq[1].nG, rgbaq[1].nB, rgbaq[1].nA);

				glVertex2f(nX2 + 1, nY2 + 1);
				glVertex2f(nX2 + 0, nY2 + 0);

			glEnd();
		}
		else
		{
			glBegin(GL_LINES);

				glColor4ub(rgbaq[0].nR, rgbaq[0].nG, rgbaq[0].nB, rgbaq[0].nA);
				glVertex2f(nX1, nY1);

				glColor4ub(rgbaq[1].nR, rgbaq[1].nG, rgbaq[1].nB, rgbaq[1].nA);
				glVertex2f(nX2, nY2);

			glEnd();
		}
	}
	else
	{

		//Yay for textured lines!
		assert(0);
	}

	if(m_PrimitiveMode.nAntiAliasing)
	{
		glDisable(GL_BLEND);
	}
}

void CGSH_OpenGL::Prim_Triangle()
{
	float nF1, nF2, nF3;

	RGBAQ rgbaq[3];

	XYZ vertex[3];
	vertex[0] <<= m_VtxBuffer[2].nPosition;
	vertex[1] <<= m_VtxBuffer[1].nPosition;
	vertex[2] <<= m_VtxBuffer[0].nPosition;

	float nX1 = vertex[0].GetX(), nX2 = vertex[1].GetX(), nX3 = vertex[2].GetX();
	float nY1 = vertex[0].GetY(), nY2 = vertex[1].GetY(), nY3 = vertex[2].GetY();
	float nZ1 = vertex[0].GetZ(), nZ2 = vertex[1].GetZ(), nZ3 = vertex[2].GetZ();

	rgbaq[0] <<= m_VtxBuffer[2].nRGBAQ;
	rgbaq[1] <<= m_VtxBuffer[1].nRGBAQ;
	rgbaq[2] <<= m_VtxBuffer[0].nRGBAQ;

	nX1 -= m_nPrimOfsX;
	nX2 -= m_nPrimOfsX;
	nX3 -= m_nPrimOfsX;

	nY1 -= m_nPrimOfsY;
	nY2 -= m_nPrimOfsY;
	nY3 -= m_nPrimOfsY;

	nZ1 = GetZ(nZ1);
	nZ2 = GetZ(nZ2);
	nZ3 = GetZ(nZ3);

	if(m_PrimitiveMode.nShading)
	{
		glShadeModel(GL_SMOOTH);
	}
	else
	{
		glShadeModel(GL_FLAT);
	}

	if(m_PrimitiveMode.nAlpha)
	{
		glEnable(GL_BLEND);
	}

	if(m_PrimitiveMode.nFog)
	{
		glEnable(GL_FOG);

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
			glBindTexture(GL_TEXTURE_2D, m_nTexHandle);

			UV uv[3];
			uv[0] <<= m_VtxBuffer[2].nUV;
			uv[1] <<= m_VtxBuffer[1].nUV;
			uv[2] <<= m_VtxBuffer[0].nUV;

			float nU1 = uv[0].GetU() / static_cast<float>(m_nTexWidth);
			float nU2 = uv[1].GetU() / static_cast<float>(m_nTexWidth);
			float nU3 = uv[2].GetU() / static_cast<float>(m_nTexWidth);

			float nV1 = uv[0].GetV() / static_cast<float>(m_nTexHeight);
			float nV2 = uv[1].GetV() / static_cast<float>(m_nTexHeight);
			float nV3 = uv[2].GetV() / static_cast<float>(m_nTexHeight);

			glBegin(GL_TRIANGLES);
			{
				glColor4ub(MulBy2Clamp(rgbaq[0].nR), MulBy2Clamp(rgbaq[0].nG), MulBy2Clamp(rgbaq[0].nB), MulBy2Clamp(rgbaq[0].nA));
				glTexCoord2f(nU1, nV1);
				glVertex3f(nX1, nY1, nZ1);

				glColor4ub(MulBy2Clamp(rgbaq[1].nR), MulBy2Clamp(rgbaq[1].nG), MulBy2Clamp(rgbaq[1].nB), MulBy2Clamp(rgbaq[1].nA));
				glTexCoord2f(nU2, nV2);
				glVertex3f(nX2, nY2, nZ2);

				glColor4ub(MulBy2Clamp(rgbaq[2].nR), MulBy2Clamp(rgbaq[2].nG), MulBy2Clamp(rgbaq[2].nB), MulBy2Clamp(rgbaq[2].nA));
				glTexCoord2f(nU3, nV3);
				glVertex3f(nX3, nY3, nZ3);
			}
			glEnd();

			glBindTexture(GL_TEXTURE_2D, NULL);

		}
		else
		{
			glBindTexture(GL_TEXTURE_2D, m_nTexHandle);

			ST st[3];
			st[0] <<= m_VtxBuffer[2].nST;
			st[1] <<= m_VtxBuffer[1].nST;
			st[2] <<= m_VtxBuffer[0].nST;

			float nS1 = st[0].nS, nS2 = st[1].nS, nS3 = st[2].nS;
			float nT1 = st[0].nT, nT2 = st[1].nT, nT3 = st[2].nT;

			bool isQ0Neg = (rgbaq[0].nQ < 0);
			bool isQ1Neg = (rgbaq[1].nQ < 0);
			bool isQ2Neg = (rgbaq[2].nQ < 0);

			assert(isQ0Neg == isQ1Neg);
			assert(isQ1Neg == isQ2Neg);

			nS1 /= rgbaq[0].nQ;
			nS2 /= rgbaq[1].nQ;
			nS3 /= rgbaq[2].nQ;

			nT1 /= rgbaq[0].nQ;
			nT2 /= rgbaq[1].nQ;
			nT3 /= rgbaq[2].nQ;

			glBegin(GL_TRIANGLES);
			{
				glColor4ub(MulBy2Clamp(rgbaq[0].nR), MulBy2Clamp(rgbaq[0].nG), MulBy2Clamp(rgbaq[0].nB), MulBy2Clamp(rgbaq[0].nA));
                glTexCoord2f(nS1, nT1);
				//glTexCoord4d(nS1, nT1, 0, rgbaq[0].nQ);
				if(glFogCoordfEXT) glFogCoordfEXT(nF1);
				glVertex3f(nX1, nY1, nZ1);

				glColor4ub(MulBy2Clamp(rgbaq[1].nR), MulBy2Clamp(rgbaq[1].nG), MulBy2Clamp(rgbaq[1].nB), MulBy2Clamp(rgbaq[1].nA));
                glTexCoord2f(nS2, nT2);
//				glTexCoord4d(nS2, nT2, 0, rgbaq[1].nQ);
				if(glFogCoordfEXT) glFogCoordfEXT(nF2);
				glVertex3f(nX2, nY2, nZ2);

				glColor4ub(MulBy2Clamp(rgbaq[2].nR), MulBy2Clamp(rgbaq[2].nG), MulBy2Clamp(rgbaq[2].nB), MulBy2Clamp(rgbaq[2].nA));
                glTexCoord2f(nS3, nT3);
//				glTexCoord4d(nS3, nT3, 0, rgbaq[2].nQ);
				if(glFogCoordfEXT) glFogCoordfEXT(nF3);
				glVertex3f(nX3, nY3, nZ3);
			}
			glEnd();

			glBindTexture(GL_TEXTURE_2D, NULL);			
		}
	}
	else
	{
		//Non Textured Triangle
		glBegin(GL_TRIANGLES);
			
			glColor4ub(rgbaq[0].nR, rgbaq[0].nG, rgbaq[0].nB, MulBy2Clamp(rgbaq[0].nA));
			glVertex3f(nX1, nY1, nZ1);

			glColor4ub(rgbaq[1].nR, rgbaq[1].nG, rgbaq[1].nB, MulBy2Clamp(rgbaq[1].nA));
			glVertex3f(nX2, nY2, nZ2);

			glColor4ub(rgbaq[2].nR, rgbaq[2].nG, rgbaq[2].nB, MulBy2Clamp(rgbaq[2].nA));
			glVertex3f(nX3, nY3, nZ3);

		glEnd();
	}

	if(m_PrimitiveMode.nFog)
	{
		glDisable(GL_FOG);
	}

	if(m_PrimitiveMode.nAlpha)
	{
		glDisable(GL_BLEND);
	}
}

void CGSH_OpenGL::Prim_Sprite()
{
	RGBAQ rgbaq[2];

	XYZ xyz[2];
	xyz[0] <<= m_VtxBuffer[1].nPosition;
	xyz[1] <<= m_VtxBuffer[0].nPosition;

	float nX1 = xyz[0].GetX();	float nY1 = xyz[0].GetY();	float nZ1 = xyz[0].GetZ();
	float nX2 = xyz[1].GetX();	float nY2 = xyz[1].GetY();	float nZ2 = xyz[1].GetZ();

	rgbaq[0] <<= m_VtxBuffer[1].nRGBAQ;
	rgbaq[1] <<= m_VtxBuffer[0].nRGBAQ;

	nX1 -= m_nPrimOfsX;
	nX2 -= m_nPrimOfsX;

	nY1 -= m_nPrimOfsY;
	nY2 -= m_nPrimOfsY;

//	assert(nZ1 == nZ2);

	nZ1 = GetZ(nZ1);
	nZ2 = GetZ(nZ2);

    if(m_PrimitiveMode.nAlpha)
	{
        glEnable(GL_BLEND);
	}

	if(m_PrimitiveMode.nTexture)
	{
		float nS[2], nT[2];

		glBindTexture(GL_TEXTURE_2D, m_nTexHandle);

		glColor4ub(MulBy2Clamp(rgbaq[0].nR), MulBy2Clamp(rgbaq[0].nG), MulBy2Clamp(rgbaq[0].nB), MulBy2Clamp(rgbaq[0].nA));

		if(m_PrimitiveMode.nUseUV)
		{
			UV uv[2];
			uv[0] <<= m_VtxBuffer[1].nUV;
			uv[1] <<= m_VtxBuffer[0].nUV;

			nS[0] = uv[0].GetU() / static_cast<float>(m_nTexWidth);
			nS[1] = uv[1].GetU() / static_cast<float>(m_nTexWidth);

			nT[0] = uv[0].GetV() / static_cast<float>(m_nTexHeight);
			nT[1] = uv[1].GetV() / static_cast<float>(m_nTexHeight);
		}
		else
		{
			ST st[2];

			st[0] <<= m_VtxBuffer[1].nST;
			st[1] <<= m_VtxBuffer[0].nST;

			float nQ1 = rgbaq[1].nQ;
			float nQ2 = rgbaq[0].nQ;
			if(nQ1 == 0) nQ1 = 1;
			if(nQ2 == 0) nQ2 = 1;

			nS[0] = st[0].nS / nQ1;
			nS[1] = st[1].nS / nQ2;

			nT[0] = st[0].nT / nQ1;
			nT[1] = st[1].nT / nQ2;
		}

		glBegin(GL_QUADS);
		{
			//REMOVE
            //glColor4d(1.0, 1.0, 1.0, 1.0);

			glTexCoord2f(nS[0], nT[0]);
			glVertex3f(nX1, nY1, nZ1);

			glTexCoord2f(nS[1], nT[0]);
			glVertex3f(nX2, nY1, nZ2);

			//REMOVE
			//glColor4d(1.0, 1.0, 1.0, 1.0);

			glTexCoord2f(nS[1], nT[1]);
			glVertex3f(nX2, nY2, nZ1);

			glTexCoord2f(nS[0], nT[1]);
			glVertex3f(nX1, nY2, nZ2);

		}
		glEnd();

		glBindTexture(GL_TEXTURE_2D, 0);
	}
	else if(!m_PrimitiveMode.nTexture)
	{
		//REMOVE
		//Humm? Would it be possible to have a gradient using those registers?
		glColor4ub(MulBy2Clamp(rgbaq[0].nR), MulBy2Clamp(rgbaq[0].nG), MulBy2Clamp(rgbaq[0].nB), MulBy2Clamp(rgbaq[0].nA));
		//glColor4ub(rgbaq[0].nR, rgbaq[0].nG, rgbaq[0].nB, rgbaq[0].nA);

		glBegin(GL_QUADS);

			glVertex3f(nX1, nY1, nZ1);
			glVertex3f(nX2, nY1, nZ2);
			glVertex3f(nX2, nY2, nZ1);
			glVertex3f(nX1, nY2, nZ2);

		glEnd();
	}
	else
	{
		assert(0);
	}

	if(m_PrimitiveMode.nAlpha)
	{
		glDisable(GL_BLEND);
	}
}

/////////////////////////////////////////////////////////////
// Other Functions
/////////////////////////////////////////////////////////////

void CGSH_OpenGL::WriteRegisterImpl(uint8 nRegister, uint64 nData)
{
	CGSHandler::WriteRegisterImpl(nRegister, nData);

    if(!m_enabled) return;

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

	case GS_REG_TEX0_1:
	case GS_REG_TEX0_2:
		{
			unsigned int nContext = nRegister - GS_REG_TEX0_1;
			assert(nContext == 0 || nContext == 1);
			TEX0 tex0;
			tex0 <<= m_nReg[GS_REG_TEX0_1 + nContext];
			SyncCLUT(&tex0);
		}
		break;

	case GS_REG_TEX2_1:
	case GS_REG_TEX2_2:
		{
			//Update TEX0
			unsigned int nContext = nRegister - GS_REG_TEX2_1;
			assert(nContext == 0 || nContext == 1);

			const uint64 nMask = 0xFFFFFFE003F00000ULL;
			m_nReg[GS_REG_TEX0_1 + nContext] &= ~nMask;
			m_nReg[GS_REG_TEX0_1 + nContext] |= nData & nMask;

			TEX0 tex0;
			tex0 <<= m_nReg[GS_REG_TEX0_1 + nContext];
			SyncCLUT(&tex0);
		}
		break;

	case GS_REG_FOGCOL:
		SetupFogColor();
		break;
	}
}

void CGSH_OpenGL::VertexKick(uint8 nRegister, uint64 nValue)
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
				if(nDrawingKick) Prim_Point();
				break;
			case 1:
				if(nDrawingKick) Prim_Line();
				break;
			case 2:
				if(nDrawingKick) Prim_Line();
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
				if(nDrawingKick) Prim_Sprite();
				m_nVtxCount = 2;
				break;
			}
		}
	}
}

void CGSH_OpenGL::ProcessImageTransfer(uint32 nAddress, uint32 nLength)
{
	FRAME* pFrame = GetFrame(0);
	BITBLTBUF* pBuf = GetBitBltBuf();

	TexCache_InvalidateTextures(nAddress, nLength);

	uint32 nFrameEnd = (pFrame->GetBasePtr() + (pFrame->GetWidth() * GetPsmPixelSize(pFrame->nPsm) / 8) * m_nHeight);
	if(nAddress < nFrameEnd)
	{
		if((pFrame->nPsm == PSMCT24) && (pBuf->nDstPsm == PSMT4HH || pBuf->nDstPsm == PSMT4HL || pBuf->nDstPsm == PSMT8H)) return;
        //This is for Atelier Iris
        if(pBuf->nDstPsm == PSMCT24) return;
		DisplayTransferedImage(nAddress);
	}
}

void CGSH_OpenGL::ReadFramebuffer(uint32 width, uint32 height, void* buffer)
{
	glFlush();
	glFinish();
	glReadPixels(0, 0, width, height, GL_BGR, GL_UNSIGNED_BYTE, buffer);
}

void CGSH_OpenGL::DisplayTransferedImage(uint32 nAddress)
{
	unsigned int nW, nH;
	unsigned int nW2, nH2;
	unsigned int nDX, nDY;
	unsigned int nTexture;
//	uint32* pImg;
	double nS, nT;
	TRXREG* pReg;
	TRXPOS* pPos;

	pReg = GetTrxReg();
	pPos = GetTrxPos();

	nW = pReg->nRRW;
	nH = pReg->nRRH;

	nDX = pPos->nDSAX;
	nDY = pPos->nDSAY;

//	pImg = (uint32*)(m_pRAM + nAddress);

	glGenTextures(1, &nTexture);

	glBindTexture(GL_TEXTURE_2D, nTexture);

	nW2 = GetNextPowerOf2(nW);
	nH2 = GetNextPowerOf2(nH);

	FetchImagePSMCT32(reinterpret_cast<uint32*>(m_pCvtBuffer), nAddress, nW / 64, nW, nH);

	//Upload the texture
	glPixelStorei(GL_UNPACK_ROW_LENGTH, nW);

	//glTexImage2D(GL_TEXTURE_2D, 0, 4, nW2, nH2, 0, GL_RGBA, GL_UNSIGNED_BYTE, pImg);
	glTexImage2D(GL_TEXTURE_2D, 0, 4, nW2, nH2, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_pCvtBuffer);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glColor4d(1.0, 1.0, 1.0, 1.0);

	nS = (double)nW / (double)nW2;
	nT = (double)nH / (double)nH2;

	glBegin(GL_QUADS);
		
		glTexCoord2d(0.0,		0.0);
		glVertex2d(nDX,			nDY);

		glTexCoord2d(0.0,		nT);
		glVertex2d(nDX,			nDY + nH);

		glTexCoord2d(nS,		nT);
		glVertex2d(nDX + nW,	nDY + nH);

		glTexCoord2d(nS,		0.0);
		glVertex2d(nDX + nW,	nDY);

	glEnd();

	glBindTexture(GL_TEXTURE_2D, NULL);

	glDeleteTextures(1, &nTexture);

    PresentBackbuffer();
}

bool CGSH_OpenGL::IsColorTableExtSupported()
{
	return glColorTableEXT != NULL;
}

bool CGSH_OpenGL::IsBlendColorExtSupported()
{
	return glBlendColorEXT != NULL;
}

bool CGSH_OpenGL::IsBlendEquationExtSupported()
{
	return glBlendEquationEXT != NULL;
}

bool CGSH_OpenGL::IsRGBA5551ExtSupported()
{
	return m_nIsRGBA5551Supported;
}

bool CGSH_OpenGL::IsFogCoordfExtSupported()
{
	return glFogCoordfEXT != NULL;
}
