#ifndef _GSH_OPENGL_H_
#define _GSH_OPENGL_H_

#include <list>
#include "GSHandler.h"
#include "opengl/OpenGlDef.h"
#include "opengl/Program.h"
#include "opengl/Shader.h"

#define PREF_CGSH_OPENGL_LINEASQUADS				"renderer.opengl.linesasquads"
#define PREF_CGSH_OPENGL_FORCEBILINEARTEXTURES		"renderer.opengl.forcebilineartextures"
#define PREF_CGSH_OPENGL_FORCEFLIPPINGVSYNC			"renderer.opengl.forceflippingvsync"
#define PREF_CGSH_OPENGL_FIXSMALLZVALUES            "renderer.opengl.fixsmallzvalues"

class CGSH_OpenGL : public CGSHandler
{
public:
									CGSH_OpenGL();
	virtual                         ~CGSH_OpenGL();

	virtual void					LoadState(Framework::CZipArchiveReader&);

	void							ProcessImageTransfer(uint32, uint32);

    virtual void                    ForcedFlip();

	bool							IsColorTableExtSupported();
	bool							IsBlendColorExtSupported();
	bool							IsBlendEquationExtSupported();
	bool							IsRGBA5551ExtSupported();
	bool							IsFogCoordfExtSupported();

protected:
    enum SHADER
    {
        SHADER_VERTEX = 1,
        SHADER_FRAGMENT = 2
    };

    void							TexCache_Flush();
    void							LoadSettings();
    virtual void                    InitializeImpl();
    virtual void                    ReleaseImpl();
	virtual void					FlipImpl();
    virtual void					SetViewport(int, int);

private:
	enum MAXCACHE
	{
		MAXCACHE = 256,
	};

	enum CVTBUFFERSIZE
	{
		CVTBUFFERSIZE = 0x400000,
	};

    enum MAX_PIXEL_BUFFERS
    {
        MAX_PIXEL_BUFFERS = 10,
    };

    enum PIXEL_BUFFER_SIZE
    {
        PIXEL_BUFFER_SIZE = CVTBUFFERSIZE,
    };

	typedef void (CGSH_OpenGL::*TEXTUREUPLOADER)(TEX0*, TEXA*);

	struct VERTEX
	{
		uint64						nPosition;
		uint64						nRGBAQ;
		uint64						nUV;
		uint64						nST;
		uint8						nFog;
	};

	class CTexture
	{
	public:
									CTexture();
									~CTexture();
		void						InvalidateFromMemorySpace(uint32, uint32);
        void                        Free();

		uint32						m_nStart;
		uint32						m_nSize;
		unsigned int				m_nPSM;
		uint32						m_nCLUTAddress;
		uint64						m_nTex0;
		uint64						m_nTexClut;
		bool						m_nIsCSM2;
		unsigned int				m_nTexture;
        uint32                      m_checksum;
        bool                        m_live;
	};

    typedef std::list<CTexture*> TextureList;

    void							WriteRegisterImpl(uint8, uint64);

	void							InitializeRC();
    virtual void                    LoadShaderSource(Framework::OpenGl::CShader*, SHADER) = 0;
	void							SetReadCircuitMatrix(int, int);
	void							LinearZOrtho(float, float, float, float);
	void							UpdateViewportImpl();
	unsigned int					GetCurrentReadCircuit();
	unsigned int					LoadTexture(TEX0*, TEX1*, CLAMP*);
	void							SyncCLUT(TEX0*);

	void							ReadCLUT4(TEX0*);
	void							ReadCLUT8(TEX0*);

	uint32							RGBA16ToRGBA32(uint16);
	uint8							MulBy2Clamp(uint8);
	float							GetZ(float);
	unsigned int					GetNextPowerOf2(unsigned int);

	void							VertexKick(uint8, uint64);

	void							Prim_Point();
	void							Prim_Line();
	void							Prim_Triangle();
	void							Prim_Sprite();

	void							SetRenderingContext(unsigned int);
	void							SetupTestFunctions(uint64);
	void							SetupDepthBuffer(uint64);
	void							SetupBlendingFunction(uint64);
	void							SetupFogColor();
	void							SetupTexture(uint64, uint64, uint64);

	void							DumpTexture(unsigned int, unsigned int, uint32);

	void							DisplayTransferedImage(uint32);

	void							TexUploader_Psm32(TEX0*, TEXA*);
    void                            TexUploader_Psm24(TEX0*, TEXA*);

	void							TexUploader_Psm8_Cvt(TEX0*, TEXA*);
	void							TexUploader_Psm8_Hw(TEX0*, TEXA*);

	void							TexUploader_Psm16_Cvt(TEX0*, TEXA*);
	void							TexUploader_Psm16_Hw(TEX0*, TEXA*);

	void							TexUploader_Psm16S_Hw(TEX0*, TEXA*);

	void							TexUploader_Psm4_Cvt(TEX0*, TEXA*);
	template <uint32> void			TexUploader_Psm4H_Cvt(TEX0*, TEXA*);
	void							TexUploader_Psm8H_Cvt(TEX0*, TEXA*);

    uint32                          ConvertTexturePsm4(TEX0*, TEXA*);
    uint32                          ConvertTexturePsm8(TEX0*, TEXA*);
    uint32                          ConvertTexturePsm8H(TEX0*, TEXA*);
    void                            UploadTexturePsm8(TEX0*, TEXA*);

	//Context variables (put this in a struct or something?)
	float							m_nPrimOfsX;
	float							m_nPrimOfsY;
	uint32							m_nTexWidth;
	uint32							m_nTexHeight;
	unsigned int					m_nTexHandle;
	float							m_nMaxZ;

	int								m_nWidth;
	int								m_nHeight;

	bool							m_nLinesAsQuads;
	bool							m_nForceBilinearTextures;
	bool							m_nForceFlippingVSync;
    bool                            m_fixSmallZValues;

	uint8*							m_pCvtBuffer;
	void*							m_pCLUT;
	uint16*							m_pCLUT16;
	uint32*							m_pCLUT32;
	uint32							m_nCBP0;
	uint32							m_nCBP1;

	void							VerifyRGBA5551Support();
	bool							m_nIsRGBA5551Supported;

    unsigned int					TexCache_SearchLive(TEX0*);
    unsigned int                    TexCache_SearchDead(TEX0*, uint32);
	void							TexCache_Insert(TEX0*, unsigned int, uint32);
	void							TexCache_InvalidateTextures(uint32, uint32);

//	CTexture						m_TexCache[MAXCACHE];
//	unsigned int					m_nTexCacheIndex;
    TextureList                     m_TexCache;

	VERTEX							m_VtxBuffer[3];
	int								m_nVtxCount;

	PRMODE							m_PrimitiveMode;
	unsigned int					m_nPrimitiveType;

	TEXTUREUPLOADER					m_pTexUploader_Psm8;
	TEXTUREUPLOADER					m_pTexUploader_Psm16;

	Framework::OpenGl::CProgram*	m_pProgram;
	Framework::OpenGl::CShader*		m_pVertShader;
	Framework::OpenGl::CShader*		m_pFragShader;

    //GLuint                          m_pixelBuffers[MAX_PIXEL_BUFFERS];
    //unsigned int                    m_currentPixelBuffer;
};

#endif