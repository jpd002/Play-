#pragma once

#include "../gs/GSHandler.h"
#include "../gs/GsTextureCache.h"
#include "win32/Window.h"
#include "win32/ComPtr.h"
#include "bitmap/Bitmap.h"
#include "OutputWnd.h"
#include "nuanceur/Builder.h"
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

	void							ProcessHostToLocalTransfer() override;
	void							ProcessLocalToHostTransfer() override;
	void							ProcessLocalToLocalTransfer() override;
	void							ProcessClutTransfer(uint32, uint32) override;
	void							ReadFramebuffer(uint32, uint32, void*) override;
	
	bool							GetDepthTestingEnabled() const;
	void							SetDepthTestingEnabled(bool);

	bool							GetAlphaBlendingEnabled() const;
	void							SetAlphaBlendingEnabled(bool);

	bool							GetAlphaTestingEnabled() const;
	void							SetAlphaTestingEnabled(bool);

	Framework::CBitmap				GetFramebuffer(uint64);
	Framework::CBitmap				GetTexture(uint64, uint32, uint64, uint64, uint32);
	const VERTEX*					GetInputVertices() const;

	static uint32					Color_Ps2ToDx9(uint32);

	static FactoryFunction			GetFactoryFunction(Framework::Win32::CWindow*);

protected:
	void							ResetImpl() override;
	void							InitializeImpl() override;
	void							ReleaseImpl() override;
	void							FlipImpl() override;

	void							WriteRegisterImpl(uint8, uint64) override;

private:
	typedef Framework::Win32::CComPtr<IDirect3D9> Direct3DPtr;
	typedef Framework::Win32::CComPtr<IDirect3DVertexBuffer9> VertexBufferPtr;
	typedef Framework::Win32::CComPtr<IDirect3DDevice9> DevicePtr;
	typedef Framework::Win32::CComPtr<IDirect3DTexture9> TexturePtr;
	typedef Framework::Win32::CComPtr<IDirect3DSurface9> SurfacePtr;
	typedef Framework::Win32::CComPtr<IDirect3DVertexDeclaration9> VertexDeclarationPtr;
	typedef Framework::Win32::CComPtr<IDirect3DVertexShader9> VertexShaderPtr;
	typedef Framework::Win32::CComPtr<IDirect3DPixelShader9> PixelShaderPtr;

	typedef std::map<uint32, VertexShaderPtr> VertexShaderMap;
	typedef std::map<uint32, PixelShaderPtr> PixelShaderMap;

	typedef void (CGSH_Direct3D9::*TEXTUREUPDATER)(D3DLOCKED_RECT*, uint32, uint32, unsigned int, unsigned int, unsigned int, unsigned int);

	enum CVTBUFFERSIZE
	{
		CVTBUFFERSIZE = 2048 * 2048 * 4,
	};

	enum
	{
		TEXTURE_SOURCE_MODE_NONE = 0,
		TEXTURE_SOURCE_MODE_STD  = 1,
		TEXTURE_SOURCE_MODE_IDX4 = 2,
		TEXTURE_SOURCE_MODE_IDX8 = 3
	};

	struct SHADERCAPS : public convertible<uint32>
	{
		uint32 texSourceMode : 2;
		uint32 padding       : 30;

		bool isIndexedTextureSource() const { return texSourceMode == TEXTURE_SOURCE_MODE_IDX4 || texSourceMode == TEXTURE_SOURCE_MODE_IDX8; }
	};
	static_assert(sizeof(SHADERCAPS) == sizeof(uint32), "SHADERCAPS structure size must be 4 bytes.");

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
		uint64		scissorReg;
	};

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
	void							DrawActiveFramebuffer();
	void							PresentBackbuffer();
	unsigned int					GetCurrentReadCircuit();

	FramebufferPtr					FindFramebuffer(uint64) const;
	Framework::CBitmap				GetFramebufferImpl(uint64);

	DepthbufferPtr					FindDepthbuffer(uint64, uint64) const;

	void							SetReadCircuitMatrix(int, int);
	void							VertexKick(uint8, uint64);

	void							FillShaderCapsFromTexture(SHADERCAPS&, const uint64&);

	void							SetRenderingContext(uint64);
	void							SetupBlendingFunction(uint64);
	void							SetupTestFunctions(uint64);
	void							SetupDepthBuffer(uint64, uint64);
	void							SetupTexture(uint64, uint64, uint64, uint64, uint64);
	void							SetupFramebuffer(uint64, uint64);
	TEXTURE_INFO					LoadTexture(const TEX0&, uint32, const MIPTBP1&, const MIPTBP2&);
	Framework::CBitmap				GetTextureImpl(uint64, uint32, uint64, uint64, uint32);

	TexturePtr						GetClutTexture(const TEX0&);

	Framework::CBitmap				CreateBitmapFromTexture(const TexturePtr&, uint32, uint32, uint8, uint32);
	Framework::CBitmap				CreateBitmapFromRenderTarget(const TexturePtr&, uint32, uint32, uint32, uint32);

	float							GetZ(float);

	void							Prim_Line();
	void							Prim_Triangle();
	void							Prim_Sprite();

	void							SetupTextureUpdaters();
	void							TexUpdater_Invalid(D3DLOCKED_RECT*, uint32, uint32, unsigned int, unsigned int, unsigned int, unsigned int);
	void							TexUpdater_Psm32(D3DLOCKED_RECT*, uint32, uint32, unsigned int, unsigned int, unsigned int, unsigned int);
	template <typename> void		TexUpdater_Psm48(D3DLOCKED_RECT*, uint32, uint32, unsigned int, unsigned int, unsigned int, unsigned int);

	VertexShaderPtr					CreateVertexShader(SHADERCAPS);
	PixelShaderPtr					CreatePixelShader(SHADERCAPS);

	Nuanceur::CShaderBuilder		GenerateVertexShader(SHADERCAPS);
	Nuanceur::CShaderBuilder		GeneratePixelShader(SHADERCAPS);

	bool							m_depthTestingEnabled = true;
	bool							m_alphaBlendingEnabled = true;
	bool							m_alphaTestingEnabled = true;

	COutputWnd*						m_outputWnd = nullptr;
	Direct3DPtr						m_d3d;
	DevicePtr						m_device;

	//Context variables (put this in a struct or something?)
	float							m_nPrimOfsX = 0;
	float							m_nPrimOfsY = 0;
	uint32							m_currentTextureWidth = 0;
	uint32							m_currentTextureHeight = 0;
	float							m_nMaxZ = 0;

	uint8*							m_cvtBuffer = nullptr;

	PRMODE							m_primitiveMode;
	unsigned int					m_primitiveType = 0;

	VERTEX							m_vtxBuffer[3];
	int								m_vtxCount = 0;

	TEXTUREUPDATER					m_textureUpdater[CGSHandler::PSM_MAX];

	CGsTextureCache<TexturePtr>		m_textureCache;
	FramebufferList					m_framebuffers;
	DepthbufferList					m_depthbuffers;
	TexturePtr						m_clutTexture;

	VertexDeclarationPtr			m_vertexDeclaration;
	VertexShaderMap					m_vertexShaders;
	PixelShaderMap					m_pixelShaders;

	RENDERSTATE						m_renderState;
	D3DMATRIX						m_projMatrix;

	VertexBufferPtr					m_drawVb;
	VertexBufferPtr					m_presentVb;
	bool							m_sceneBegun = false;

	uint32							m_deviceWindowWidth = 0;
	uint32							m_deviceWindowHeight = 0;
};
