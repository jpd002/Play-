#include <stdio.h>
#include <assert.h>
#include <math.h>
#include "AppConfig.h"
#include "GSH_OpenGL.h"
#include "PtrMacro.h"
#include "Log.h"
#include "GsPixelFormats.h"

//#define _WIREFRAME

//#define HIGHRES_MODE
#ifdef HIGHRES_MODE
#define FBSCALE (2.0f)
#else
#define FBSCALE (1.0f)
#endif

GLenum CGSH_OpenGL::g_nativeClampModes[CGSHandler::CLAMP_MODE_MAX] =
{
	GL_REPEAT,
	GL_CLAMP_TO_EDGE,
	GL_REPEAT,
	GL_REPEAT
};

unsigned int CGSH_OpenGL::g_shaderClampModes[CGSHandler::CLAMP_MODE_MAX] =
{
	TEXTURE_CLAMP_MODE_STD,
	TEXTURE_CLAMP_MODE_STD,
	TEXTURE_CLAMP_MODE_REGION_CLAMP,
	TEXTURE_CLAMP_MODE_REGION_REPEAT
};

CGSH_OpenGL::CGSH_OpenGL() 
: m_pCvtBuffer(nullptr)
{
	CAppConfig::GetInstance().RegisterPreferenceBoolean(PREF_CGSH_OPENGL_LINEASQUADS, false);
	CAppConfig::GetInstance().RegisterPreferenceBoolean(PREF_CGSH_OPENGL_FORCEBILINEARTEXTURES, false);
	CAppConfig::GetInstance().RegisterPreferenceBoolean(PREF_CGSH_OPENGL_FIXSMALLZVALUES, false);

	LoadSettings();

	m_pCvtBuffer = new uint8[CVTBUFFERSIZE];

	memset(&m_renderState, 0, sizeof(m_renderState));
}

CGSH_OpenGL::~CGSH_OpenGL()
{
	delete [] m_pCvtBuffer;
}

void CGSH_OpenGL::InitializeImpl()
{
	InitializeRC();

	m_nVtxCount = 0;

	for(unsigned int i = 0; i < MAX_TEXTURE_CACHE; i++)
	{
		m_textureCache.push_back(TexturePtr(new CTexture()));
	}

	for(unsigned int i = 0; i < MAX_PALETTE_CACHE; i++)
	{
		m_paletteCache.push_back(PalettePtr(new CPalette()));
	}

	m_nMaxZ = 32768.0;
	m_renderState.isValid = false;
}

void CGSH_OpenGL::ReleaseImpl()
{
	ResetImpl();

	m_textureCache.clear();
	m_paletteCache.clear();
	m_shaderInfos.clear();
}

void CGSH_OpenGL::ResetImpl()
{
	TexCache_Flush();
	PalCache_Flush();
	m_framebuffers.clear();
	m_depthbuffers.clear();
}

void CGSH_OpenGL::FlipImpl()
{
	//Clear all of our output framebuffer
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glDisable(GL_SCISSOR_TEST);
		glClearColor(0, 0, 0, 0);
		glViewport(0, 0, m_presentationParams.windowWidth, m_presentationParams.windowHeight);
		glClear(GL_COLOR_BUFFER_BIT);
	}

	unsigned int sourceWidth = GetCrtWidth();
	unsigned int sourceHeight = GetCrtHeight();
	switch(m_presentationParams.mode)
	{
	case PRESENTATION_MODE_FILL:
		glViewport(0, 0, m_presentationParams.windowWidth, m_presentationParams.windowHeight);
		break;
	case PRESENTATION_MODE_FIT:
		{
			int viewportWidth[2];
			int viewportHeight[2];
			{
				viewportWidth[0] = m_presentationParams.windowWidth;
				viewportHeight[0] = (sourceWidth != 0) ? (m_presentationParams.windowWidth * sourceHeight) / sourceWidth : 0;
			}
			{
				viewportWidth[1] = (sourceHeight != 0) ? (m_presentationParams.windowHeight * sourceWidth) / sourceHeight : 0;
				viewportHeight[1] = m_presentationParams.windowHeight;
			}
			int selectedViewport = 0;
			if(
			   (viewportWidth[0] > static_cast<int>(m_presentationParams.windowWidth)) ||
			   (viewportHeight[0] > static_cast<int>(m_presentationParams.windowHeight))
			   )
			{
				selectedViewport = 1;
				assert(
					   viewportWidth[1] <= static_cast<int>(m_presentationParams.windowWidth) &&
					   viewportHeight[1] <= static_cast<int>(m_presentationParams.windowHeight));
			}
			int offsetX = static_cast<int>(m_presentationParams.windowWidth - viewportWidth[selectedViewport]) / 2;
			int offsetY = static_cast<int>(m_presentationParams.windowHeight - viewportHeight[selectedViewport]) / 2;
			glViewport(offsetX, offsetY, viewportWidth[selectedViewport], viewportHeight[selectedViewport]);
		}
		break;
	case PRESENTATION_MODE_ORIGINAL:
		{
			int offsetX = static_cast<int>(m_presentationParams.windowWidth - sourceWidth) / 2;
			int offsetY = static_cast<int>(m_presentationParams.windowHeight - sourceHeight) / 2;
			glViewport(offsetX, offsetY, sourceWidth, sourceHeight);
		}
		break;
	}

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

	FramebufferPtr framebuffer;
	for(const auto& candidateFramebuffer : m_framebuffers)
	{
		if(
			(candidateFramebuffer->m_basePtr == fb.GetBufPtr()) &&
			(candidateFramebuffer->m_width == fb.GetBufWidth())
			)
		{
			//We have a winner
			framebuffer = candidateFramebuffer;
			break;
		}
	}

	if(!framebuffer && (fb.GetBufWidth() != 0))
	{
		framebuffer = FramebufferPtr(new CFramebuffer(fb.GetBufPtr(), fb.GetBufWidth(), 1024, fb.nPSM));
		m_framebuffers.push_back(framebuffer);
#ifndef HIGHRES_MODE
		PopulateFramebuffer(framebuffer);
#endif
	}

	if(framebuffer)
	{
		//BGDA (US version) requires the interlaced check to work properly
		//Data read from dirty pages here would probably need to be scaled up by 2
		//if we are in interlaced mode (guessing that Unreal Tournament would need that here)
		bool halfHeight = GetCrtIsInterlaced() && GetCrtIsFrameMode();
		CommitFramebufferDirtyPages(framebuffer, 0, halfHeight ? (dispHeight / 2) : dispHeight);
	}

	if(framebuffer)
	{
		float u0 = 0;
		float u1 = static_cast<float>(dispWidth) / static_cast<float>(framebuffer->m_width);

		float v0 = 0;
		float v1 = static_cast<float>(dispHeight) / static_cast<float>(framebuffer->m_height);

		glUseProgram(0);

		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();

		glDisable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_ALPHA_TEST);
		glColor4f(1, 1, 1, 1);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, 0);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, framebuffer->m_texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();

		glBegin(GL_QUADS);
		{
			glTexCoord2f(u1, v0);
			glVertex2f(1, 1);

			glTexCoord2f(u1, v1);
			glVertex2f(1, -1);

			glTexCoord2f(u0, v1);
			glVertex2f(-1, -1);

			glTexCoord2f(u0, v0);
			glVertex2f(-1, 1);
		}
		glEnd();

		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
	}

	m_renderState.isValid = false;

	assert(glGetError() == GL_NO_ERROR);

	static bool g_dumpFramebuffers = false;
	if(g_dumpFramebuffers)
	{
		for(const auto& framebuffer : m_framebuffers)
		{
			glBindTexture(GL_TEXTURE_2D, framebuffer->m_texture);
			DumpTexture(framebuffer->m_width * FBSCALE, framebuffer->m_height * FBSCALE, framebuffer->m_basePtr);
		}
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	PresentBackbuffer();
	CGSHandler::FlipImpl();
}

