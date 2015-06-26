#pragma once

#include <list>
#include <unordered_map>
#include "../GSHandler.h"
#include "../GsCachedArea.h"
#include "opengl/OpenGlDef.h"
#include "opengl/Program.h"
#include "opengl/Shader.h"
#include "opengl/Resource.h"

#define PREF_CGSH_OPENGL_LINEASQUADS				"renderer.opengl.linesasquads"
#define PREF_CGSH_OPENGL_FORCEBILINEARTEXTURES		"renderer.opengl.forcebilineartextures"
#define PREF_CGSH_OPENGL_FIXSMALLZVALUES			"renderer.opengl.fixsmallzvalues"

class CGSH_OpenGL : public CGSHandler
{
public:
									CGSH_OpenGL();
	virtual							~CGSH_OpenGL();

	virtual void					LoadState(Framework::CZipArchiveReader&) override;
	
	void							ProcessImageTransfer() override;
	void							ProcessClutTransfer(uint32, uint32) override;
	void							ProcessLocalToLocalTransfer() override;
	void							ReadFramebuffer(uint32, uint32, void*) override;

	bool							IsBlendColorExtSupported();
	bool							IsBlendEquationExtSupported();
	bool							IsFogCoordfExtSupported();

protected:
	void							TexCache_Flush();
	void							PalCache_Flush();
	void							LoadSettings();
	virtual void					InitializeImpl() override;
	virtual void					ReleaseImpl() override;
	virtual void					ResetImpl() override;
	virtual void					FlipImpl() override;

	GLuint							m_presentFramebuffer = 0;

private:
	struct RENDERSTATE
	{
		bool		isValid;
		uint64		primReg;
		uint64		frameReg;
		uint64		testReg;
		uint64		alphaReg;
		uint64		zbufReg;
		uint64		scissorReg;
		uint64		tex0Reg;
		uint64		tex1Reg;
		uint64		texAReg;
		uint64		clampReg;
		GLuint		shaderHandle;
	};

	enum
	{
		MAX_TEXTURE_CACHE = 256,
		MAX_PALETTE_CACHE = 256,
	};

	enum CVTBUFFERSIZE
	{
		CVTBUFFERSIZE = 0x800000,
	};

	typedef void (CGSH_OpenGL::*TEXTUREUPLOADER)(uint32, uint32, unsigned int, unsigned int);
	typedef void (CGSH_OpenGL::*TEXTUREUPDATER)(uint32, uint32, unsigned int, unsigned int, unsigned int, unsigned int);

	struct VERTEX
	{
		uint64						nPosition;
		uint64						nRGBAQ;
		uint64						nUV;
		uint64						nST;
		uint8						nFog;
	};

	enum
	{
		TEXTURE_SOURCE_MODE_NONE	= 0,
		TEXTURE_SOURCE_MODE_STD		= 1,
		TEXTURE_SOURCE_MODE_IDX4	= 2,
		TEXTURE_SOURCE_MODE_IDX8	= 3
	};

	enum TEXTURE_CLAMP_MODE
	{
		TEXTURE_CLAMP_MODE_STD					= 0,
		TEXTURE_CLAMP_MODE_REGION_CLAMP			= 1,
		TEXTURE_CLAMP_MODE_REGION_REPEAT		= 2,
		TEXTURE_CLAMP_MODE_REGION_REPEAT_SIMPLE	= 3
	};

	struct SHADERCAPS : public convertible<uint32>
	{
		unsigned int texFunction			: 2;		//0 - Modulate, 1 - Decal, 2 - Highlight, 3 - Hightlight2
		unsigned int texClampS				: 2;
		unsigned int texClampT				: 2;
		unsigned int texSourceMode			: 2;
		unsigned int texHasAlpha			: 1;
		unsigned int texBilinearFilter		: 1;
		unsigned int texUseAlphaExpansion	: 1;
		unsigned int texBlackIsTransparent	: 1;
		unsigned int hasFog					: 1;
		unsigned int padding				: 19;

		bool isIndexedTextureSource() const { return texSourceMode == TEXTURE_SOURCE_MODE_IDX4 || texSourceMode == TEXTURE_SOURCE_MODE_IDX8; }
	};
	static_assert(sizeof(SHADERCAPS) == sizeof(uint32), "SHADERCAPS structure size must be 4 bytes.");

