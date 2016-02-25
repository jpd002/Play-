#include <stdio.h>
#include <assert.h>
#include <math.h>

#include "../../Log.h"
#include "../../AppConfig.h"
#include "../GsPixelFormats.h"
#include "GSH_OpenGL.h"

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

static uint32 MakeColor(uint8 r, uint8 g, uint8 b, uint8 a)
{
	return (a << 24) | (b << 16) | (g << 8) | (r);
}

CGSH_OpenGL::CGSH_OpenGL() 
: m_pCvtBuffer(nullptr)
{
	RegisterPreferences();
	LoadPreferences();

	m_pCvtBuffer = new uint8[CVTBUFFERSIZE];

	memset(&m_renderState, 0, sizeof(m_renderState));
	m_vertexBuffer.reserve(VERTEX_BUFFER_SIZE);
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
	m_validGlState = 0;
}

void CGSH_OpenGL::ReleaseImpl()
{
	ResetImpl();

	m_textureCache.clear();
	m_paletteCache.clear();
	m_shaders.clear();
	m_presentProgram.reset();
	m_presentVertexBuffer.Reset();
	m_presentVertexArray.Reset();
	m_primBuffer.Reset();
	m_primVertexArray.Reset();
	m_copyToFbTexture.Reset();
	m_copyToFbFramebuffer.Reset();
	m_vertexParamsBuffer.Reset();
	m_fragmentParamsBuffer.Reset();
}

void CGSH_OpenGL::ResetImpl()
{
	LoadPreferences();
	TexCache_Flush();
	PalCache_Flush();
	m_framebuffers.clear();
	m_depthbuffers.clear();
	m_vertexBuffer.clear();
	m_renderState.isValid = false;
	m_validGlState = 0;
	m_drawingToDepth = false;
}

void CGSH_OpenGL::FlipImpl()
{
	FlushVertexBuffer();
	m_renderState.isValid = false;
	m_validGlState = 0;

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
			(GetFramebufferBitDepth(candidateFramebuffer->m_psm) == GetFramebufferBitDepth(fb.nPSM)) &&
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
		framebuffer = FramebufferPtr(new CFramebuffer(fb.GetBufPtr(), fb.GetBufWidth(), 1024, fb.nPSM, m_fbScale));
		m_framebuffers.push_back(framebuffer);
		PopulateFramebuffer(framebuffer);
	}

	if(framebuffer)
	{
		CommitFramebufferDirtyPages(framebuffer, 0, dispHeight);
	}

	//Clear all of our output framebuffer
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_presentFramebuffer);

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

	if(framebuffer)
	{
		float u1 = static_cast<float>(dispWidth) / static_cast<float>(framebuffer->m_width);
		float v1 = static_cast<float>(dispHeight) / static_cast<float>(framebuffer->m_height);

		glDisable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, 0);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, framebuffer->m_texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glUseProgram(*m_presentProgram);

		assert(m_presentTextureUniform != -1);
		glUniform1i(m_presentTextureUniform, 0);

		assert(m_presentTexCoordScaleUniform != -1);
		glUniform2f(m_presentTexCoordScaleUniform, u1, v1);

		glBindBuffer(GL_ARRAY_BUFFER, m_presentVertexBuffer);
		glBindVertexArray(m_presentVertexArray);

#ifdef _DEBUG
		m_presentProgram->Validate();