void CGSH_OpenGL::LoadState(Framework::CZipArchiveReader& archive)
{
	CGSHandler::LoadState(archive);

	m_mailBox.SendCall(std::bind(&CGSH_OpenGL::TexCache_InvalidateTextures, this, 0, RAMSIZE));
}

void CGSH_OpenGL::LoadSettings()
{
	CGSHandler::LoadSettings();

	m_nLinesAsQuads				= CAppConfig::GetInstance().GetPreferenceBoolean(PREF_CGSH_OPENGL_LINEASQUADS);
	m_nForceBilinearTextures	= CAppConfig::GetInstance().GetPreferenceBoolean(PREF_CGSH_OPENGL_FORCEBILINEARTEXTURES);
	m_fixSmallZValues			= CAppConfig::GetInstance().GetPreferenceBoolean(PREF_CGSH_OPENGL_FIXSMALLZVALUES);
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

	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	//Initialize fog
	glFogi(GL_FOG_MODE, GL_LINEAR);
	glFogf(GL_FOG_START, 0.0f);
	glFogf(GL_FOG_END, 1.0f);
	glHint(GL_FOG_HINT, GL_NICEST);
	glFogi(GL_FOG_COORDINATE_SOURCE_EXT, GL_FOG_COORDINATE_EXT);

	SetupTextureUploaders();

#ifdef _WIREFRAME
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDisable(GL_TEXTURE_2D);
#endif

	PresentBackbuffer();
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

unsigned int CGSH_OpenGL::GetCurrentReadCircuit()
{
//	assert((m_nPMODE & 0x3) != 0x03);
	if(m_nPMODE & 0x1) return 0;
	if(m_nPMODE & 0x2) return 1;
	//Getting here is bad
	return 0;
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

void CGSH_OpenGL::SetRenderingContext(uint64 primReg)
{
	auto prim = make_convertible<PRMODE>(primReg);

	unsigned int context = prim.nContext;

	uint64 testReg = m_nReg[GS_REG_TEST_1 + context];
	uint64 frameReg = m_nReg[GS_REG_FRAME_1 + context];
	uint64 alphaReg = m_nReg[GS_REG_ALPHA_1 + context];
	uint64 zbufReg = m_nReg[GS_REG_ZBUF_1 + context];
	uint64 tex0Reg = m_nReg[GS_REG_TEX0_1 + context];
	uint64 tex1Reg = m_nReg[GS_REG_TEX1_1 + context];
	uint64 texAReg = m_nReg[GS_REG_TEXA];
	uint64 clampReg = m_nReg[GS_REG_CLAMP_1 + context];
	uint64 scissorReg = m_nReg[GS_REG_SCISSOR_1 + context];

	//--------------------------------------------------------
	//Get shader caps
	//--------------------------------------------------------

	SHADERCAPS shaderCaps;
	memset(&shaderCaps, 0, sizeof(SHADERCAPS));

	FillShaderCapsFromTexture(shaderCaps, tex0Reg, tex1Reg, texAReg, clampReg);

	if(prim.nFog)
	{
		shaderCaps.hasFog = 1;
	}

	if(!prim.nTexture)
	{
		shaderCaps.texSourceMode = TEXTURE_SOURCE_MODE_NONE;
	}

	//--------------------------------------------------------
	//Create shader if it doesn't exist
	//--------------------------------------------------------

	auto shaderInfoIterator = m_shaderInfos.find(static_cast<uint32>(shaderCaps));
	if(shaderInfoIterator == m_shaderInfos.end())
	{
		auto shader = GenerateShader(shaderCaps);
		SHADERINFO shaderInfo;
		shaderInfo.shader = shader;

		shaderInfo.textureUniform		= glGetUniformLocation(*shader, "g_texture");
		shaderInfo.paletteUniform		= glGetUniformLocation(*shader, "g_palette");
		shaderInfo.textureSizeUniform	= glGetUniformLocation(*shader, "g_textureSize");
		shaderInfo.texelSizeUniform		= glGetUniformLocation(*shader, "g_texelSize");
		shaderInfo.clampMinUniform		= glGetUniformLocation(*shader, "g_clampMin");
		shaderInfo.clampMaxUniform		= glGetUniformLocation(*shader, "g_clampMax");
		shaderInfo.texA0Uniform			= glGetUniformLocation(*shader, "g_texA0");
		shaderInfo.texA1Uniform			= glGetUniformLocation(*shader, "g_texA1");

		m_shaderInfos.insert(ShaderInfoMap::value_type(static_cast<uint32>(shaderCaps), shaderInfo));
		shaderInfoIterator = m_shaderInfos.find(static_cast<uint32>(shaderCaps));
	}

	const auto& shaderInfo = shaderInfoIterator->second;

	//--------------------------------------------------------
	//Bind shader
	//--------------------------------------------------------

	GLuint shaderHandle = *shaderInfo.shader;
	if(!m_renderState.isValid ||
		(m_renderState.shaderHandle != shaderHandle))
	{
		glUseProgram(shaderHandle);

		if(shaderInfo.textureUniform != -1)
		{
			glUniform1i(shaderInfo.textureUniform, 0);
		}

		if(shaderInfo.paletteUniform != -1)
		{
			glUniform1i(shaderInfo.paletteUniform, 1);
		}

		//Invalidate because we might need to set uniforms again
		m_renderState.isValid = false;
	}

	//--------------------------------------------------------
	//Set render states
	//--------------------------------------------------------

	if(!m_renderState.isValid ||
		(m_renderState.primReg != primReg))
	{
		//Humm, not quite sure about this
//		if(prim.nAntiAliasing)
//		{
//			glEnable(GL_BLEND);
//		}
//		else
//		{
//			glDisable(GL_BLEND);
//		}

		if(prim.nShading)
		{
			glShadeModel(GL_SMOOTH);
		}
		else
		{
			glShadeModel(GL_FLAT);
		}

		if(prim.nAlpha)
		{
			glEnable(GL_BLEND);
		}
		else
		{
			glDisable(GL_BLEND);
		}
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
		(m_renderState.testReg != testReg))
	{
		SetupDepthBuffer(zbufReg, testReg);
	}

	if(!m_renderState.isValid ||
		(m_renderState.frameReg != frameReg) ||
		(m_renderState.zbufReg != zbufReg) ||
		(m_renderState.scissorReg != scissorReg))
	{
		SetupFramebuffer(frameReg, zbufReg, scissorReg);
	}

	if(!m_renderState.isValid ||
		(m_renderState.tex0Reg != tex0Reg) ||
		(m_renderState.tex1Reg != tex1Reg) ||
		(m_renderState.texAReg != texAReg) ||
		(m_renderState.clampReg != clampReg) ||
		(m_renderState.primReg != primReg))
	{
		SetupTexture(shaderInfo, primReg, tex0Reg, tex1Reg, texAReg, clampReg);
	}

	XYOFFSET offset;
	offset <<= m_nReg[GS_REG_XYOFFSET_1 + context];
	m_nPrimOfsX = offset.GetX();
	m_nPrimOfsY = offset.GetY();
	
	assert(glGetError() == GL_NO_ERROR);

	m_renderState.isValid = true;
	m_renderState.shaderHandle = shaderHandle;
	m_renderState.primReg = primReg;
	m_renderState.alphaReg = alphaReg;
	m_renderState.testReg = testReg;
	m_renderState.zbufReg = zbufReg;
	m_renderState.scissorReg = scissorReg;
	m_renderState.frameReg = frameReg;
	m_renderState.tex0Reg = tex0Reg;
	m_renderState.tex1Reg = tex1Reg;
	m_renderState.texAReg = texAReg;
	m_renderState.clampReg = clampReg;
}

void CGSH_OpenGL::SetupBlendingFunction(uint64 alphaReg)
{
	int nFunction = GL_FUNC_ADD_EXT;
	auto alpha = make_convertible<ALPHA>(alphaReg);

	if((alpha.nA == 0) && (alpha.nB == 0) && (alpha.nC == 0) && (alpha.nD == 0))
	{
		glBlendFuncSeparate(GL_ONE, GL_ZERO, GL_ONE, GL_ZERO);
	}
	else if((alpha.nA == 0) && (alpha.nB == 1) && (alpha.nC == 0) && (alpha.nD == 1))
	{
		glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);
	}
	else if((alpha.nA == 0) && (alpha.nB == 1) && (alpha.nC == 1) && (alpha.nD == 1))
	{
		//Cs * Ad + Cd * (1 - Ad)
		glBlendFuncSeparate(GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA, GL_ONE, GL_ZERO);
	}
	else if((alpha.nA == 0) && (alpha.nB == 1) && (alpha.nC == 2) && (alpha.nD == 1))
	{
		if(alpha.nFix == 0x80)
		{
			glBlendFuncSeparate(GL_ONE, GL_ZERO, GL_ONE, GL_ZERO);
		}
		else
		{
			//Source alpha value is implied in the formula
			//As = FIX / 0x80
			if(glBlendColorEXT != NULL)
			{
				glBlendColorEXT(0.0f, 0.0f, 0.0f, (float)alpha.nFix / 128.0f);
				glBlendFuncSeparate(GL_CONSTANT_ALPHA_EXT, GL_ONE_MINUS_CONSTANT_ALPHA_EXT, GL_ONE, GL_ZERO);
			}
		}
	}
	else if((alpha.nA == 0) && (alpha.nB == 2) && (alpha.nC == 0) && (alpha.nD == 1))
	{
		glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_ONE, GL_ZERO);
	}
	else if((alpha.nA == 0) && (alpha.nB == 2) && (alpha.nC == 0) && (alpha.nD == 2))
	{
		//Cs * As
		glBlendFuncSeparate(GL_SRC_ALPHA, GL_ZERO, GL_ONE, GL_ZERO);
	}
	else if((alpha.nA == 0) && (alpha.nB == 2) && (alpha.nC == 1) && (alpha.nD == 1))
	{
		//Cs * Ad + Cd
		glBlendFuncSeparate(GL_DST_ALPHA, GL_ONE, GL_ONE, GL_ZERO);
	}
	else if((alpha.nA == 0) && (alpha.nB == 2) && (alpha.nC == 2) && (alpha.nD == 1))
	{
		if(alpha.nFix == 0x80)
		{
			glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ONE, GL_ZERO);
		}
		else
		{
			//Cs * FIX + Cd
			if(glBlendColor != NULL)
			{
				glBlendColor(0, 0, 0, static_cast<float>(alpha.nFix) / 128.0f);
				glBlendFuncSeparate(GL_CONSTANT_ALPHA, GL_ONE, GL_ONE, GL_ZERO);
			}
		}
	}
	else if((alpha.nA == 1) && (alpha.nB == 0) && (alpha.nC == 0) && (alpha.nD == 0))
	{
		//(Cd - Cs) * As + Cs
		glBlendFuncSeparate(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE, GL_ZERO);
	}
	else if((alpha.nA == 1) && (alpha.nB == 0) && (alpha.nC == 2) && (alpha.nD == 2))
	{
		nFunction = GL_FUNC_REVERSE_SUBTRACT_EXT;
		if(glBlendColorEXT != NULL)
		{
			glBlendColorEXT(0.0f, 0.0f, 0.0f, (float)alpha.nFix / 128.0f);
			glBlendFuncSeparate(GL_CONSTANT_ALPHA_EXT, GL_CONSTANT_ALPHA_EXT, GL_ONE, GL_ZERO);
		}
	}
