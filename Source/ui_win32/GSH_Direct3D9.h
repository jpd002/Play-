#pragma once

#include "../gs/GSHandler.h"
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

	void							ProcessImageTransfer() override;
	void							ProcessClutTransfer(uint32, uint32) override;
	void							ProcessLocalToLocalTransfer() override;
	void							ReadFramebuffer(uint32, uint32, void*) override;
	
	bool							GetDepthTestingEnabled() const;
	void							SetDepthTestingEnabled(bool);

	bool							GetAlphaBlendingEnabled() const;
	void							SetAlphaBlendingEnabled(bool);

	bool							GetAlphaTestingEnabled() const;
	void							SetAlphaTestingEnabled(bool);

	Framework::CBitmap				GetFramebuffer(uint64);
	Framework::CBitmap				GetTexture(uint64, uint64, uint64);
	const VERTEX*					GetInputVertices() const;

	static FactoryFunction			GetFactoryFunction(Framework::Win32::CWindow*);

protected:
	virtual void					ResetImpl() override;
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
		CVTBUFFERSIZE = 2048 * 2048 * 4,
	};

	struct RENDERSTATE
	{
		bool		isValid;
		uint64		primReg;
		uint64		frameReg;
		uint64		testReg;
		uint64		alphaReg;
		uint64		zbufReg;
		uint64		tex0Reg;
		uint64		tex1Reg;
		uint64		clampReg;
	};

	class CCachedTexture
	{
	public:
									CCachedTexture();
									~CCachedTexture();
		void						InvalidateFromMemorySpace(uint32, uint32);
		void						Free();

		uint32						m_nStart;
		uint32						m_nSize;
		uint32						m_nCLUTAddress;
		uint64						m_nTex0;
		uint64						m_nTexClut;
		bool						m_nIsCSM2;
		TexturePtr					m_texture;
		uint32						m_checksum;
		bool						m_live;
	};
	typedef std::shared_ptr<CCachedTexture> CachedTexturePtr;
	typedef std::list<CachedTexturePtr> CachedTextureList;

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
	};
	typedef std::shared_ptr<CFramebuffer> FramebufferPtr;
	typedef std::vector<FramebufferPtr> FramebufferList;

	class CDepthbuffer
	{
	public:
									CDepthbuffer(DevicePtr&, uint32, uint32, uint32, uint32);
									~CDepthbuffer();

		uint32						m_basePtr;
		uint32						m_width;
		uint32						m_height;
		uint32						m_psm;

		SurfacePtr					m_depthSurface;
	};
	typedef std::shared_ptr<CDepthbuffer> DepthbufferPtr;
	typedef std::vector<DepthbufferPtr> DepthbufferList;

	struct TEXTURE_INFO
	{
		TexturePtr		texture;
		bool			isRenderTarget = false;
		uint32			renderTargetWidth = 0;
		uint32			renderTargetHeight = 0;
	};

	void							BeginScene();
	void							EndScene();
	bool							TestDevice();
	void							CreateDevice();
	void							OnDeviceReset();
	void							OnDeviceResetting();
	D3DPRESENT_PARAMETERS			CreatePresentParams();
	void							PresentBackbuffer();

	FramebufferPtr					FindFramebuffer(uint64) const;
	void							GetFramebufferImpl(Framework::CBitmap&, uint64);

	DepthbufferPtr					FindDepthbuffer(uint64, uint64) const;

	void							SetReadCircuitMatrix(int, int);
	void							VertexKick(uint8, uint64);

	void							SetRenderingContext(uint64);
	void							SetupBlendingFunction(uint64);
	void							SetupTestFunctions(uint64);
	void							SetupDepthBuffer(uint64, uint64);
	void							SetupTexture(uint64, uint64, uint64);
	void							SetupFramebuffer(uint64);
	TEXTURE_INFO					LoadTexture(const TEX0&, const TEX1&, const CLAMP&);
	void							GetTextureImpl(Framework::CBitmap&, uint64, uint64, uint64);

	void							CopyTextureToBitmap(Framework::CBitmap&, const TexturePtr&, uint32, uint32);
	void							CopyRenderTargetToBitmap(Framework::CBitmap&, const TexturePtr&, uint32, uint32, uint32, uint32);

	float							GetZ(float);
	uint8							MulBy2Clamp(uint8);

	void							Prim_Line();
	void							Prim_Triangle();
	void							Prim_Sprite();

	void							TexUploader_Psm32(const TEX0&, const TEXA&, TexturePtr);
	void							UploadConversionBuffer(const TEX0&, const TEXA&, TexturePtr);

	uint32							ConvertTexturePsmct16(const TEX0&, const TEXA&);
	uint32							ConvertTexturePsm8(const TEX0&, const TEXA&);
	uint32							ConvertTexturePsm8H(const TEX0&, const TEXA&);
	uint32							ConvertTexturePsm4(const TEX0&, const TEXA&);
	template <uint32> uint32		ConvertTexturePsm4H(const TEX0&, const TEXA&);

	static uint32					Color_Ps2ToDx9(uint32);
	static uint32					RGBA16ToRGBA32(uint16);

	TexturePtr						TexCache_SearchLive(const TEX0&);
	TexturePtr						TexCache_SearchDead(const TEX0&, uint32);
	void							TexCache_Insert(const TEX0&, const TexturePtr&, uint32);
	void							TexCache_InvalidateTextures(uint32, uint32);
	void							TexCache_Flush();

	void							FlattenClut(const TEX0&, uint32*);

	bool							m_depthTestingEnabled = true;
	bool							m_alphaBlendingEnabled = true;
	bool							m_alphaTestingEnabled = true;

	COutputWnd*						m_outputWnd;
	Direct3DPtr						m_d3d;
	DevicePtr						m_device;

	//Context variables (put this in a struct or something?)
	float							m_nPrimOfsX;
	float							m_nPrimOfsY;
	TexturePtr						m_currentTexture;
	uint32							m_currentTextureWidth;
	uint32							m_currentTextureHeight;
	float							m_nMaxZ;

	uint8*							m_cvtBuffer;

	PRMODE							m_primitiveMode;
	unsigned int					m_primitiveType;

	VERTEX							m_vtxBuffer[3];
	int								m_vtxCount;

	CachedTextureList				m_cachedTextures;
	FramebufferList					m_framebuffers;
	DepthbufferList					m_depthbuffers;

	RENDERSTATE						m_renderState;

	VertexBufferPtr					m_triangleVb;
	VertexBufferPtr					m_quadVb;
	bool							m_sceneBegun;
};
