#ifndef _GSH_OPENGL_H_
#define _GSH_OPENGL_H_

#include "../GSHandler.h"
#include "../PS2VM.h"
#include "win32/Window.h"
#include "opengl/OpenGl.h"
#include "opengl/Program.h"
#include "opengl/Shader.h"
#include "SettingsDialogProvider.h"

#define PREF_CGSH_OPENGL_LINEASQUADS				"renderer.opengl.linesasquads"
#define PREF_CGSH_OPENGL_FORCEBILINEARTEXTURES		"renderer.opengl.forcebilineartextures"
#define PREF_CGSH_OPENGL_FORCEFLIPPINGVSYNC			"renderer.opengl.forceflippingvsync"

class CGSH_OpenGL : public CGSHandler, public CSettingsDialogProvider
{
public:
									CGSH_OpenGL(Framework::Win32::CWindow*);
	virtual                         ~CGSH_OpenGL();

	static void						CreateGSHandler(CPS2VM&, Framework::Win32::CWindow*);

	virtual void					LoadState(Framework::CStream*);

	void							ProcessImageTransfer(uint32, uint32);

	virtual void					SetVBlank();
	CSettingsDialogProvider*		GetSettingsDialogProvider();

	CModalWindow*					CreateSettingsDialog(HWND);
	void							OnSettingsDialogDestroyed();

	bool							IsColorTableExtSupported();
	bool							IsBlendColorExtSupported();
	bool							IsBlendEquationExtSupported();
	bool							IsRGBA5551ExtSupported();
	bool							IsFogCoordfExtSupported();

private:
	enum MAXCACHE
	{
		MAXCACHE = 50,
	};

	enum CVTBUFFERSIZE
	{
		CVTBUFFERSIZE = 0x400000,
	};

	typedef void (CGSH_OpenGL::*TEXTUREUPLOADER)(GSTEX0*, GSTEXA*);

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
		void						Invalidate();
		void						InvalidateFromMemorySpace(uint32, uint32);
		bool						IsValid();

		uint32						m_nStart;
		uint32						m_nSize;
		unsigned int				m_nPSM;
		uint32						m_nCLUTAddress;
		uint64						m_nTex0;
		uint64						m_nTexClut;
		bool						m_nIsCSM2;
		unsigned int				m_nTexture;
	};

	static CGSHandler*				GSHandlerFactory(void*);

    void                            InitializeImpl();
	virtual void					FlipImpl();
    void							WriteRegisterImpl(uint8, uint64);

    void							LoadSettings();

	void							InitializeRC();
	void							LoadShaderSourceFromResource(Framework::OpenGl::CShader*, const TCHAR*);
	void							SetViewport(int, int);
	void							SetReadCircuitMatrix(int, int);
	void							LinearZOrtho(double, double, double, double);
	void							UpdateViewportImpl();
	unsigned int					GetCurrentReadCircuit();
	unsigned int					LoadTexture(GSTEX0*, GSTEX1*, CLAMP*);

	void							ReadCLUT4(GSTEX0*);
	void							ReadCLUT8(GSTEX0*);

	uint32							RGBA16ToRGBA32(uint16);
	uint8							MulBy2Clamp(uint8);
	double							GetZ(double);
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

	void							DumpTexture(unsigned int, unsigned int);

	void							DisplayTransferedImage(uint32);

	void							TexUploader_Psm32(GSTEX0*, GSTEXA*);

	void							TexUploader_Psm8_Cvt(GSTEX0*, GSTEXA*);
	void							TexUploader_Psm8_Hw(GSTEX0*, GSTEXA*);

	void							TexUploader_Psm16_Cvt(GSTEX0*, GSTEXA*);
	void							TexUploader_Psm16_Hw(GSTEX0*, GSTEXA*);

	void							TexUploader_Psm16S_Hw(GSTEX0*, GSTEXA*);

	void							TexUploader_Psm4_Cvt(GSTEX0*, GSTEXA*);
	template <uint32> void			TexUploader_Psm4H_Cvt(GSTEX0*, GSTEXA*);
	void							TexUploader_Psm8H_Cvt(GSTEX0*, GSTEXA*);

	//Context variables (put this in a struct or something?)
	double							m_nPrimOfsX;
	double							m_nPrimOfsY;
	uint32							m_nTexWidth;
	uint32							m_nTexHeight;
	unsigned int					m_nTexHandle;
	double							m_nMaxZ;

	int								m_nWidth;
	int								m_nHeight;

	bool							m_nLinesAsQuads;
	bool							m_nForceBilinearTextures;
	bool							m_nForceFlippingVSync;

	uint8*							m_pCvtBuffer;
	void*							m_pCLUT;
	uint16*							m_pCLUT16;
	uint32*							m_pCLUT32;

	void							VerifyRGBA5551Support();
	bool							m_nIsRGBA5551Supported;

	unsigned int					TexCache_Search(GSTEX0*);
	void							TexCache_Insert(GSTEX0*, unsigned int);
	void							TexCache_InvalidateTextures(uint32, uint32);
	void							TexCache_Flush();

	CTexture						m_TexCache[MAXCACHE];
	unsigned int					m_nTexCacheIndex;

	VERTEX							m_VtxBuffer[3];
	int								m_nVtxCount;

	GSPRMODE						m_PrimitiveMode;
	unsigned int					m_nPrimitiveType;

	TEXTUREUPLOADER					m_pTexUploader_Psm8;
	TEXTUREUPLOADER					m_pTexUploader_Psm16;

	Framework::Win32::CWindow*		m_pOutputWnd;

	Framework::OpenGl::CProgram*	m_pProgram;
	Framework::OpenGl::CShader*		m_pVertShader;
	Framework::OpenGl::CShader*		m_pFragShader;

	HGLRC							m_hRC;
	HDC								m_hDC;
	static PIXELFORMATDESCRIPTOR	m_PFD;
};

#endif