//	else if((alpha.nA == 1) && (alpha.nB == 2) && (alpha.nC == 0) && (alpha.nD == 1))
//	{
//		//Cd * As + Cd
//		//Implemented as Cd * As
//		glBlendFunc(GL_ZERO, GL_SRC_ALPHA);
//	}
	else if((alpha.nA == 1) && (alpha.nB == 2) && (alpha.nC == 0) && (alpha.nD == 0))
	{
		//Cd * As + Cs
		glBlendFuncSeparate(GL_ONE, GL_SRC_ALPHA, GL_ONE, GL_ZERO);
	}
	else if((alpha.nA == 1) && (alpha.nB == 2) && (alpha.nC == 0) && (alpha.nD == 2))
	{
		//Cd * As
		//glBlendFunc(GL_ZERO, GL_SRC_ALPHA);
		//REMOVE
		glBlendFuncSeparate(GL_ZERO, GL_ONE, GL_ONE, GL_ZERO);
		//REMOVE
	}
	else if((alpha.nA == 2) && (alpha.nB == 0) && (alpha.nC == 0) && (alpha.nD == 1))
	{
		nFunction = GL_FUNC_REVERSE_SUBTRACT_EXT;
		glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_ONE, GL_ZERO);
	}
	else if((alpha.nA == 2) && (alpha.nB == 2) && (alpha.nC == 2) && (alpha.nD == 1))
	{
		//Cd (no blend)
		glBlendFuncSeparate(GL_ZERO, GL_ONE, GL_ONE, GL_ZERO);
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
		static const GLenum g_alphaTestFunc[ALPHA_TEST_MAX] =
		{
			GL_NEVER,
			GL_ALWAYS,
			GL_LESS,
			GL_LEQUAL,
			GL_EQUAL,
			GL_GEQUAL,
			GL_GREATER,
			GL_NOTEQUAL
		};

		//Special way of turning off depth writes:
		//Always fail alpha testing but write RGBA and not depth if it fails
		if(tst.nAlphaMethod == ALPHA_TEST_NEVER && tst.nAlphaFail == ALPHA_TEST_FAIL_FBONLY)
		{
			glDisable(GL_ALPHA_TEST);
		}
		else
		{
			float nValue = (float)tst.nAlphaRef / 255.0f;
			glAlphaFunc(g_alphaTestFunc[tst.nAlphaMethod], nValue);

			glEnable(GL_ALPHA_TEST);
		}
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

void CGSH_OpenGL::SetupDepthBuffer(uint64 zbufReg, uint64 testReg)
{
	auto zbuf = make_convertible<ZBUF>(zbufReg);
	auto test = make_convertible<TEST>(testReg);

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

	bool depthWriteEnabled = (zbuf.nMask ? false : true);
	//If alpha test is enabled for always failing and update only colors, depth writes are disabled
	if((test.nAlphaEnabled == 1) && (test.nAlphaMethod == 0) && (test.nAlphaFail == 1))
	{
		depthWriteEnabled = false;
	}
	glDepthMask(depthWriteEnabled ? GL_TRUE : GL_FALSE);
}

void CGSH_OpenGL::SetupFramebuffer(uint64 frameReg, uint64 zbufReg, uint64 scissorReg)
{
	if(frameReg == 0) return;

	auto frame = make_convertible<FRAME>(frameReg);
	auto zbuf = make_convertible<ZBUF>(zbufReg);
	auto scissor = make_convertible<SCISSOR>(scissorReg);

	bool r = (frame.nMask & 0x000000FF) == 0;
	bool g = (frame.nMask & 0x0000FF00) == 0;
	bool b = (frame.nMask & 0x00FF0000) == 0;
	bool a = (frame.nMask & 0xFF000000) == 0;
	glColorMask(r, g, b, a);

	//Look for a framebuffer that matches the specified information
	auto framebuffer = FindFramebuffer(frame);
	if(!framebuffer)
	{
		framebuffer = FramebufferPtr(new CFramebuffer(frame.GetBasePtr(), frame.GetWidth(), 1024, frame.nPsm));
		m_framebuffers.push_back(framebuffer);
#ifndef HIGHRES_MODE
		PopulateFramebuffer(framebuffer);
#endif
	}

	CommitFramebufferDirtyPages(framebuffer, scissor.scay0, scissor.scay1);

	auto depthbuffer = FindDepthbuffer(zbuf, frame);
	if(!depthbuffer)
	{
		depthbuffer = DepthbufferPtr(new CDepthbuffer(zbuf.GetBasePtr(), frame.GetWidth(), 1024, zbuf.nPsm));
		m_depthbuffers.push_back(depthbuffer);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer->m_framebuffer);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthbuffer->m_depthBuffer);
	assert(glGetError() == GL_NO_ERROR);

	GLenum result = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	assert(result == GL_FRAMEBUFFER_COMPLETE);

	{
		GLenum drawBufferId = GL_COLOR_ATTACHMENT0;
		glDrawBuffers(1, &drawBufferId);
		assert(glGetError() == GL_NO_ERROR);
	}

	glViewport(0, 0, framebuffer->m_width * FBSCALE, framebuffer->m_height * FBSCALE);
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	bool halfHeight = GetCrtIsInterlaced() && GetCrtIsFrameMode();
	float projWidth = static_cast<float>(framebuffer->m_width);
	float projHeight = static_cast<float>(halfHeight ? (framebuffer->m_height / 2) : framebuffer->m_height);
	LinearZOrtho(0, projWidth, 0, projHeight);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glEnable(GL_SCISSOR_TEST);
	int scissorX = scissor.scax0;
	int scissorY = scissor.scay0;
	int scissorWidth = scissor.scax1 - scissor.scax0 + 1;
	int scissorHeight = scissor.scay1 - scissor.scay0 + 1;
	if(halfHeight)
	{
		scissorY *= 2;
		scissorHeight *= 2;
	}
	glScissor(scissorX * FBSCALE, scissorY * FBSCALE, scissorWidth * FBSCALE, scissorHeight * FBSCALE);
}