	struct SHADERINFO
	{
		Framework::OpenGl::ProgramPtr	shader;
		GLint							projMatrixUniform;
		GLint							texMatrixUniform;
		GLint							textureUniform;
		GLint							paletteUniform;
		GLint							textureSizeUniform;
		GLint							texelSizeUniform;
		GLint							clampMinUniform;
		GLint							clampMaxUniform;
		GLint							texA0Uniform;
		GLint							texA1Uniform;
	};

	typedef std::unordered_map<uint32, SHADERINFO> ShaderInfoMap;

	class CTexture
	{
	public:
									CTexture();
									~CTexture();
		void						Free();

		uint64						m_tex0;
		GLuint						m_texture;
		bool						m_live;

		CGsCachedArea				m_cachedArea;
	};
	typedef std::shared_ptr<CTexture> TexturePtr;
	typedef std::list<TexturePtr> TextureList;

	class CPalette
	{
	public:
									CPalette();
									~CPalette();

		void						Invalidate(uint32);
		void						Free();

		bool						m_live;

		bool						m_isIDTEX4;
		uint32						m_cpsm;
		uint32						m_csa;
		GLuint						m_texture;
		uint32						m_contents[256];
	};
	typedef std::shared_ptr<CPalette> PalettePtr;
	typedef std::list<PalettePtr> PaletteList;

	class CFramebuffer
	{
	public:
									CFramebuffer(uint32, uint32, uint32, uint32);
									~CFramebuffer();

		void						SetBufferWidth(uint32);

		uint32						m_basePtr;
		uint32						m_width;
		uint32						m_height;
		uint32						m_psm;

		GLuint						m_framebuffer;
		GLuint						m_texture;
		uint32						m_textureWidth;

		CGsCachedArea				m_cachedArea;
	};
	typedef std::shared_ptr<CFramebuffer> FramebufferPtr;
	typedef std::vector<FramebufferPtr> FramebufferList;

	class CDepthbuffer
	{
	public:
									CDepthbuffer(uint32, uint32, uint32, uint32);
									~CDepthbuffer();

		uint32						m_basePtr;
		uint32						m_width;
		uint32						m_height;
		uint32						m_psm;
		GLuint						m_depthBuffer;
	};
	typedef std::shared_ptr<CDepthbuffer> DepthbufferPtr;
	typedef std::vector<DepthbufferPtr> DepthbufferList;

	struct TEXTURE_INFO
	{
		float	offsetX = 0;
		float	scaleRatioX = 1;
		float	scaleRatioY = 1;
	};

	enum class PRIM_VERTEX_ATTRIB
	{
		POSITION = 1,
		COLOR = 2,
		TEXCOORD = 3,
	};

	struct PRIM_VERTEX
	{
		float x, y, z;
		uint32 color;
		float s, t, q;
	};

	enum VERTEX_BUFFER_SIZE
	{
		VERTEX_BUFFER_SIZE = 0x1000,
	};

	typedef std::vector<PRIM_VERTEX> VertexBuffer;

	void							WriteRegisterImpl(uint8, uint64) override;

	void							InitializeRC();
	void							SetupTextureUploaders();
	virtual void					PresentBackbuffer() = 0;
	void							MakeLinearZOrtho(float*, float, float, float, float);
	unsigned int					GetCurrentReadCircuit();
	TEXTURE_INFO					PrepareTexture(const TEX0&);
	void							PreparePalette(const TEX0&);

	uint32							RGBA16ToRGBA32(uint16);
	uint8							MulBy2Clamp(uint8);
	float							GetZ(float);
	unsigned int					GetNextPowerOf2(unsigned int);

	void							VertexKick(uint8, uint64);

	Framework::OpenGl::ProgramPtr	GenerateShader(const SHADERCAPS&);
	Framework::OpenGl::CShader		GenerateVertexShader(const SHADERCAPS&);
	Framework::OpenGl::CShader		GenerateFragmentShader(const SHADERCAPS&);
	std::string						GenerateTexCoordClampingSection(TEXTURE_CLAMP_MODE, const char*);

	Framework::OpenGl::ProgramPtr	GeneratePresentProgram();
	Framework::OpenGl::CVertexArray	GeneratePrimVertexArray();

	void							Prim_Point();
	void							Prim_Line();
	void							Prim_Triangle();
	void							Prim_Sprite();

	void							FlushVertexBuffer();

	void							DrawToDepth(unsigned int, uint64);