#endif

		glDrawArrays(GL_TRIANGLES, 0, 3);
	}

	CHECKGLERROR();

	static bool g_dumpFramebuffers = false;
	if(g_dumpFramebuffers)
	{
		for(const auto& framebuffer : m_framebuffers)
		{
			glBindTexture(GL_TEXTURE_2D, framebuffer->m_texture);
			DumpTexture(framebuffer->m_width * m_fbScale, framebuffer->m_height * m_fbScale, framebuffer->m_basePtr);
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

void CGSH_OpenGL::RegisterPreferences()
{
	CGSHandler::RegisterPreferences();
	CAppConfig::GetInstance().RegisterPreferenceBoolean(PREF_CGSH_OPENGL_ENABLEHIGHRESMODE, false);
	CAppConfig::GetInstance().RegisterPreferenceBoolean(PREF_CGSH_OPENGL_FORCEBILINEARTEXTURES, false);
}

void CGSH_OpenGL::NotifyPreferencesChangedImpl()
{
	LoadPreferences();
	TexCache_Flush();
	PalCache_Flush();
	m_framebuffers.clear();
	m_depthbuffers.clear();
	CGSHandler::NotifyPreferencesChangedImpl();
}

void CGSH_OpenGL::LoadPreferences()
{
	m_fbScale = CAppConfig::GetInstance().GetPreferenceBoolean(PREF_CGSH_OPENGL_ENABLEHIGHRESMODE) ? 2 : 1;
	m_forceBilinearTextures = CAppConfig::GetInstance().GetPreferenceBoolean(PREF_CGSH_OPENGL_FORCEBILINEARTEXTURES);
}

void CGSH_OpenGL::InitializeRC()
{
	//Initialize basic stuff
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepthf(0.0f);

	SetupTextureUploaders();

	m_presentProgram = GeneratePresentProgram();
	m_presentVertexBuffer = GeneratePresentVertexBuffer();
	m_presentVertexArray = GeneratePresentVertexArray();
	m_presentTextureUniform = glGetUniformLocation(*m_presentProgram, "g_texture");
	m_presentTexCoordScaleUniform = glGetUniformLocation(*m_presentProgram, "g_texCoordScale");

	m_copyToFbTexture = Framework::OpenGl::CTexture::Create();
	m_copyToFbFramebuffer = Framework::OpenGl::CFramebuffer::Create();

	m_primBuffer = Framework::OpenGl::CBuffer::Create();
	m_primVertexArray = GeneratePrimVertexArray();

	m_vertexParamsBuffer = GenerateUniformBlockBuffer(sizeof(VERTEXPARAMS));
	m_fragmentParamsBuffer = GenerateUniformBlockBuffer(sizeof(FRAGMENTPARAMS));

	PresentBackbuffer();

	CHECKGLERROR();
}

Framework::OpenGl::CBuffer CGSH_OpenGL::GeneratePresentVertexBuffer()
{
	auto buffer = Framework::OpenGl::CBuffer::Create();

	static const float bufferContents[] =
	{
		//Pos         UV
		-1.0f, -1.0f, 0.0f,  1.0f,
		-1.0f,  3.0f, 0.0f, -1.0f,
		 3.0f, -1.0f, 2.0f,  1.0f,
	};

	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(bufferContents), bufferContents, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	CHECKGLERROR();

	return buffer;
}

Framework::OpenGl::CVertexArray CGSH_OpenGL::GeneratePresentVertexArray()
{
	auto vertexArray = Framework::OpenGl::CVertexArray::Create();

	glBindVertexArray(vertexArray);
	
	glBindBuffer(GL_ARRAY_BUFFER, m_presentVertexBuffer);

	glEnableVertexAttribArray(static_cast<GLuint>(PRIM_VERTEX_ATTRIB::POSITION));
	glVertexAttribPointer(static_cast<GLuint>(PRIM_VERTEX_ATTRIB::POSITION), 2, GL_FLOAT, 
		GL_FALSE, sizeof(float) * 4, reinterpret_cast<const GLvoid*>(0));

	glEnableVertexAttribArray(static_cast<GLuint>(PRIM_VERTEX_ATTRIB::TEXCOORD));
	glVertexAttribPointer(static_cast<GLuint>(PRIM_VERTEX_ATTRIB::TEXCOORD), 2, GL_FLOAT, 
		GL_FALSE, sizeof(float) * 4, reinterpret_cast<const GLvoid*>(8));

	glBindVertexArray(0);

	CHECKGLERROR();

	return vertexArray;
}

Framework::OpenGl::CVertexArray CGSH_OpenGL::GeneratePrimVertexArray()
{
	auto vertexArray = Framework::OpenGl::CVertexArray::Create();

	glBindVertexArray(vertexArray);

	glBindBuffer(GL_ARRAY_BUFFER, m_primBuffer);

	glEnableVertexAttribArray(static_cast<GLuint>(PRIM_VERTEX_ATTRIB::POSITION));
	glVertexAttribPointer(static_cast<GLuint>(PRIM_VERTEX_ATTRIB::POSITION), 3, GL_FLOAT, 
		GL_FALSE, sizeof(PRIM_VERTEX), reinterpret_cast<const GLvoid*>(offsetof(PRIM_VERTEX, x)));

	glEnableVertexAttribArray(static_cast<GLuint>(PRIM_VERTEX_ATTRIB::COLOR));
	glVertexAttribPointer(static_cast<GLuint>(PRIM_VERTEX_ATTRIB::COLOR), 4, GL_UNSIGNED_BYTE, 
		GL_TRUE, sizeof(PRIM_VERTEX), reinterpret_cast<const GLvoid*>(offsetof(PRIM_VERTEX, color)));

	glEnableVertexAttribArray(static_cast<GLuint>(PRIM_VERTEX_ATTRIB::TEXCOORD));
	glVertexAttribPointer(static_cast<GLuint>(PRIM_VERTEX_ATTRIB::TEXCOORD), 3, GL_FLOAT, 
		GL_FALSE, sizeof(PRIM_VERTEX), reinterpret_cast<const GLvoid*>(offsetof(PRIM_VERTEX, s)));

	glEnableVertexAttribArray(static_cast<GLuint>(PRIM_VERTEX_ATTRIB::FOG));
	glVertexAttribPointer(static_cast<GLuint>(PRIM_VERTEX_ATTRIB::FOG), 1, GL_FLOAT, 
		GL_FALSE, sizeof(PRIM_VERTEX), reinterpret_cast<const GLvoid*>(offsetof(PRIM_VERTEX, f)));

	glBindVertexArray(0);

	CHECKGLERROR();

	return vertexArray;
}

Framework::OpenGl::CBuffer CGSH_OpenGL::GenerateUniformBlockBuffer(size_t blockSize)
{
	auto uniformBlockBuffer = Framework::OpenGl::CBuffer::Create();

	glBindBuffer(GL_UNIFORM_BUFFER, uniformBlockBuffer);
	glBufferData(GL_UNIFORM_BUFFER, blockSize, nullptr, GL_STREAM_DRAW);
	CHECKGLERROR();

	return uniformBlockBuffer;
}

void CGSH_OpenGL::MakeLinearZOrtho(float* matrix, float left, float right, float bottom, float top)
{
	matrix[ 0] = 2.0f / (right - left);
	matrix[ 1] = 0;
	matrix[ 2] = 0;
	matrix[ 3] = 0;

	matrix[ 4] = 0;
	matrix[ 5] = 2.0f / (top - bottom);
	matrix[ 6] = 0;
	matrix[ 7] = 0;

	matrix[ 8] = 0;
	matrix[ 9] = 0;
	matrix[10] = 1;
	matrix[11] = 0;

	matrix[12] = - (right + left) / (right - left);
	matrix[13] = - (top + bottom) / (top - bottom);
	matrix[14] = 0;
	matrix[15] = 1;
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

float CGSH_OpenGL::GetZ(float nZ)
{
	if(nZ == 0)
	{
		return -1;
	}
	
	nZ -= m_nMaxZ;
	if(nZ > m_nMaxZ) return 1.0;
	if(nZ < -m_nMaxZ) return -1.0;
	return nZ / m_nMaxZ;
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
	uint64 fogColReg = m_nReg[GS_REG_FOGCOL];
	uint64 scissorReg = m_nReg[GS_REG_SCISSOR_1 + context];

	//--------------------------------------------------------
	//Get shader caps
	//--------------------------------------------------------

	SHADERCAPS shaderCaps;
	memset(&shaderCaps, 0, sizeof(SHADERCAPS));

	FillShaderCapsFromTexture(shaderCaps, tex0Reg, tex1Reg, texAReg, clampReg);
	FillShaderCapsFromTest(shaderCaps, testReg);

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

	auto shaderIterator = m_shaders.find(static_cast<uint32>(shaderCaps));
	if(shaderIterator == m_shaders.end())
	{
		auto shader = GenerateShader(shaderCaps);

		glUseProgram(*shader);
		m_validGlState &= ~GLSTATE_PROGRAM;

		auto textureUniform = glGetUniformLocation(*shader, "g_texture");
		if(textureUniform != -1)
		{
			glUniform1i(textureUniform, 0);
		}

		auto paletteUniform = glGetUniformLocation(*shader, "g_palette");
		if(paletteUniform != -1)
		{
			glUniform1i(paletteUniform, 1);
		}

		auto vertexParamsUniformBlock = glGetUniformBlockIndex(*shader, "VertexParams");
		if(vertexParamsUniformBlock != GL_INVALID_INDEX)
		{
			glUniformBlockBinding(*shader, vertexParamsUniformBlock, 0);
		}

		auto fragmentParamsUniformBlock = glGetUniformBlockIndex(*shader, "FragmentParams");
		if(fragmentParamsUniformBlock != GL_INVALID_INDEX)
		{
			glUniformBlockBinding(*shader, fragmentParamsUniformBlock, 1);
		}

		CHECKGLERROR();

		m_shaders.insert(std::make_pair(static_cast<uint32>(shaderCaps), shader));
		shaderIterator = m_shaders.find(static_cast<uint32>(shaderCaps));
	}

	//--------------------------------------------------------
	//Bind shader
	//--------------------------------------------------------

	GLuint shaderHandle = *shaderIterator->second;
	if(!m_renderState.isValid ||
		(m_renderState.shaderHandle != shaderHandle))
	{
		FlushVertexBuffer();

		//Invalidate because we might need to set uniforms again
		m_renderState.isValid = false;
	}

	//--------------------------------------------------------
	//Set render states
	//--------------------------------------------------------

	if(!m_renderState.isValid ||
		(m_renderState.primReg != primReg))
	{
		FlushVertexBuffer();

		//Humm, not quite sure about this
//		if(prim.nAntiAliasing)
//		{
//			glEnable(GL_BLEND);
//		}
//		else
//		{
//			glDisable(GL_BLEND);
//		}

		m_renderState.blendEnabled = prim.nAlpha ? GL_TRUE : GL_FALSE;
		m_validGlState &= ~GLSTATE_BLEND;
	}

	if(!m_renderState.isValid ||
		(m_renderState.alphaReg != alphaReg))
	{
		FlushVertexBuffer();
		SetupBlendingFunction(alphaReg);
		CHECKGLERROR();
	}

	if(!m_renderState.isValid ||
		(m_renderState.testReg != testReg))
	{
		FlushVertexBuffer();
		SetupTestFunctions(testReg);
		CHECKGLERROR();
	}

	if(!m_renderState.isValid ||
		(m_renderState.zbufReg != zbufReg) ||
		(m_renderState.testReg != testReg))
	{
		FlushVertexBuffer();
		SetupDepthBuffer(zbufReg, testReg);
		CHECKGLERROR();
	}

	if(!m_renderState.isValid ||
		(m_renderState.frameReg != frameReg) ||
		(m_renderState.zbufReg != zbufReg) ||
		(m_renderState.scissorReg != scissorReg) ||
		(m_renderState.testReg != testReg))
	{
		FlushVertexBuffer();
		SetupFramebuffer(frameReg, zbufReg, scissorReg, testReg);
		CHECKGLERROR();
	}

	if(!m_renderState.isValid ||
		(m_renderState.tex0Reg != tex0Reg) ||
		(m_renderState.tex1Reg != tex1Reg) ||
		(m_renderState.texAReg != texAReg) ||
		(m_renderState.clampReg != clampReg) ||
		(m_renderState.primReg != primReg))
	{
		FlushVertexBuffer();
		SetupTexture(primReg, tex0Reg, tex1Reg, texAReg, clampReg);
		CHECKGLERROR();
	}

	if(shaderCaps.hasFog &&
		(
			!m_renderState.isValid ||
			(m_renderState.fogColReg != fogColReg)
		))
	{
		FlushVertexBuffer();
		SetupFogColor(fogColReg);
		CHECKGLERROR();
	}

	auto offset = make_convertible<XYOFFSET>(m_nReg[GS_REG_XYOFFSET_1 + context]);
	m_nPrimOfsX = offset.GetX();
	m_nPrimOfsY = offset.GetY();
	
	CHECKGLERROR();

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
	m_renderState.fogColReg = fogColReg;
}

void CGSH_OpenGL::SetupBlendingFunction(uint64 alphaReg)
{
	int nFunction = GL_FUNC_ADD;
	auto alpha = make_convertible<ALPHA>(alphaReg);

	if((alpha.nA == alpha.nB) && (alpha.nD == ALPHABLEND_ABD_CS))
	{
		//ab*0 (when a == b) - Cs
		glBlendFuncSeparate(GL_ONE, GL_ZERO, GL_ONE, GL_ZERO);
	}
	else if((alpha.nA == alpha.nB) && (alpha.nD == ALPHABLEND_ABD_CD))
	{
		//ab*1 (when a == b) - Cd
		glBlendFuncSeparate(GL_ZERO, GL_ONE, GL_ONE, GL_ZERO);
	}
	else if((alpha.nA == alpha.nB) && (alpha.nD == ALPHABLEND_ABD_ZERO))
	{
		//ab*2 (when a == b) - Zero
		glBlendFuncSeparate(GL_ZERO, GL_ZERO, GL_ONE, GL_ZERO);
	}
	else if((alpha.nA == ALPHABLEND_ABD_CS) && (alpha.nB == ALPHABLEND_ABD_CD) && (alpha.nC == ALPHABLEND_C_AS) && (alpha.nD == ALPHABLEND_ABD_CD))
	{
		//0101 - Cs * As + Cd * (1 - As)
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
			glBlendColor(0.0f, 0.0f, 0.0f, (float)alpha.nFix / 128.0f);
			glBlendFuncSeparate(GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA, GL_ONE, GL_ZERO);
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
			glBlendColor(0, 0, 0, static_cast<float>(alpha.nFix) / 128.0f);
			glBlendFuncSeparate(GL_CONSTANT_ALPHA, GL_ONE, GL_ONE, GL_ZERO);
		}
	}
	else if((alpha.nA == 1) && (alpha.nB == 0) && (alpha.nC == 0) && (alpha.nD == 0))
	{
		//(Cd - Cs) * As + Cs
		glBlendFuncSeparate(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE, GL_ZERO);
	}
	else if((alpha.nA == 1) && (alpha.nB == 0) && (alpha.nC == 2) && (alpha.nD == 2))
	{
		nFunction = GL_FUNC_REVERSE_SUBTRACT;
		glBlendColor(0.0f, 0.0f, 0.0f, (float)alpha.nFix / 128.0f);
		glBlendFuncSeparate(GL_CONSTANT_ALPHA, GL_CONSTANT_ALPHA, GL_ONE, GL_ZERO);
	}
	else if((alpha.nA == 1) && (alpha.nB == 2) && (alpha.nC == 0) && (alpha.nD == 0))
	{
		//Cd * As + Cs
		glBlendFuncSeparate(GL_ONE, GL_SRC_ALPHA, GL_ONE, GL_ZERO);
	}
	else if((alpha.nA == ALPHABLEND_ABD_CD) && (alpha.nB == ALPHABLEND_ABD_ZERO) && (alpha.nC == ALPHABLEND_C_AS) && (alpha.nD == ALPHABLEND_ABD_CD))
	{
		//1201 -> Cd * (As + 1)
		//TODO: Implement this properly (multiple passes?)
		glBlendFuncSeparate(GL_ZERO, GL_ONE, GL_ONE, GL_ZERO);
	}
	else if((alpha.nA == ALPHABLEND_ABD_CD) && (alpha.nB == ALPHABLEND_ABD_ZERO) && (alpha.nC == ALPHABLEND_C_AS) && (alpha.nD == ALPHABLEND_ABD_ZERO))
	{
		//1202 - Cd * As
		glBlendFuncSeparate(GL_ZERO, GL_SRC_ALPHA, GL_ONE, GL_ZERO);
	}
	else if((alpha.nA == ALPHABLEND_ABD_CD) && (alpha.nB == ALPHABLEND_ABD_ZERO) && (alpha.nC == ALPHABLEND_C_FIX) && (alpha.nD == ALPHABLEND_ABD_CS))
	{
		//1220 -> Cd * FIX + Cs
		glBlendColor(0, 0, 0, static_cast<float>(alpha.nFix) / 128.0f);
		glBlendFuncSeparate(GL_ONE, GL_CONSTANT_ALPHA, GL_ONE, GL_ZERO);
	}
	else if((alpha.nA == 1) && (alpha.nB == 2) && (alpha.nC == 2) && (alpha.nD == 2))
	{
		//Cd * FIX
		glBlendColor(0, 0, 0, static_cast<float>(alpha.nFix) / 128.0f);
		glBlendFuncSeparate(GL_ZERO, GL_CONSTANT_ALPHA, GL_ONE, GL_ZERO);
	}
	else if((alpha.nA == 2) && (alpha.nB == 0) && (alpha.nC == 0) && (alpha.nD == 1))
	{
		nFunction = GL_FUNC_REVERSE_SUBTRACT;
		glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_ONE, GL_ZERO);
	}
	else if((alpha.nA == ALPHABLEND_ABD_ZERO) && (alpha.nB == ALPHABLEND_ABD_CS) && (alpha.nC == ALPHABLEND_C_FIX) && (alpha.nD == ALPHABLEND_ABD_CD))
	{
		//2021 -> Cd - Cs * FIX
		nFunction = GL_FUNC_REVERSE_SUBTRACT;
		glBlendColor(0, 0, 0, static_cast<float>(alpha.nFix) / 128.0f);
		glBlendFuncSeparate(GL_CONSTANT_ALPHA, GL_ONE, GL_ONE, GL_ZERO);
	}
	else if((alpha.nA == ALPHABLEND_ABD_ZERO) && (alpha.nB == ALPHABLEND_ABD_CD) && (alpha.nC == ALPHABLEND_C_AS) && (alpha.nD == ALPHABLEND_ABD_CD))
	{
		//2101 -> Cd * (1 - As)
		glBlendFuncSeparate(GL_ZERO, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);
	}
	else
	{
		assert(0);
		//Default blending
		glBlendFuncSeparate(GL_ONE, GL_ZERO, GL_ONE, GL_ZERO);
	}

	glBlendEquation(nFunction);
}

void CGSH_OpenGL::SetupTestFunctions(uint64 testReg)
{
	auto test = make_convertible<TEST>(testReg);

	m_fragmentParams.alphaRef = static_cast<float>(test.nAlphaRef) / 255.0f;
	m_validGlState &= GLSTATE_FRAGMENT_PARAMS;

	if(test.nDepthEnabled)
	{
		unsigned int nFunc = GL_NEVER;

		switch(test.nDepthMethod)
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
	if(
		(test.nAlphaEnabled == 1) && 
		(test.nAlphaMethod == ALPHA_TEST_NEVER) && 
		((test.nAlphaFail == ALPHA_TEST_FAIL_FBONLY) || (test.nAlphaFail == ALPHA_TEST_FAIL_RGBONLY)))
	{
		depthWriteEnabled = false;
	}
	glDepthMask(depthWriteEnabled ? GL_TRUE : GL_FALSE);
}

void CGSH_OpenGL::SetupFramebuffer(uint64 frameReg, uint64 zbufReg, uint64 scissorReg, uint64 testReg)
{
	if(frameReg == 0) return;

	auto frame = make_convertible<FRAME>(frameReg);
	auto zbuf = make_convertible<ZBUF>(zbufReg);
	auto scissor = make_convertible<SCISSOR>(scissorReg);
	auto test = make_convertible<TEST>(testReg);

	bool r = (frame.nMask & 0x000000FF) == 0;
	bool g = (frame.nMask & 0x0000FF00) == 0;
	bool b = (frame.nMask & 0x00FF0000) == 0;
	bool a = (frame.nMask & 0xFF000000) == 0;

	if((test.nAlphaEnabled == 1) && (test.nAlphaMethod == ALPHA_TEST_NEVER))
	{
		if(test.nAlphaFail == ALPHA_TEST_FAIL_RGBONLY)
		{
			a = false;
		}
		else if(test.nAlphaFail == ALPHA_TEST_FAIL_ZBONLY)
		{
			r = g = b = a = false;
		}
	}

	m_renderState.colorMaskR = r;
	m_renderState.colorMaskG = g;
	m_renderState.colorMaskB = b;
	m_renderState.colorMaskA = a;
	m_validGlState &= ~GLSTATE_COLORMASK;

	//Check if we're drawing into a buffer that's been used for depth before
	{
		auto zbufWrite = make_convertible<ZBUF>(frameReg);
		auto depthbuffer = FindDepthbuffer(zbufWrite, frame);
		m_drawingToDepth = (depthbuffer != nullptr);
	}

	//Look for a framebuffer that matches the specified information
	auto framebuffer = FindFramebuffer(frame);
	if(!framebuffer)
	{
		framebuffer = FramebufferPtr(new CFramebuffer(frame.GetBasePtr(), frame.GetWidth(), 1024, frame.nPsm, m_fbScale));
		m_framebuffers.push_back(framebuffer);
		PopulateFramebuffer(framebuffer);
	}

	CommitFramebufferDirtyPages(framebuffer, scissor.scay0, scissor.scay1);

	auto depthbuffer = FindDepthbuffer(zbuf, frame);
	if(!depthbuffer)
	{
		depthbuffer = DepthbufferPtr(new CDepthbuffer(zbuf.GetBasePtr(), frame.GetWidth(), 1024, zbuf.nPsm, m_fbScale));
		m_depthbuffers.push_back(depthbuffer);
	}

	assert(framebuffer->m_width == depthbuffer->m_width);

	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer->m_framebuffer);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthbuffer->m_depthBuffer);
	CHECKGLERROR();

	GLenum result = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	assert(result == GL_FRAMEBUFFER_COMPLETE);

	m_renderState.framebufferHandle = framebuffer->m_framebuffer;
	m_validGlState &= ~GLSTATE_FRAMEBUFFER;

	{
		GLenum drawBufferId = GL_COLOR_ATTACHMENT0;
		glDrawBuffers(1, &drawBufferId);
		CHECKGLERROR();
	}

	glViewport(0, 0, framebuffer->m_width * m_fbScale, framebuffer->m_height * m_fbScale);
	
	float projWidth = static_cast<float>(framebuffer->m_width);
	float projHeight = static_cast<float>(framebuffer->m_height);

	MakeLinearZOrtho(m_vertexParams.projMatrix, 0, projWidth, 0, projHeight);
	m_validGlState &= ~GLSTATE_VERTEX_PARAMS;

	m_renderState.scissorX = scissor.scax0;
	m_renderState.scissorY = scissor.scay0;
	m_renderState.scissorWidth = scissor.scax1 - scissor.scax0 + 1;
	m_renderState.scissorHeight = scissor.scay1 - scissor.scay0 + 1;
	m_validGlState &= ~GLSTATE_SCISSOR;
}

