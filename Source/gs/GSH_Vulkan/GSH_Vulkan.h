#pragma once

#include "vulkan/VulkanDef.h"
#include "vulkan/Instance.h"
#include "vulkan/Image.h"
#include "GSH_VulkanContext.h"
#include "GSH_VulkanFrameCommandBuffer.h"
#include "GSH_VulkanClutLoad.h"
#include "GSH_VulkanDraw.h"
#include "GSH_VulkanPresent.h"
#include "GSH_VulkanTransferHost.h"
#include "GSH_VulkanTransferLocal.h"
#include <vector>
#include "../GSHandler.h"
#include "../GsCachedArea.h"
#include "../GsTextureCache.h"

class CGSH_Vulkan : public CGSHandler
{
public:
	CGSH_Vulkan();
	virtual ~CGSH_Vulkan() = default;

	static Framework::Vulkan::CInstance CreateInstance();

	void SetPresentationParams(const CGSHandler::PRESENTATION_PARAMS&) override;

	void ProcessHostToLocalTransfer() override;
	void ProcessLocalToHostTransfer() override;
	void ProcessLocalToLocalTransfer() override;
	void ProcessClutTransfer(uint32, uint32) override;
	void ReadFramebuffer(uint32, uint32, void*) override;

	uint8* GetRam() const override;

	Framework::CBitmap GetScreenshot() override;

protected:
	void WriteRegisterImpl(uint8, uint64);
	void InitializeImpl() override;
	void ReleaseImpl() override;
	void ResetImpl() override;
	void MarkNewFrame() override;
	void FlipImpl() override;
	void BeginTransferWrite() override;
	void TransferWrite(const uint8*, uint32) override;
	void SyncCLUT(const TEX0&) override;

	Framework::Vulkan::CInstance m_instance;
	GSH_Vulkan::ContextPtr m_context;

private:
	struct VERTEX
	{
		uint64 position;
		uint64 rgbaq;
		uint64 uv;
		uint64 st;
		uint8 fog;
	};

	struct CLUTKEY : public convertible<uint64>
	{
		uint32 idx4 : 1;
		uint32 cbp : 14;
		uint32 cpsm : 4;
		uint32 csm : 1;
		uint32 csa : 5;
		uint32 cbw : 6;
		uint32 cou : 6;
		uint32 cov : 10;
		uint32 reserved : 16;
	};
	static_assert(sizeof(CLUTKEY) == sizeof(uint64), "CLUTKEY too big for an uint64");

	enum
	{
		CLUT_CACHE_SIZE = 32,
	};

	virtual void PresentBackbuffer() = 0;

	std::vector<VkPhysicalDevice> GetPhysicalDevices();
	uint32 GetPhysicalDeviceIndex(const std::vector<VkPhysicalDevice>&) const;
	std::vector<uint32_t> GetRenderQueueFamilies(VkPhysicalDevice);
	std::vector<VkSurfaceFormatKHR> GetDeviceSurfaceFormats(VkPhysicalDevice);

	void CreateDevice(VkPhysicalDevice);
	void CreateDescriptorPool();
	void CreateMemoryBuffer();
	void CreateClutBuffer();

	void VertexKick(uint8, uint64);
	void SetRenderingContext(uint64);

	void Prim_Triangle();
	void Prim_Sprite();

	int32 FindCachedClut(const CLUTKEY&) const;
	static CLUTKEY MakeCachedClutKey(const TEX0&, const TEXCLUT&);

	GSH_Vulkan::FrameCommandBufferPtr m_frameCommandBuffer;
	GSH_Vulkan::ClutLoadPtr m_clutLoad;
	GSH_Vulkan::DrawPtr m_draw;
	GSH_Vulkan::PresentPtr m_present;
	GSH_Vulkan::TransferHostPtr m_transferHost;
	GSH_Vulkan::TransferLocalPtr m_transferLocal;

	uint8* m_memoryBufferPtr = nullptr;

	//Draw context
	VERTEX m_vtxBuffer[3];
	uint32 m_vtxCount = 0;
	uint32 m_primitiveType = 0;
	PRMODE m_primitiveMode;
	float m_primOfsX = 0;
	float m_primOfsY = 0;
	uint32 m_texWidth = 0;
	uint32 m_texHeight = 0;
	CLUTKEY m_clutStates[CLUT_CACHE_SIZE];
	uint32 m_nextClutCacheIndex = 0;
	std::vector<uint8> m_xferBuffer;

	Framework::Vulkan::CImage m_swizzleTablePSMCT32;
	Framework::Vulkan::CImage m_swizzleTablePSMCT16;
	Framework::Vulkan::CImage m_swizzleTablePSMCT16S;
	Framework::Vulkan::CImage m_swizzleTablePSMT8;
	Framework::Vulkan::CImage m_swizzleTablePSMT4;
	Framework::Vulkan::CImage m_swizzleTablePSMZ32;
	Framework::Vulkan::CImage m_swizzleTablePSMZ16;
};