void CGSH_OpenGL::SetupFogColor()
{
	float nColor[4];

	FOGCOL color;
	color <<= m_nReg[GS_REG_FOGCOL];
	nColor[0] = static_cast<float>(color.nFCR) / 255.0f;
	nColor[1] = static_cast<float>(color.nFCG) / 255.0f;
	nColor[2] = static_cast<float>(color.nFCB) / 255.0f;
	nColor[3] = 0.0f;

	glFogfv(GL_FOG_COLOR, nColor);
}

bool CGSH_OpenGL::CanRegionRepeatClampModeSimplified(uint32 clampMin, uint32 clampMax)
{
	for(unsigned int j = 1; j < 0x3FF; j = ((j << 1) | 1))
	{
		if(clampMin < j) break;
		if(clampMin != j) continue;

		if((clampMin & clampMax) != 0) break;

		return true;
	}

	return false;
}

void CGSH_OpenGL::FillShaderCapsFromTexture(SHADERCAPS& shaderCaps, uint64 tex0Reg, uint64 tex1Reg, uint64 texAReg, uint64 clampReg)
{
	auto tex0 = make_convertible<TEX0>(tex0Reg);
	auto tex1 = make_convertible<TEX1>(tex1Reg);
	auto texA = make_convertible<TEXA>(texAReg);
	auto clamp = make_convertible<CLAMP>(clampReg);

	shaderCaps.texSourceMode = TEXTURE_SOURCE_MODE_STD;

	if((clamp.nWMS > CLAMP_MODE_CLAMP) || (clamp.nWMT > CLAMP_MODE_CLAMP))
	{
		unsigned int clampMode[2];

		clampMode[0] = g_shaderClampModes[clamp.nWMS];
		clampMode[1] = g_shaderClampModes[clamp.nWMT];

		if(clampMode[0] == TEXTURE_CLAMP_MODE_REGION_REPEAT && CanRegionRepeatClampModeSimplified(clamp.GetMinU(), clamp.GetMaxU())) clampMode[0] = TEXTURE_CLAMP_MODE_REGION_REPEAT_SIMPLE;
		if(clampMode[1] == TEXTURE_CLAMP_MODE_REGION_REPEAT && CanRegionRepeatClampModeSimplified(clamp.GetMinV(), clamp.GetMaxV())) clampMode[1] = TEXTURE_CLAMP_MODE_REGION_REPEAT_SIMPLE;

		shaderCaps.texClampS = clampMode[0];
		shaderCaps.texClampT = clampMode[1];
	}

	if(CGsPixelFormats::IsPsmIDTEX(tex0.nPsm) && ((tex1.nMinFilter != MIN_FILTER_NEAREST) || (tex1.nMagFilter != MIN_FILTER_NEAREST)))
	{
		//We'll need to filter the texture manually
		shaderCaps.texBilinearFilter = 1;
	}

	if(tex0.nColorComp == 1)
	{
		shaderCaps.texHasAlpha = 1;
	}

	if((tex0.nPsm == PSMCT16) || (tex0.nPsm == PSMCT16S) || (tex0.nPsm == PSMCT24))
	{
		shaderCaps.texUseAlphaExpansion = 1;
	}

	if(CGsPixelFormats::IsPsmIDTEX(tex0.nPsm))
	{
		if((tex0.nCPSM == PSMCT16) || (tex0.nCPSM == PSMCT16S))
		{
			shaderCaps.texUseAlphaExpansion = 1;
		}

		shaderCaps.texSourceMode = CGsPixelFormats::IsPsmIDTEX4(tex0.nPsm) ? TEXTURE_SOURCE_MODE_IDX4 : TEXTURE_SOURCE_MODE_IDX8;
	}

	if(texA.nAEM)
	{
		shaderCaps.texBlackIsTransparent = 1;
	}

	shaderCaps.texFunction = tex0.nFunction;
}