void CGSH_OpenGL::SetupFogColor(uint64 fogColReg)
{
	auto fogCol = make_convertible<FOGCOL>(fogColReg);
	m_fragmentParams.fogColor[0] = static_cast<float>(fogCol.nFCR) / 255.0f;
	m_fragmentParams.fogColor[1] = static_cast<float>(fogCol.nFCG) / 255.0f;
	m_fragmentParams.fogColor[2] = static_cast<float>(fogCol.nFCB) / 255.0f;
	m_validGlState &= ~GLSTATE_FRAGMENT_PARAMS;
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

void CGSH_OpenGL::FillShaderCapsFromTexture(SHADERCAPS& shaderCaps, const uint64& tex0Reg, const uint64& tex1Reg, const uint64& texAReg, const uint64& clampReg)
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

	if(CGsPixelFormats::IsPsmIDTEX(tex0.nPsm))
	{
		if((tex1.nMinFilter != MIN_FILTER_NEAREST) || (tex1.nMagFilter != MIN_FILTER_NEAREST))
		{
			//We'll need to filter the texture manually
			shaderCaps.texBilinearFilter = 1;
		}

		if(m_forceBilinearTextures)
		{
			shaderCaps.texBilinearFilter = 1;
		}
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

void CGSH_OpenGL::FillShaderCapsFromTest(SHADERCAPS& shaderCaps, const uint64& testReg)
{
	auto test = make_convertible<TEST>(testReg);

	if(test.nAlphaEnabled)
	{
		//Handle special way of turning off color or depth writes
		//Proper write masks will be set at other places
		if((test.nAlphaMethod == ALPHA_TEST_NEVER) && (test.nAlphaFail != ALPHA_TEST_FAIL_KEEP))
		{
			shaderCaps.hasAlphaTest = 0;
		}
		else
		{
			shaderCaps.hasAlphaTest = 1;
			shaderCaps.alphaTestMethod = test.nAlphaMethod;
		}
	}
	else
	{
		shaderCaps.hasAlphaTest = 0;
	}
}

void CGSH_OpenGL::SetupTexture(uint64 primReg, uint64 tex0Reg, uint64 tex1Reg, uint64 texAReg, uint64 clampReg)
{
	m_renderState.texture0Handle = 0;
	m_renderState.texture1Handle = 0;
	m_renderState.texture0MinFilter = GL_NEAREST;
	m_renderState.texture0MagFilter = GL_NEAREST;
	m_renderState.texture0WrapS = GL_CLAMP_TO_EDGE;
	m_renderState.texture0WrapT = GL_CLAMP_TO_EDGE;
	m_validGlState &= ~GLSTATE_TEXTURE;

	auto prim = make_convertible<PRMODE>(primReg);

	if(tex0Reg == 0 || prim.nTexture == 0)
	{
		return;
	}

	auto tex0 = make_convertible<TEX0>(tex0Reg);
	auto tex1 = make_convertible<TEX1>(tex1Reg);
	auto texA = make_convertible<TEXA>(texAReg);
	auto clamp = make_convertible<CLAMP>(clampReg);

	m_nTexWidth = tex0.GetWidth();
	m_nTexHeight = tex0.GetHeight();

	auto texInfo = PrepareTexture(tex0);
	m_renderState.texture0Handle = texInfo.textureHandle;

	//Setup sampling modes
	switch(tex1.nMagFilter)
	{
	case MAG_FILTER_NEAREST:
		m_renderState.texture0MagFilter = GL_NEAREST;
		break;
	case MAG_FILTER_LINEAR:
		m_renderState.texture0MagFilter = GL_LINEAR;
		break;
	}

	switch(tex1.nMinFilter)
	{
	case MIN_FILTER_NEAREST:
		m_renderState.texture0MinFilter = GL_NEAREST;
		break;
	case MIN_FILTER_LINEAR:
		m_renderState.texture0MinFilter = GL_LINEAR;
		break;
	case MIN_FILTER_LINEAR_MIP_LINEAR:
		m_renderState.texture0MinFilter = GL_LINEAR_MIPMAP_LINEAR;
		break;
	default:
		assert(0);
		break;
	}

	if(m_forceBilinearTextures)
	{
		m_renderState.texture0MagFilter = GL_LINEAR;
		m_renderState.texture0MinFilter = GL_LINEAR;
	}

	unsigned int clampMin[2] = { 0, 0 };
	unsigned int clampMax[2] = { 0, 0 };
	float textureScaleRatio[2] = { texInfo.scaleRatioX, texInfo.scaleRatioY };
	m_renderState.texture0WrapS = g_nativeClampModes[clamp.nWMS];
	m_renderState.texture0WrapT = g_nativeClampModes[clamp.nWMT];

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

	if(CGsPixelFormats::IsPsmIDTEX(tex0.nPsm) && 
		(m_renderState.texture0MinFilter != GL_NEAREST || m_renderState.texture0MagFilter != GL_NEAREST))
	{
		//We'll need to filter the texture manually
		m_renderState.texture0MinFilter = GL_NEAREST;
		m_renderState.texture0MagFilter = GL_NEAREST;
	}

	if(CGsPixelFormats::IsPsmIDTEX(tex0.nPsm))
	{
		m_renderState.texture1Handle = PreparePalette(tex0);
	}

	memset(m_vertexParams.texMatrix, 0, sizeof(m_vertexParams.texMatrix));
	m_vertexParams.texMatrix[0 + (0 * 4)] = texInfo.scaleRatioX;
	m_vertexParams.texMatrix[1 + (1 * 4)] = texInfo.scaleRatioY;
	m_vertexParams.texMatrix[2 + (2 * 4)] = 1;
	m_vertexParams.texMatrix[0 + (3 * 4)] = texInfo.offsetX;
	m_vertexParams.texMatrix[3 + (3 * 4)] = 1;
	m_validGlState &= ~GLSTATE_VERTEX_PARAMS;

	m_fragmentParams.textureSize[0] = static_cast<float>(tex0.GetWidth());
	m_fragmentParams.textureSize[1] = static_cast<float>(tex0.GetHeight());
	m_fragmentParams.texelSize[0] = 1.0f / static_cast<float>(tex0.GetWidth());
	m_fragmentParams.texelSize[1] = 1.0f / static_cast<float>(tex0.GetHeight());
	m_fragmentParams.clampMin[0] = static_cast<float>(clampMin[0]);
	m_fragmentParams.clampMin[1] = static_cast<float>(clampMin[1]);
	m_fragmentParams.clampMax[0] = static_cast<float>(clampMax[0]);
	m_fragmentParams.clampMax[1] = static_cast<float>(clampMax[1]);
	m_fragmentParams.texA0 = static_cast<float>(texA.nTA0) / 255.f;
	m_fragmentParams.texA1 = static_cast<float>(texA.nTA1) / 255.f;
	m_validGlState &= ~GLSTATE_FRAGMENT_PARAMS;
}

CGSH_OpenGL::FramebufferPtr CGSH_OpenGL::FindFramebuffer(const FRAME& frame) const
{
	auto framebufferIterator = std::find_if(std::begin(m_framebuffers), std::end(m_framebuffers), 
		[&] (const FramebufferPtr& framebuffer)
		{
			return (framebuffer->m_basePtr == frame.GetBasePtr()) &&
				(framebuffer->m_psm == frame.nPsm) &&
				(framebuffer->m_width == frame.GetWidth());
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
	auto xyz = make_convertible<XYZ>(m_VtxBuffer[0].nPosition);
	auto rgbaq = make_convertible<RGBAQ>(m_VtxBuffer[0].nRGBAQ);

	float x = xyz.GetX(); float y = xyz.GetY(); float z = xyz.GetZ();

	x -= m_nPrimOfsX;
	y -= m_nPrimOfsY;
	z = GetZ(z);

	auto color = MakeColor(
		rgbaq.nR, rgbaq.nG,
		rgbaq.nB, rgbaq.nA);

	PRIM_VERTEX vertex =
	{
		//x, y, z, color, s, t, q, f
		  x, y, z, color, 0, 0, 1, 0,
	};

	assert((m_vertexBuffer.size() + 1) <= VERTEX_BUFFER_SIZE);
	m_vertexBuffer.push_back(vertex);
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

	nZ1 = GetZ(nZ1);
	nZ2 = GetZ(nZ2);

	RGBAQ rgbaq[2];
	rgbaq[0] <<= m_VtxBuffer[1].nRGBAQ;
	rgbaq[1] <<= m_VtxBuffer[0].nRGBAQ;

	float nS[2] = { 0 ,0 };
	float nT[2] = { 0, 0 };
	float nQ[2] = { 1, 1 };

	auto color1 = MakeColor(
		rgbaq[0].nR, rgbaq[0].nG,
		rgbaq[0].nB, rgbaq[0].nA);

	auto color2 = MakeColor(
		rgbaq[1].nR, rgbaq[1].nG,
		rgbaq[1].nB, rgbaq[1].nA);

	PRIM_VERTEX vertices[] =
	{
		{	nX1,	nY1,	nZ1,	color1,	nS[0],	nT[0],	nQ[0],	0	},
		{	nX2,	nY2,	nZ2,	color2,	nS[1],	nT[1],	nQ[1],	0	},
	};

	assert((m_vertexBuffer.size() + 2) <= VERTEX_BUFFER_SIZE);
	m_vertexBuffer.insert(m_vertexBuffer.end(), std::begin(vertices), std::end(vertices));
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

	float nS[3] = { 0 ,0, 0 };
	float nT[3] = { 0, 0, 0 };
	float nQ[3] = { 1, 1, 1 };

	if(m_PrimitiveMode.nTexture)
	{
		if(m_PrimitiveMode.nUseUV)
		{
			UV uv[3];
			uv[0] <<= m_VtxBuffer[2].nUV;
			uv[1] <<= m_VtxBuffer[1].nUV;
			uv[2] <<= m_VtxBuffer[0].nUV;

			nS[0] = uv[0].GetU() / static_cast<float>(m_nTexWidth);
			nS[1] = uv[1].GetU() / static_cast<float>(m_nTexWidth);
			nS[2] = uv[2].GetU() / static_cast<float>(m_nTexWidth);

			nT[0] = uv[0].GetV() / static_cast<float>(m_nTexHeight);
			nT[1] = uv[1].GetV() / static_cast<float>(m_nTexHeight);
			nT[2] = uv[2].GetV() / static_cast<float>(m_nTexHeight);
		}
		else
		{
			ST st[3];
			st[0] <<= m_VtxBuffer[2].nST;
			st[1] <<= m_VtxBuffer[1].nST;
			st[2] <<= m_VtxBuffer[0].nST;

			nS[0] = st[0].nS; nS[1] = st[1].nS; nS[2] = st[2].nS;
			nT[0] = st[0].nT; nT[1] = st[1].nT; nT[2] = st[2].nT;

			bool isQ0Neg = (rgbaq[0].nQ < 0);
			bool isQ1Neg = (rgbaq[1].nQ < 0);
			bool isQ2Neg = (rgbaq[2].nQ < 0);

			assert(isQ0Neg == isQ1Neg);
			assert(isQ1Neg == isQ2Neg);

			nQ[0] = rgbaq[0].nQ; nQ[1] = rgbaq[1].nQ; nQ[2] = rgbaq[2].nQ;
		}
	}

	auto color1 = MakeColor(
		rgbaq[0].nR, rgbaq[0].nG,
		rgbaq[0].nB, rgbaq[0].nA);

	auto color2 = MakeColor(
		rgbaq[1].nR, rgbaq[1].nG,
		rgbaq[1].nB, rgbaq[1].nA);

	auto color3 = MakeColor(
		rgbaq[2].nR, rgbaq[2].nG,
		rgbaq[2].nB, rgbaq[2].nA);

	PRIM_VERTEX vertices[] =
	{
		{	nX1,	nY1,	nZ1,	color1,	nS[0],	nT[0],	nQ[0],	nF1	},
		{	nX2,	nY2,	nZ2,	color2,	nS[1],	nT[1],	nQ[1],	nF2	},
		{	nX3,	nY3,	nZ3,	color3,	nS[2],	nT[2],	nQ[2],	nF3	},
	};

	assert((m_vertexBuffer.size() + 3) <= VERTEX_BUFFER_SIZE);
	m_vertexBuffer.insert(m_vertexBuffer.end(), std::begin(vertices), std::end(vertices));
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

	float nS[2] = { 0 ,0 };
	float nT[2] = { 0, 0 };

	if(m_PrimitiveMode.nTexture)
	{
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
	}

	auto color = MakeColor(
		rgbaq[0].nR, rgbaq[0].nG,
		rgbaq[0].nB, rgbaq[0].nA);

	PRIM_VERTEX vertices[] =
	{
		{	nX1,	nY1,	nZ,	color,	nS[0],	nT[0],	1,	0	},
		{	nX2,	nY1,	nZ,	color,	nS[1],	nT[0],	1,	0	},
		{	nX1,	nY2,	nZ,	color,	nS[0],	nT[1],	1,	0	},

		{	nX1,	nY2,	nZ,	color,	nS[0],	nT[1],	1,	0	},
		{	nX2,	nY1,	nZ,	color,	nS[1],	nT[0],	1,	0	},
		{	nX2,	nY2,	nZ,	color,	nS[1],	nT[1],	1,	0	},
	};

	assert((m_vertexBuffer.size() + 6) <= VERTEX_BUFFER_SIZE);
	m_vertexBuffer.insert(m_vertexBuffer.end(), std::begin(vertices), std::end(vertices));
}

void CGSH_OpenGL::FlushVertexBuffer()
{
	if(m_vertexBuffer.empty()) return;

	assert(m_renderState.isValid == true);

	if((m_validGlState & GLSTATE_VERTEX_PARAMS) == 0)
	{
		glBindBuffer(GL_UNIFORM_BUFFER, m_vertexParamsBuffer);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(VERTEXPARAMS), &m_vertexParams);
		CHECKGLERROR();
		m_validGlState |= GLSTATE_VERTEX_PARAMS;
	}

	if((m_validGlState & GLSTATE_FRAGMENT_PARAMS) == 0)
	{
		glBindBuffer(GL_UNIFORM_BUFFER, m_fragmentParamsBuffer);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(FRAGMENTPARAMS), &m_fragmentParams);
		CHECKGLERROR();
		m_validGlState |= GLSTATE_FRAGMENT_PARAMS;
	}

	if((m_validGlState & GLSTATE_PROGRAM) == 0)
	{
		glUseProgram(m_renderState.shaderHandle);
		m_validGlState |= GLSTATE_PROGRAM;
	}

	if((m_validGlState & GLSTATE_SCISSOR) == 0)
	{
		glEnable(GL_SCISSOR_TEST);
		glScissor(m_renderState.scissorX * m_fbScale, m_renderState.scissorY * m_fbScale,
			m_renderState.scissorWidth * m_fbScale, m_renderState.scissorHeight * m_fbScale);
		m_validGlState |= GLSTATE_SCISSOR;
	}

	if((m_validGlState & GLSTATE_BLEND) == 0)
	{
		m_renderState.blendEnabled ? glEnable(GL_BLEND) : glDisable(GL_BLEND);
		m_validGlState |= GLSTATE_BLEND;
	}

	if((m_validGlState & GLSTATE_COLORMASK) == 0)
	{
		glColorMask(
			m_renderState.colorMaskR, m_renderState.colorMaskG,
			m_renderState.colorMaskB, m_renderState.colorMaskA);
		m_validGlState |= GLSTATE_COLORMASK;
	}

	if((m_validGlState & GLSTATE_TEXTURE) == 0)
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_renderState.texture0Handle);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, m_renderState.texture0MinFilter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, m_renderState.texture0MagFilter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, m_renderState.texture0WrapS);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, m_renderState.texture0WrapT);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, m_renderState.texture1Handle);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		m_validGlState |= GLSTATE_TEXTURE;
	}

	if((m_validGlState & GLSTATE_FRAMEBUFFER) == 0)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_renderState.framebufferHandle);
		m_validGlState |= GLSTATE_FRAMEBUFFER;
	}

	glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_vertexParamsBuffer);
	glBindBufferBase(GL_UNIFORM_BUFFER, 1, m_fragmentParamsBuffer);

	glBindBuffer(GL_ARRAY_BUFFER, m_primBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(PRIM_VERTEX) * m_vertexBuffer.size(), m_vertexBuffer.data(), GL_STREAM_DRAW);

	glBindVertexArray(m_primVertexArray);

	GLenum primitiveMode = GL_NONE;
	switch(m_primitiveType)
	{
	case PRIM_POINT:
		primitiveMode = GL_POINTS;
		break;
	case PRIM_LINE:
	case PRIM_LINESTRIP:
		primitiveMode = GL_LINES;
		break;
	case PRIM_TRIANGLE:
	case PRIM_TRIANGLESTRIP:
	case PRIM_TRIANGLEFAN:
	case PRIM_SPRITE:
		primitiveMode = GL_TRIANGLES;
		break;
	default:
		assert(false);
		break;
	}

	glDrawArrays(primitiveMode, 0, m_vertexBuffer.size());

	m_vertexBuffer.clear();

	m_drawCallCount++;
}

