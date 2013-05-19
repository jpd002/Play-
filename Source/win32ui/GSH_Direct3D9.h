#pragma once

#include "../GSHandler.h"
#include "win32/Window.h"
#include "win32/ComPtr.h"
#include "bitmap/Bitmap.h"
#include "OutputWnd.h"
#include <list>
#ifdef _DEBUG
#define D3D_DEBUG_INFO
#endif
#include <d3d9.h>

class CGSH_Direct3D9 : public CGSHandler
{
public:
	struct VERTEX
	{
		uint64						nPosition;
		uint64						nRGBAQ;
		uint64						nUV;
		uint64						nST;
		uint8						nFog;
	};

									CGSH_Direct3D9(Framework::Win32::CWindow*);
	virtual							~CGSH_Direct3D9();

	void							ProcessImageTransfer(uint32, uint32, bool) override;
	void							ProcessClutTransfer(uint32, uint32) override;
	void							ProcessLocalToLocalTransfer() override;
	void							ReadFramebuffer(uint32, uint32, void*) override;
	
	Framework::CBitmap				GetFramebuffer(uint64);
	const VERTEX*					GetInputVertices() const;

	static FactoryFunction			GetFactoryFunction(Framework::Win32::CWindow*);

protected:
	virtual void					ResetBase() override;
	virtual void					InitializeImpl() override;
	virtual void					ReleaseImpl() override;
	virtual void					FlipImpl() override;

	virtual void					WriteRegisterImpl(uint8, uint64) override;

private:
	typedef Framework::Win32::CComPtr<IDirect3D9> Direct3DPtr;
	typedef Framework::Win32::CComPtr<IDirect3DVertexBuffer9> VertexBufferPtr;
	typedef Framework::Win32::CComPtr<IDirect3DDevice9> DevicePtr;
	typedef Framework::Win32::CComPtr<IDirect3DTexture9> TexturePtr;
	typedef Framework::Win32::CComPtr<IDirect3DSurface9> SurfacePtr;

	enum MAXCACHE
	{
		MAXCACHE = 256,
	};

	enum CVTBUFFERSIZE
	{
		CVTBUFFERSIZE = 0x400000,
	};

	class CTexture
	{
	public:
									CTexture();
									~CTexture();
		void						InvalidateFromMemorySpace(uint32, uint32);
		void						Free();

		uint32						m_nStart;
		uint32						m_nSize;
		unsigned int				m_nPSM;
		uint32						m_nCLUTAddress;
		uint64						m_nTex0;
		uint64						m_nTexClut;
		bool						m_nIsCSM2;
		LPDIRECT3DTEXTURE9			m_texture;
		uint32						m_checksum;
		bool						m_live;
	};
	typedef std::list<CTexture*> TextureList;

	class CFramebuffer
	{
	public:
									CFramebuffer(DevicePtr&, uint32, uint32, uint32, uint32);
									~CFramebuffer();

		uint32						m_basePtr;
		uint32						m_width;
		uint32						m_height;
		uint32						m_psm;
		bool						m_canBeUsedAsTexture;

		TexturePtr					m_renderTarget;
		SurfacePtr					m_depthSurface;
	};
	typedef std::shared_ptr<CFramebuffer> FramebufferPtr;
	typedef std::vector<FramebufferPtr> FramebufferList;

	void							BeginScene();
	void							EndScene();
	void							RecreateDevice(int, int);
	void							PresentBackbuffer();

	FramebufferPtr					FindFramebuffer(uint64) const;
	void							GetFramebufferImpl(Framework::CBitmap&, uint64);

	void							SetReadCircuitMatrix(int, int);
	void							UpdateViewportImpl();
	void							VertexKick(uint8, uint64);

	void							SetRenderingContext(unsigned int);
	void							SetupBlendingFunction(uint64);
	void							SetupTestFunctions(uint64);
	void							SetupDepthBuffer(uint64);
	void							SetupTexture(uint64, uint64, uint64);
	void							SetupFramebuffer(uint64);
	LPDIRECT3DTEXTURE9				LoadTexture(TEX0*, TEX1*, CLAMP*);

	float							GetZ(float);
	uint8							MulBy2Clamp(uint8);

	void							Prim_Triangle();
	void							Prim_Sprite();

	void							DumpTexture(LPDIRECT3DTEXTURE9, uint32);
	void							TexUploader_Psm32(TEX0*, TEXA*, LPDIRECT3DTEXTURE9);
	void							UploadConversionBuffer(TEX0*, TEXA*, LPDIRECT3DTEXTURE9);

	void							ReadCLUT8(TEX0*);
	uint32							ConvertTexturePsm8(TEX0*, TEXA*);

	uint32							Color_Ps2ToDx9(uint32);

	LPDIRECT3DTEXTURE9				TexCache_SearchLive(TEX0*);
	LPDIRECT3DTEXTURE9				TexCache_SearchDead(TEX0*, uint32);
	void							TexCache_Insert(TEX0*, LPDIRECT3DTEXTURE9, uint32);
	void							TexCache_InvalidateTextures(uint32, uint32);
	void							TexCache_Flush();

	COutputWnd*						m_outputWnd;

	//Context variables (put this in a struct or something?)
	float							m_nPrimOfsX;
	float							m_nPrimOfsY;
	uint32							m_nTexWidth;
	uint32							m_nTexHeight;
	LPDIRECT3DTEXTURE9				m_nTexHandle;
	float							m_nMaxZ;

	uint8*							m_cvtBuffer;
	uint8*							m_clut;
	uint16*							m_clut16;
	uint32*							m_clut32;

	PRMODE							m_PrimitiveMode;
	unsigned int					m_nPrimitiveType;

	VERTEX							m_VtxBuffer[3];
	int								m_nVtxCount;

	TextureList						m_TexCache;
	FramebufferList					m_framebuffers;

	int								m_nWidth;
	int								m_nHeight;
	Direct3DPtr						m_d3d;
	DevicePtr						m_device;

	VertexBufferPtr					m_triangleVb;
	VertexBufferPtr					m_quadVb;
	bool							m_sceneBegun;
};