void CGSH_OpenGL::SetupTexture(const SHADERINFO& shaderInfo, uint64 primReg, uint64 tex0Reg, uint64 tex1Reg, uint64 texAReg, uint64 clampReg)
{
	auto prim = make_convertible<PRMODE>(primReg);

	if(tex0Reg == 0 || prim.nTexture == 0)
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, 0);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, 0);
		return;
	}

	auto tex0 = make_convertible<TEX0>(tex0Reg);
	auto tex1 = make_convertible<TEX1>(tex1Reg);
	auto texA = make_convertible<TEXA>(texAReg);
	auto clamp = make_convertible<CLAMP>(clampReg);

	m_nTexWidth = tex0.GetWidth();
	m_nTexHeight = tex0.GetHeight();

	glActiveTexture(GL_TEXTURE0);
	auto texInfo = PrepareTexture(tex0);

	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glTranslatef(texInfo.offsetX, 0, 0);
	glScalef(texInfo.scaleRatioX, texInfo.scaleRatioY, 1);

	int nMagFilter, nMinFilter;

	//Setup sampling modes
	if(tex1.nMagFilter == MAG_FILTER_NEAREST)
	{
		nMagFilter = GL_NEAREST;
	}
	else
	{
		nMagFilter = GL_LINEAR;
	}

	switch(tex1.nMinFilter)
	{
	case MIN_FILTER_NEAREST:
		nMinFilter = GL_NEAREST;
		break;
	case MIN_FILTER_LINEAR:
		nMinFilter = GL_LINEAR;
		break;
	case MIN_FILTER_LINEAR_MIP_LINEAR:
		nMinFilter = GL_LINEAR_MIPMAP_LINEAR;
		break;
	default:
		assert(0);
		break;
	}

	if(m_nForceBilinearTextures)
	{
		nMagFilter = GL_LINEAR;
		nMinFilter = GL_LINEAR;
	}

	unsigned int clampMin[2] = { 0, 0 };
	unsigned int clampMax[2] = { 0, 0 };
	float textureScaleRatio[2] = { texInfo.scaleRatioX, texInfo.scaleRatioY };
	GLenum nWrapS = g_nativeClampModes[clamp.nWMS];
	GLenum nWrapT = g_nativeClampModes[clamp.nWMT];

	if((clamp.nWMS > CLAMP_MODE_CLAMP) || (clamp.nWMT > CLAMP_MODE_CLAMP))
	{
		unsigned int clampMode[2];

		clampMode[0] = g_shaderClampModes[clamp.nWMS];
		clampMode[1] = g_shaderClampModes[clamp.nWMT];

		clampMin[0] = clamp.GetMinU();
		clampMin[1] = clamp.GetMinV();

		clampMax[0] = clamp.GetMaxU();
		clampMax[1] = clamp.GetMaxV();

		for(unsigned int i = 0; i < 2; i++)
		{
			if(clampMode[i] == TEXTURE_CLAMP_MODE_REGION_REPEAT)
			{
				if(CanRegionRepeatClampModeSimplified(clampMin[i], clampMax[i]))
				{
					clampMin[i]++;
				}
			}
			if(clampMode[i] == TEXTURE_CLAMP_MODE_REGION_CLAMP)
			{
				clampMin[i] = static_cast<unsigned int>(static_cast<float>(clampMin[i]) * textureScaleRatio[i]);
				clampMax[i] = static_cast<unsigned int>(static_cast<float>(clampMax[i]) * textureScaleRatio[i]);
			}
		}
	}

	if(CGsPixelFormats::IsPsmIDTEX(tex0.nPsm) && (nMinFilter != GL_NEAREST || nMagFilter != GL_NEAREST))
	{
		//We'll need to filter the texture manually
		nMinFilter = GL_NEAREST;
		nMagFilter = GL_NEAREST;
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, nMagFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, nMinFilter);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, nWrapS);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, nWrapT);

	if(CGsPixelFormats::IsPsmIDTEX(tex0.nPsm))
	{
		glActiveTexture(GL_TEXTURE1);
		PreparePalette(tex0);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}

	if(shaderInfo.textureSizeUniform != -1)
	{
		glUniform2f(shaderInfo.textureSizeUniform, 
			static_cast<float>(tex0.GetWidth()), static_cast<float>(tex0.GetHeight()));
	}

	if(shaderInfo.texelSizeUniform != -1)
	{
		glUniform2f(shaderInfo.texelSizeUniform, 
			1.0f / static_cast<float>(tex0.GetWidth()), 1.0f / static_cast<float>(tex0.GetHeight()));
	}

	if(shaderInfo.clampMinUniform != -1)
	{
		glUniform2f(shaderInfo.clampMinUniform,
			static_cast<float>(clampMin[0]), static_cast<float>(clampMin[1]));
	}

	if(shaderInfo.clampMaxUniform != -1)
	{
		glUniform2f(shaderInfo.clampMaxUniform,
			static_cast<float>(clampMax[0]), static_cast<float>(clampMax[1]));
	}

	if(shaderInfo.texA0Uniform != -1)
	{
		float a = static_cast<float>(MulBy2Clamp(texA.nTA0)) / 255.f;
		glUniform1f(shaderInfo.texA0Uniform, a);
	}

	if(shaderInfo.texA1Uniform != -1)
	{
		float a = static_cast<float>(MulBy2Clamp(texA.nTA1)) / 255.f;
		glUniform1f(shaderInfo.texA1Uniform, a);
	}
}