void CGSH_OpenGL::DrawToDepth(unsigned int primitiveType, uint64 primReg)
{
	//A game might be attempting to clear depth by using the zbuffer
	//as a frame buffer and drawing a black sprite into it
	//Space Harrier does this

	//Must be flat, no texture map, no fog, no blend and no aa
	if((primReg & 0x1F8) != 0) return;

	//Must be a sprite
	if(primitiveType != PRIM_SPRITE) return;

	//Invalidate state
	FlushVertexBuffer();
	m_renderState.isValid = false;

	auto prim = make_convertible<PRMODE>(primReg);

	uint64 frameReg = m_nReg[GS_REG_FRAME_1 + prim.nContext];

	auto frame = make_convertible<FRAME>(frameReg);
	auto zbufWrite = make_convertible<ZBUF>(frameReg);

	auto depthbuffer = FindDepthbuffer(zbufWrite, frame);
	assert(depthbuffer);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthbuffer->m_depthBuffer);
	CHECKGLERROR();

	GLenum result = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	assert(result == GL_FRAMEBUFFER_COMPLETE);

	glDepthMask(GL_TRUE);
	glClearDepthf(0);
	glClear(GL_DEPTH_BUFFER_BIT);
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
		{
			unsigned int newPrimitiveType = static_cast<unsigned int>(nData & 0x07);
			if(newPrimitiveType != m_primitiveType)
			{
				FlushVertexBuffer();
			}
			m_primitiveType = newPrimitiveType;
			switch(m_primitiveType)
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
		}
		break;

	case GS_REG_XYZ2:
	case GS_REG_XYZ3:
	case GS_REG_XYZF2:
	case GS_REG_XYZF3:
		VertexKick(nRegister, nData);
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
		}

		switch(m_primitiveType)
		{
		case PRIM_POINT:
			if(nDrawingKick) Prim_Point();
			m_nVtxCount = 1;
			break;
		case PRIM_LINE:
			if(nDrawingKick) Prim_Line();
			m_nVtxCount = 2;
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

		if(nDrawingKick && m_drawingToDepth)
		{
			DrawToDepth(m_primitiveType, m_PrimitiveMode);
		}
	}
}