	void							SetRenderingContext(uint64);
	void							SetupTestFunctions(uint64);
	void							SetupDepthBuffer(uint64, uint64);
	void							SetupFramebuffer(const SHADERINFO&, uint64, uint64, uint64, uint64);
	void							SetupBlendingFunction(uint64);
	void							SetupFogColor();

	static bool						CanRegionRepeatClampModeSimplified(uint32, uint32);
	void							FillShaderCapsFromTexture(SHADERCAPS&, uint64, uint64, uint64, uint64);
	void							SetupTexture(const SHADERINFO&, uint64, uint64, uint64, uint64, uint64);
	static bool						IsCompatibleFramebufferPSM(unsigned int, unsigned int);

	FramebufferPtr					FindCompatibleFramebuffer(const FRAME&) const;
	DepthbufferPtr					FindDepthbuffer(const ZBUF&, const FRAME&) const;

	void							DumpTexture(unsigned int, unsigned int, uint32);

	//Texture uploaders
	void							TexUploader_Invalid(uint32, uint32, unsigned int, unsigned int);

	void							TexUploader_Psm32(uint32, uint32, unsigned int, unsigned int);
	template <typename> void		TexUploader_Psm16(uint32, uint32, unsigned int, unsigned int);

	template <typename> void		TexUploader_Psm48(uint32, uint32, unsigned int, unsigned int);
	template <uint32, uint32> void	TexUploader_Psm48H(uint32, uint32, unsigned int, unsigned int);

	//Texture updaters
	void							TexUpdater_Invalid(uint32, uint32, unsigned int, unsigned int, unsigned int, unsigned int);

	void							TexUpdater_Psm32(uint32, uint32, unsigned int, unsigned int, unsigned int, unsigned int);
	template <typename> void		TexUpdater_Psm16(uint32, uint32, unsigned int, unsigned int, unsigned int, unsigned int);

	template <typename> void		TexUpdater_Psm48(uint32, uint32, unsigned int, unsigned int, unsigned int, unsigned int);
	template <uint32, uint32> void	TexUpdater_Psm48H(uint32, uint32, unsigned int, unsigned int, unsigned int, unsigned int);

	//Context variables (put this in a struct or something?)
	float							m_nPrimOfsX;
	float							m_nPrimOfsY;
	uint32							m_nTexWidth;
	uint32							m_nTexHeight;
	float							m_nMaxZ;

	bool							m_nLinesAsQuads;
	bool							m_nForceBilinearTextures;
	bool							m_fixSmallZValues;

	uint8*							m_pCvtBuffer;

	CTexture*						TexCache_Search(const TEX0&);
	void							TexCache_Insert(const TEX0&, GLuint);
	void							TexCache_InvalidateTextures(uint32, uint32);

	GLuint							PalCache_Search(const TEX0&);
	GLuint							PalCache_Search(unsigned int, const uint32*);
	void							PalCache_Insert(const TEX0&, const uint32*, GLuint);
	void							PalCache_Invalidate(uint32);

	void							PopulateFramebuffer(const FramebufferPtr&);
	void							CommitFramebufferDirtyPages(const FramebufferPtr&, unsigned int, unsigned int);

	Framework::OpenGl::ProgramPtr	m_presentProgram;
	GLint							m_presentTextureUniform = -1;
	GLint							m_presentTexCoordScaleUniform = -1;

	TextureList						m_textureCache;
	PaletteList						m_paletteCache;
	FramebufferList					m_framebuffers;
	DepthbufferList					m_depthbuffers;

	Framework::OpenGl::CVertexArray	m_emptyVertexArray;
	Framework::OpenGl::CBuffer		m_primBuffer;
	Framework::OpenGl::CVertexArray	m_primVertexArray;

	VERTEX							m_VtxBuffer[3];
	int								m_nVtxCount;

	PRMODE							m_PrimitiveMode;
	unsigned int					m_primitiveType;
	bool							m_drawingToDepth = false;

	static GLenum					g_nativeClampModes[CGSHandler::CLAMP_MODE_MAX];
	static unsigned int				g_shaderClampModes[CGSHandler::CLAMP_MODE_MAX];

	TEXTUREUPLOADER					m_textureUploader[CGSHandler::PSM_MAX];
	TEXTUREUPDATER					m_textureUpdater[CGSHandler::PSM_MAX];

	ShaderInfoMap					m_shaderInfos;
	RENDERSTATE						m_renderState;
	VertexBuffer					m_vertexBuffer;
};