CGSH_OpenGL::FramebufferPtr CGSH_OpenGL::FindFramebuffer(const FRAME& frame) const
{
	auto framebufferIterator = std::find_if(std::begin(m_framebuffers), std::end(m_framebuffers), 
		[&] (const FramebufferPtr& framebuffer)
		{
			return (framebuffer->m_basePtr == frame.GetBasePtr()) && (framebuffer->m_width == frame.GetWidth());
		}
	);

	return (framebufferIterator != std::end(m_framebuffers)) ? *(framebufferIterator) : FramebufferPtr();
}

CGSH_OpenGL::DepthbufferPtr CGSH_OpenGL::FindDepthbuffer(const ZBUF& zbuf, const FRAME& frame) const
{
	auto depthbufferIterator = std::find_if(std::begin(m_depthbuffers), std::end(m_depthbuffers), 
		[&] (const DepthbufferPtr& depthbuffer)
		{
			return (depthbuffer->m_basePtr == zbuf.GetBasePtr()) && (depthbuffer->m_width == frame.GetWidth());
		}
	);

	return (depthbufferIterator != std::end(m_depthbuffers)) ? *(depthbufferIterator) : DepthbufferPtr();
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

	if(m_PrimitiveMode.nFog)
	{
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
		}
		else
		{
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

			glBegin(GL_TRIANGLES);
			{
				glColor4ub(MulBy2Clamp(rgbaq[0].nR), MulBy2Clamp(rgbaq[0].nG), MulBy2Clamp(rgbaq[0].nB), MulBy2Clamp(rgbaq[0].nA));
				glTexCoord4f(nS1, nT1, 0, rgbaq[0].nQ);
				if(glFogCoordfEXT) glFogCoordfEXT(nF1);
				glVertex3f(nX1, nY1, nZ1);

				glColor4ub(MulBy2Clamp(rgbaq[1].nR), MulBy2Clamp(rgbaq[1].nG), MulBy2Clamp(rgbaq[1].nB), MulBy2Clamp(rgbaq[1].nA));
				glTexCoord4f(nS2, nT2, 0, rgbaq[1].nQ);
				if(glFogCoordfEXT) glFogCoordfEXT(nF2);
				glVertex3f(nX2, nY2, nZ2);

				glColor4ub(MulBy2Clamp(rgbaq[2].nR), MulBy2Clamp(rgbaq[2].nG), MulBy2Clamp(rgbaq[2].nB), MulBy2Clamp(rgbaq[2].nA));
				glTexCoord4f(nS3, nT3, 0, rgbaq[2].nQ);
				if(glFogCoordfEXT) glFogCoordfEXT(nF3);
				glVertex3f(nX3, nY3, nZ3);
			}
			glEnd();
		}
	}
	else
	{
		//Non Textured Triangle
		glBegin(GL_TRIANGLES);
			
			glColor4ub(rgbaq[0].nR, rgbaq[0].nG, rgbaq[0].nB, MulBy2Clamp(rgbaq[0].nA));
			if(glFogCoordfEXT) glFogCoordfEXT(nF1);
			glVertex3f(nX1, nY1, nZ1);

			glColor4ub(rgbaq[1].nR, rgbaq[1].nG, rgbaq[1].nB, MulBy2Clamp(rgbaq[1].nA));
			if(glFogCoordfEXT) glFogCoordfEXT(nF2);
			glVertex3f(nX2, nY2, nZ2);

			glColor4ub(rgbaq[2].nR, rgbaq[2].nG, rgbaq[2].nB, MulBy2Clamp(rgbaq[2].nA));
			if(glFogCoordfEXT) glFogCoordfEXT(nF3);
			glVertex3f(nX3, nY3, nZ3);

		glEnd();
	}
}