void CGSH_OpenGL::ProcessHostToLocalTransfer()
{
	auto bltBuf = make_convertible<BITBLTBUF>(m_nReg[GS_REG_BITBLTBUF]);
	uint32 transferAddress = bltBuf.GetDstPtr();

	if(m_trxCtx.nDirty)
	{
		FlushVertexBuffer();
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

		TexCache_InvalidateTextures(transferAddress + transferOffset, transferSize);

		bool isUpperByteTransfer = (bltBuf.nDstPsm == PSMT8H) || (bltBuf.nDstPsm == PSMT4HL) || (bltBuf.nDstPsm == PSMT4HH);
		for(const auto& framebuffer : m_framebuffers)
		{
			if((framebuffer->m_psm == PSMCT24) && isUpperByteTransfer) continue;
			framebuffer->m_cachedArea.Invalidate(transferAddress + transferOffset, transferSize);
		}
	}
}

void CGSH_OpenGL::ProcessLocalToHostTransfer()
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

	FlushVertexBuffer();
	m_renderState.isValid = false;

	auto pixels = new uint32[trxReg.nRRW * trxReg.nRRH];

	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer->m_framebuffer);
	glReadPixels(trxPos.nSSAX, trxPos.nSSAY, trxReg.nRRW, trxReg.nRRH, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	CHECKGLERROR();

	//Write back to RAM
	{
		CGsPixelFormats::CPixelIndexorPSMCT32 indexor(m_pRAM, bltBuf.GetSrcPtr(), bltBuf.nSrcWidth);
		for(uint32 y = trxPos.nSSAY; y < (trxPos.nSSAY + trxReg.nRRH); y++)
		{
			for(uint32 x = trxPos.nSSAX; x < (trxPos.nSSAX + trxReg.nRRH); x++)
			{
				uint32 pixel = pixels[x + (y * trxReg.nRRW)];
				indexor.SetPixel(x, y, pixel);
			}
		}
	}

	delete [] pixels;
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
		FlushVertexBuffer();
		m_renderState.isValid = false;

		const auto& srcFramebuffer = (*srcFramebufferIterator);
		const auto& dstFramebuffer = (*dstFramebufferIterator);

		glBindFramebuffer(GL_FRAMEBUFFER, dstFramebuffer->m_framebuffer);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, srcFramebuffer->m_framebuffer);

		//Copy buffers
		glBlitFramebuffer(
			0, 0, srcFramebuffer->m_width * m_fbScale, srcFramebuffer->m_height * m_fbScale, 
			0, 0, srcFramebuffer->m_width * m_fbScale, srcFramebuffer->m_height * m_fbScale, 
			GL_COLOR_BUFFER_BIT, GL_NEAREST);
		CHECKGLERROR();

		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	}
}

