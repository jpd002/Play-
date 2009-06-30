#ifndef _GSH_DIRECTX9_H_
#define _GSH_DIRECTX9_H_

#include "../GSHandler.h"
#include "win32/Window.h"
#include "OutputWnd.h"
#include <list>
#ifdef _DEBUG
#define D3D_DEBUG_INFO
#endif
#include <d3d9.h>

class CGSH_DirectX9 : public CGSHandler
{
public:
									CGSH_DirectX9(Framework::Win32::CWindow*);
	virtual                         ~CGSH_DirectX9();

	void							ProcessImageTransfer(uint32, uint32);

    static FactoryFunction          GetFactoryFunction(Framework::Win32::CWindow*);

    virtual void                    InitializeImpl();
    virtual void                    ReleaseImpl();
	virtual void					FlipImpl();

protected:
	virtual void					WriteRegisterImpl(uint8, uint64);

private:
	enum MAXCACHE
	{
		MAXCACHE = 256,
	};

	enum CVTBUFFERSIZE
	{
		CVTBUFFERSIZE = 0x400000,
	};

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
		LPDIRECT3DTEXTURE9			m_texture;
        uint32                      m_checksum;
        bool                        m_live;
	};

    typedef std::list<CTexture*> TextureList;

	void							BeginScene();
	void							EndScene();
	void							ReCreateDevice(int, int);

	virtual void                    SetViewport(int, int);
	void							SetReadCircuitMatrix(int, int);
	void							UpdateViewportImpl();
	void							VertexKick(uint8, uint64);

	void							SetRenderingContext(unsigned int);
	void							SetupBlendingFunction(uint64);
	void							SetupTexture(uint64, uint64, uint64);
	LPDIRECT3DTEXTURE9				LoadTexture(TEX0*, TEX1*, CLAMP*);

	void							Prim_Triangle();

    static CGSHandler*				GSHandlerFactory(Framework::Win32::CWindow*);

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

	COutputWnd*						m_pOutputWnd;

	//Context variables (put this in a struct or something?)
	float							m_nPrimOfsX;
	float							m_nPrimOfsY;
	uint32							m_nTexWidth;
	uint32							m_nTexHeight;
	LPDIRECT3DTEXTURE9				m_nTexHandle;

	uint8*							m_pCvtBuffer;
	void*							m_pCLUT;
	uint16*							m_pCLUT16;
	uint32*							m_pCLUT32;

	PRMODE							m_PrimitiveMode;
	unsigned int					m_nPrimitiveType;

	VERTEX							m_VtxBuffer[3];
	int								m_nVtxCount;

    TextureList                     m_TexCache;

	int								m_nWidth;
	int								m_nHeight;
	LPDIRECT3D9						m_d3d;
	LPDIRECT3DDEVICE9				m_device;

	LPDIRECT3DVERTEXBUFFER9			m_triangleVb;
	bool							m_sceneBegun;
};

#endif