void CGSH_OpenGL::Prim_Sprite()
{
	RGBAQ rgbaq[2];

	XYZ xyz[2];
	xyz[0] <<= m_VtxBuffer[1].nPosition;
	xyz[1] <<= m_VtxBuffer[0].nPosition;

	float nX1 = xyz[0].GetX();	float nY1 = xyz[0].GetY();
	float nX2 = xyz[1].GetX();	float nY2 = xyz[1].GetY();	float nZ = xyz[1].GetZ();

	rgbaq[0] <<= m_VtxBuffer[1].nRGBAQ;
	rgbaq[1] <<= m_VtxBuffer[0].nRGBAQ;

	nX1 -= m_nPrimOfsX;
	nX2 -= m_nPrimOfsX;

	nY1 -= m_nPrimOfsY;
	nY2 -= m_nPrimOfsY;

	nZ = GetZ(nZ);

	if(m_PrimitiveMode.nTexture)
	{
		float nS[2], nT[2];

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
			glTexCoord2f(nS[0], nT[0]);
			glVertex3f(nX1, nY1, nZ);

			glTexCoord2f(nS[1], nT[0]);
			glVertex3f(nX2, nY1, nZ);

			glTexCoord2f(nS[1], nT[1]);
			glVertex3f(nX2, nY2, nZ);

			glTexCoord2f(nS[0], nT[1]);
			glVertex3f(nX1, nY2, nZ);

		}
		glEnd();
	}
	else if(!m_PrimitiveMode.nTexture)
	{
		//REMOVE
		//Humm? Would it be possible to have a gradient using those registers?
		glColor4ub(MulBy2Clamp(rgbaq[0].nR), MulBy2Clamp(rgbaq[0].nG), MulBy2Clamp(rgbaq[0].nB), MulBy2Clamp(rgbaq[0].nA));
		//glColor4ub(rgbaq[0].nR, rgbaq[0].nG, rgbaq[0].nB, rgbaq[0].nA);

		glBegin(GL_QUADS);

			glVertex3f(nX1, nY1, nZ);
			glVertex3f(nX2, nY1, nZ);
			glVertex3f(nX2, nY2, nZ);
			glVertex3f(nX1, nY2, nZ);

		glEnd();
	}
	else
	{
		assert(0);
	}
}

/////////////////////////////////////////////////////////////
// Other Functions
/////////////////////////////////////////////////////////////

void CGSH_OpenGL::WriteRegisterImpl(uint8 nRegister, uint64 nData)
{
	CGSHandler::WriteRegisterImpl(nRegister, nData);

	switch(nRegister)
	{
	case GS_REG_PRIM:
		m_nPrimitiveType = static_cast<unsigned int>(nData & 0x07);
		switch(m_nPrimitiveType)
		{
		case PRIM_POINT:
			m_nVtxCount = 1;
			break;
		case PRIM_LINE:
		case PRIM_LINESTRIP:
			m_nVtxCount = 2;
			break;
		case PRIM_TRIANGLE:
		case PRIM_TRIANGLESTRIP:
		case PRIM_TRIANGLEFAN:
			m_nVtxCount = 3;
			break;
		case PRIM_SPRITE:
			m_nVtxCount = 2;
			break;
		}
		break;

	case GS_REG_XYZ2:
	case GS_REG_XYZ3:
	case GS_REG_XYZF2:
	case GS_REG_XYZF3:
		VertexKick(nRegister, nData);
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

	if(!m_drawEnabled) nDrawingKick = false;

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
		if((m_nReg[GS_REG_PRMODECONT] & 1) != 0)
		{
			m_PrimitiveMode <<= m_nReg[GS_REG_PRIM];
		}
		else
		{
			m_PrimitiveMode <<= m_nReg[GS_REG_PRMODE];
		}

		if(nDrawingKick)
		{
			SetRenderingContext(m_PrimitiveMode);
			m_drawCallCount++;
		}

		switch(m_nPrimitiveType)
		{
		case PRIM_POINT:
			if(nDrawingKick) Prim_Point();
			break;
		case PRIM_LINE:
			if(nDrawingKick) Prim_Line();
			break;
		case PRIM_LINESTRIP:
			if(nDrawingKick) Prim_Line();
			memcpy(&m_VtxBuffer[1], &m_VtxBuffer[0], sizeof(VERTEX));
			m_nVtxCount = 1;
			break;
		case PRIM_TRIANGLE:
			if(nDrawingKick) Prim_Triangle();
			m_nVtxCount = 3;
			break;
		case PRIM_TRIANGLESTRIP:
			if(nDrawingKick) Prim_Triangle();
			memcpy(&m_VtxBuffer[2], &m_VtxBuffer[1], sizeof(VERTEX));
			memcpy(&m_VtxBuffer[1], &m_VtxBuffer[0], sizeof(VERTEX));
			m_nVtxCount = 1;
			break;
		case PRIM_TRIANGLEFAN:
			if(nDrawingKick) Prim_Triangle();
			memcpy(&m_VtxBuffer[1], &m_VtxBuffer[0], sizeof(VERTEX));
			m_nVtxCount = 1;
			break;
		case PRIM_SPRITE:
			if(nDrawingKick) Prim_Sprite();
			m_nVtxCount = 2;
			break;
		}
	}
}

void CGSH_OpenGL::ProcessImageTransfer()
{
	auto bltBuf = make_convertible<BITBLTBUF>(m_nReg[GS_REG_BITBLTBUF]);
	uint32 transferAddress = bltBuf.GetDstPtr();

	if(m_trxCtx.nDirty)
	{
		auto trxReg = make_convertible<TRXREG>(m_nReg[GS_REG_TRXREG]);
		auto trxPos = make_convertible<TRXPOS>(m_nReg[GS_REG_TRXPOS]);

		//Find the pages that are touched by this transfer
		auto transferPageSize = CGsPixelFormats::GetPsmPageSize(bltBuf.nDstPsm);

		uint32 pageCountX = (bltBuf.GetDstWidth() + transferPageSize.first - 1) / transferPageSize.first;
		uint32 pageCountY = (trxReg.nRRH + transferPageSize.second - 1) / transferPageSize.second;

		uint32 pageCount = pageCountX * pageCountY;
		uint32 transferSize = pageCount * CGsPixelFormats::PAGESIZE;
		uint32 transferOffset = (trxPos.nDSAY / transferPageSize.second) * pageCountX * CGsPixelFormats::PAGESIZE;

		TexCache_InvalidateTextures(transferAddress + transferOffset, transferSize);

		bool isUpperByteTransfer = (bltBuf.nDstPsm == PSMT8H) || (bltBuf.nDstPsm == PSMT4HL) || (bltBuf.nDstPsm == PSMT4HH);
		for(const auto& framebuffer : m_framebuffers)
		{
			if((framebuffer->m_psm == PSMCT24) && isUpperByteTransfer) continue;
			framebuffer->m_cachedArea.Invalidate(transferAddress + transferOffset, transferSize);
		}

		m_renderState.isValid = false;
	}
}

void CGSH_OpenGL::ProcessClutTransfer(uint32 csa, uint32)
{
	PalCache_Invalidate(csa);
	m_renderState.isValid = false;
}