void CGSH_OpenGL::ProcessClutTransfer(uint32 csa, uint32)
{
	FlushVertexBuffer();
	m_renderState.isValid = false;
	PalCache_Invalidate(csa);
}

void CGSH_OpenGL::ReadFramebuffer(uint32 width, uint32 height, void* buffer)
{
	//TODO: Implement this in a better way. This is only used for movie recording on Win32 for now.
#ifdef GLES_COMPATIBILITY
	assert(false);
#else
	glFlush();
	glFinish();
	glReadPixels(0, 0, width, height, GL_BGR, GL_UNSIGNED_BYTE, buffer);
#endif
}

/////////////////////////////////////////////////////////////
// Framebuffer
/////////////////////////////////////////////////////////////

CGSH_OpenGL::CFramebuffer::CFramebuffer(uint32 basePtr, uint32 width, uint32 height, uint32 psm, uint32 scale)
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
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width * scale, m_height * scale, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	CHECKGLERROR();
		
	//Build framebuffer
	glGenFramebuffers(1, &m_framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texture, 0);

	CHECKGLERROR();

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
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_copyToFbTexture);
	((this)->*(m_textureUploader[framebuffer->m_psm]))(framebuffer->m_basePtr, 
		framebuffer->m_width / 64, framebuffer->m_width, framebuffer->m_height);
	CHECKGLERROR();

	glBindFramebuffer(GL_FRAMEBUFFER, m_copyToFbFramebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_copyToFbTexture, 0);
	auto fbStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	assert(fbStatus == GL_FRAMEBUFFER_COMPLETE);
	CHECKGLERROR();

	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer->m_framebuffer);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, m_copyToFbFramebuffer);

	//Copy buffers
	glBlitFramebuffer(
		0, 0, framebuffer->m_width, framebuffer->m_height, 
		0, 0, framebuffer->m_width * m_fbScale, framebuffer->m_height * m_fbScale, 
		GL_COLOR_BUFFER_BIT, GL_NEAREST);
	CHECKGLERROR();

	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
}

