#ifndef _GSH_OPENGL_H_
#define _GSH_OPENGL_H_

#include <list>
#include <unordered_map>
#include "GSHandler.h"
#include "opengl/OpenGlDef.h"
#include "opengl/Program.h"
#include "opengl/Shader.h"

#define PREF_CGSH_OPENGL_LINEASQUADS				"renderer.opengl.linesasquads"
#define PREF_CGSH_OPENGL_FORCEBILINEARTEXTURES		"renderer.opengl.forcebilineartextures"
#define PREF_CGSH_OPENGL_FIXSMALLZVALUES			"renderer.opengl.fixsmallzvalues"

class CGSH_OpenGL : public CGSHandler
{
public:
	enum PRESENTATION_MODE
	{
		PRESENTATION_MODE_FILL,
		PRESENTATION_MODE_FIT,
		PRESENTATION_MODE_ORIGINAL
	};
	
	struct PRESENTATION_PARAMS
	{
		uint32				windowWidth;
		uint32				windowHeight;
		PRESENTATION_MODE	mode;
	};
									CGSH_OpenGL();
	virtual							~CGSH_OpenGL();

	virtual void					LoadState(Framework::CZipArchiveReader&);

	void							SetPresentationParams(const PRESENTATION_PARAMS&);
	
	void							ProcessImageTransfer(uint32, uint32);
	void							ProcessClutTransfer(uint32, uint32);
	void							ReadFramebuffer(uint32, uint32, void*);

	bool							IsBlendColorExtSupported();
	bool							IsBlendEquationExtSupported();
	bool							IsRGBA5551ExtSupported();
	bool							IsFogCoordfExtSupported();

protected:
	void							TexCache_Flush();
	void							PalCache_Flush();
	void							LoadSettings();
	virtual void					InitializeImpl();
	virtual void					ReleaseImpl();
	virtual void					FlipImpl();

private:
	struct RENDERSTATE
	{
		bool		isValid;
		uint64		frameReg;
		uint64		testReg;
		uint64		alphaReg;
		uint64		zbufReg;
		GLuint		shaderHandle;
	};

	enum
	{
		MAX_TEXTURE_CACHE = 256,
		MAX_PALETTE_CACHE = 256,
	};

	enum CVTBUFFERSIZE
	{
		CVTBUFFERSIZE = 0x400000,
	};

	typedef void (CGSH_OpenGL::*TEXTUREUPLOADER)(const TEX0&, const TEXA&);

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
		unsigned int texFunction		: 2;		//0 - Modulate, 1 - Decal, 2 - Highlight, 3 - Hightlight2
		unsigned int texClampS			: 2;
		unsigned int texClampT			: 2;
		unsigned int texSourceMode		: 2;
		unsigned int texBilinearFilter	: 1;
		unsigned int hasFog				: 1;

		bool isIndexedTextureSource() const { return texSourceMode == TEXTURE_SOURCE_MODE_IDX4 || texSourceMode == TEXTURE_SOURCE_MODE_IDX8; }
	};
	static_assert(sizeof(SHADERCAPS) == sizeof(uint32), "SHADERCAPS structure size must be 4 bytes.");

	struct SHADERINFO
	{
		Framework::OpenGl::ProgramPtr	shader;
		GLint							textureUniform;
		GLint							paletteUniform;
		GLint							textureSizeUniform;
		GLint							texelSizeUniform;
		GLint							clampMinUniform;
		GLint							clampMaxUniform;
	};

	typedef std::unordered_map<uint32, SHADERINFO> ShaderInfoMap;

	class CTexture
	{
	public:
									CTexture();
									~CTexture();
		void						InvalidateFromMemorySpace(uint32, uint32);
		void						Free();

		uint32						m_start;
		uint32						m_size;
		uint64						m_tex0;
		GLuint						m_texture;
		bool						m_live;
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

	void							WriteRegisterImpl(uint8, uint64);

	void							InitializeRC();
	virtual void					PresentBackbuffer() = 0;
	void							SetReadCircuitMatrix(int, int);
	void							LinearZOrtho(float, float, float, float);
	void							UpdateViewportImpl();
	unsigned int					GetCurrentReadCircuit();
	void							PrepareTexture(const TEX0&);
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

	void							Prim_Point();
	void							Prim_Line();
	void							Prim_Triangle();
	void							Prim_Sprite();

	void							SetRenderingContext(unsigned int);
	void							SetupTestFunctions(uint64);
	void							SetupDepthBuffer(uint64);
	void							SetupBlendingFunction(uint64);
	void							SetupFogColor();
	void							SetupTexture(SHADERCAPS&, uint64, uint64, uint64);

	void							DumpTexture(unsigned int, unsigned int, uint32);

	void							DisplayTransferedImage(uint32);

	void							TexUploader_Psm32(const TEX0&, const TEXA&);
	void							TexUploader_Psm24(const TEX0&, const TEXA&);

	void							TexUploader_Psm16_Cvt(const TEX0&, const TEXA&);
	void							TexUploader_Psm16_Hw(const TEX0&, const TEXA&);
	void							TexUploader_Psm16S_Hw(const TEX0&, const TEXA&);

	void							TexUploader_Psm8(const TEX0&, const TEXA&);
	void							TexUploader_Psm4(const TEX0&, const TEXA&);
	template <uint32> void			TexUploader_Psm4H(const TEX0&, const TEXA&);
	void							TexUploader_Psm8H(const TEX0&, const TEXA&);

	PRESENTATION_PARAMS				m_presentationParams;
	
	//Context variables (put this in a struct or something?)
	float							m_nPrimOfsX;
	float							m_nPrimOfsY;
	uint32							m_nTexWidth;
	uint32							m_nTexHeight;
	float							m_nMaxZ;

	int								m_displayWidth;
	int								m_displayHeight;

	bool							m_nLinesAsQuads;
	bool							m_nForceBilinearTextures;
	bool							m_fixSmallZValues;

	uint8*							m_pCvtBuffer;

	void							VerifyRGBA5551Support();
	bool							m_nIsRGBA5551Supported;

	GLuint							TexCache_Search(const TEX0&);
	void							TexCache_Insert(const TEX0&, GLuint);
	void							TexCache_InvalidateTextures(uint32, uint32);

	GLuint							PalCache_Search(const TEX0&);
	GLuint							PalCache_Search(unsigned int, const uint32*);
	void							PalCache_Insert(const TEX0&, const uint32*, GLuint);
	void							PalCache_Invalidate(uint32);

	TextureList						m_textureCache;
	PaletteList						m_paletteCache;

	VERTEX							m_VtxBuffer[3];
	int								m_nVtxCount;

	PRMODE							m_PrimitiveMode;
	unsigned int					m_nPrimitiveType;

	static GLenum					g_nativeClampModes[CGSHandler::CLAMP_MODE_MAX];
	static unsigned int				g_shaderClampModes[CGSHandler::CLAMP_MODE_MAX];

	unsigned int					m_clampMin[2];
	unsigned int					m_clampMax[2];

	TEXTUREUPLOADER					m_pTexUploader_Psm16;

	ShaderInfoMap					m_shaderInfos;
	RENDERSTATE						m_renderState;
};

#endif