void CGSH_OpenGL::ProcessLocalToLocalTransfer()
{
	auto bltBuf = make_convertible<BITBLTBUF>(m_nReg[GS_REG_BITBLTBUF]);
	auto srcFramebufferIterator = std::find_if(m_framebuffers.begin(), m_framebuffers.end(), 
		[&] (const FramebufferPtr& framebuffer) 
		{
			return (framebuffer->m_basePtr == bltBuf.GetSrcPtr()) &&
				(framebuffer->m_width == bltBuf.GetSrcWidth());
		}
	);
	auto dstFramebufferIterator = std::find_if(m_framebuffers.begin(), m_framebuffers.end(), 
		[&] (const FramebufferPtr& framebuffer) 
		{
			return (framebuffer->m_basePtr == bltBuf.GetDstPtr()) && 
				(framebuffer->m_width == bltBuf.GetDstWidth());
		}
	);
	if(
		srcFramebufferIterator != std::end(m_framebuffers) && 
		dstFramebufferIterator != std::end(m_framebuffers))
	{
		const auto& srcFramebuffer = (*srcFramebufferIterator);
		const auto& dstFramebuffer = (*dstFramebufferIterator);

		glBindFramebuffer(GL_FRAMEBUFFER, dstFramebuffer->m_framebuffer);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, srcFramebuffer->m_framebuffer);

		//Copy buffers
		glBlitFramebuffer(
			0, 0, srcFramebuffer->m_width, srcFramebuffer->m_height, 
			0, 0, srcFramebuffer->m_width, srcFramebuffer->m_height, 
			GL_COLOR_BUFFER_BIT, GL_NEAREST);
		assert(glGetError() == GL_NO_ERROR);

		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

		m_renderState.isValid = false;
	}
}

void CGSH_OpenGL::ReadFramebuffer(uint32 width, uint32 height, void* buffer)
{
	glFlush();
	glFinish();
	glReadPixels(0, 0, width, height, GL_BGR, GL_UNSIGNED_BYTE, buffer);
}

bool CGSH_OpenGL::IsBlendColorExtSupported()
{
	return glBlendColorEXT != NULL;
}

bool CGSH_OpenGL::IsBlendEquationExtSupported()
{
	return glBlendEquationEXT != NULL;
}

bool CGSH_OpenGL::IsFogCoordfExtSupported()
{
	return glFogCoordfEXT != NULL;
}

/////////////////////////////////////////////////////////////
// Framebuffer
/////////////////////////////////////////////////////////////

CGSH_OpenGL::CFramebuffer::CFramebuffer(uint32 basePtr, uint32 width, uint32 height, uint32 psm)
: m_basePtr(basePtr)
, m_width(width)
, m_height(height)
, m_psm(psm)
, m_framebuffer(0)
, m_texture(0)
{
	m_cachedArea.SetArea(psm, basePtr, width, height);

	//Build color attachment
	glGenTextures(1, &m_texture);
	glBindTexture(GL_TEXTURE_2D, m_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width * FBSCALE, m_height * FBSCALE, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	assert(glGetError() == GL_NO_ERROR);
		
	//Build framebuffer
	glGenFramebuffers(1, &m_framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
	glFramebufferTextureEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_texture, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

CGSH_OpenGL::CFramebuffer::~CFramebuffer()
{
	if(m_framebuffer != 0)
	{
		glDeleteFramebuffers(1, &m_framebuffer);
	}
	if(m_texture != 0)
	{
		glDeleteTextures(1, &m_texture);
	}
}

void CGSH_OpenGL::PopulateFramebuffer(const FramebufferPtr& framebuffer)
{
	if(framebuffer->m_psm != PSMCT32)
	{
		//In some cases we might not want to populate the framebuffer if its
		//pixel format isn't PSMCT32 since it will also change the pixel format of the
		//underlying OpenGL framebuffer (due to the call to glTexImage2D)
		return;
	}

	glBindTexture(GL_TEXTURE_2D, framebuffer->m_texture);
	((this)->*(m_textureUploader[framebuffer->m_psm]))(framebuffer->m_basePtr, 
		framebuffer->m_width / 64, framebuffer->m_width, framebuffer->m_height);
	assert(glGetError() == GL_NO_ERROR);
}

void CGSH_OpenGL::CommitFramebufferDirtyPages(const FramebufferPtr& framebuffer, unsigned int minY, unsigned int maxY)
{
	auto& cachedArea = framebuffer->m_cachedArea;

	if(cachedArea.HasDirtyPages())
	{
		glBindTexture(GL_TEXTURE_2D, framebuffer->m_texture);

		auto texturePageSize = CGsPixelFormats::GetPsmPageSize(framebuffer->m_psm);
		auto pageRect = cachedArea.GetPageRect();

		for(unsigned int dirtyPageIndex = 0; dirtyPageIndex < CGsCachedArea::MAX_DIRTYPAGES; dirtyPageIndex++)
		{
			if(!cachedArea.IsPageDirty(dirtyPageIndex)) continue;

			uint32 pageX = dirtyPageIndex % pageRect.first;
			uint32 pageY = dirtyPageIndex / pageRect.first;
			uint32 texX = pageX * texturePageSize.first;
			uint32 texY = pageY * texturePageSize.second;
			uint32 texWidth = texturePageSize.first;
			uint32 texHeight = texturePageSize.second;
			if(texX >= framebuffer->m_width) continue;
			if(texY >= framebuffer->m_height) continue;
			if(texY < minY) continue;
			if(texY >= maxY) continue;
			//assert(texX < tex0.GetWidth());
			//assert(texY < tex0.GetHeight());
			if((texX + texWidth) > framebuffer->m_width)
			{
				texWidth = framebuffer->m_width - texX;
			}
			if((texY + texHeight) > framebuffer->m_height)
			{
				texHeight = framebuffer->m_height - texY;
			}
			((this)->*(m_textureUpdater[framebuffer->m_psm]))(framebuffer->m_basePtr, framebuffer->m_width / 64, texX, texY, texWidth, texHeight);
		}

		//Mark all pages as clean, but might be wrong due to range not
		//covering an area that might be used later on
		cachedArea.ClearDirtyPages();
	}
}

/////////////////////////////////////////////////////////////
// Depthbuffer
/////////////////////////////////////////////////////////////

CGSH_OpenGL::CDepthbuffer::CDepthbuffer(uint32 basePtr, uint32 width, uint32 height, uint32 psm)
: m_basePtr(basePtr)
, m_width(width)
, m_height(height)
, m_psm(psm)
, m_depthBuffer(0)
{
	//Build depth attachment
	glGenRenderbuffers(1, &m_depthBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, m_depthBuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, m_width * FBSCALE, m_height * FBSCALE);
	assert(glGetError() == GL_NO_ERROR);
}

CGSH_OpenGL::CDepthbuffer::~CDepthbuffer()
{
	if(m_depthBuffer != 0)
	{
		glDeleteRenderbuffers(1, &m_depthBuffer);
	}
}