void CGSH_OpenGL::CommitFramebufferDirtyPages(const FramebufferPtr& framebuffer, unsigned int minY, unsigned int maxY)
{
	class CCopyToFbEnabler
	{
	public:
		~CCopyToFbEnabler()
		{

		}

		void EnableCopyToFb(const FramebufferPtr& framebuffer, GLuint copyToFbFramebuffer, GLuint copyToFbTexture)
		{
			if(m_copyToFbEnabled) return;

			glDisable(GL_SCISSOR_TEST);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, copyToFbTexture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, framebuffer->m_width, framebuffer->m_height, 
				0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

			glBindFramebuffer(GL_FRAMEBUFFER, copyToFbFramebuffer);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, copyToFbTexture, 0);
			auto fbStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			assert(fbStatus == GL_FRAMEBUFFER_COMPLETE);
			CHECKGLERROR();

			glBindFramebuffer(GL_FRAMEBUFFER, framebuffer->m_framebuffer);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, copyToFbFramebuffer);

			m_copyToFbEnabled = true;
		}

	private:
		bool m_copyToFbEnabled = false;
	};

	auto& cachedArea = framebuffer->m_cachedArea;

	if(cachedArea.HasDirtyPages())
	{
		CCopyToFbEnabler copyToFbEnabler;

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
			
			m_validGlState &= ~(GLSTATE_SCISSOR | GLSTATE_FRAMEBUFFER);
			copyToFbEnabler.EnableCopyToFb(framebuffer, m_copyToFbFramebuffer, m_copyToFbTexture);

			((this)->*(m_textureUpdater[framebuffer->m_psm]))(framebuffer->m_basePtr, framebuffer->m_width / 64,
				texX, texY, texWidth, texHeight);
			
			glBlitFramebuffer(
				texX,             texY,             (texX + texWidth),             (texY + texHeight), 
				texX * m_fbScale, texY * m_fbScale, (texX + texWidth) * m_fbScale, (texY + texHeight) * m_fbScale, 
				GL_COLOR_BUFFER_BIT, GL_NEAREST);
			CHECKGLERROR();
		}

		//Mark all pages as clean, but might be wrong due to range not
		//covering an area that might be used later on
		cachedArea.ClearDirtyPages();
	}
}

/////////////////////////////////////////////////////////////
// Depthbuffer
/////////////////////////////////////////////////////////////

CGSH_OpenGL::CDepthbuffer::CDepthbuffer(uint32 basePtr, uint32 width, uint32 height, uint32 psm, uint32 scale)
: m_basePtr(basePtr)
, m_width(width)
, m_height(height)
, m_psm(psm)
, m_depthBuffer(0)
{
	//Build depth attachment
	glGenRenderbuffers(1, &m_depthBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, m_depthBuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, m_width * scale, m_height * scale);
	CHECKGLERROR();
}

CGSH_OpenGL::CDepthbuffer::~CDepthbuffer()
{
	if(m_depthBuffer != 0)
	{
		glDeleteRenderbuffers(1, &m_depthBuffer);
	}
}
