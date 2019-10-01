#pragma once

#include "vulkan/VulkanDef.h"
#include "vulkan/Instance.h"
#include "GSH_VulkanContext.h"
#include "GSH_VulkanDraw.h"
#include "GSH_VulkanPresent.h"
#include <vector>
#include "../GSHandler.h"
#include "../GsCachedArea.h"
#include "../GsTextureCache.h"

class CGSH_Vulkan : public CGSHandler
{
public:
	CGSH_Vulkan();
	virtual ~CGSH_Vulkan();

	virtual void LoadState(Framework::CZipArchiveReader&) override;

	void ProcessHostToLocalTransfer() override;
	void ProcessLocalToHostTransfer() override;
	void ProcessLocalToLocalTransfer() override;
	void ProcessClutTransfer(uint32, uint32) override;
	void ReadFramebuffer(uint32, uint32, void*) override;

	Framework::CBitmap GetScreenshot() override;

protected:
	void WriteRegisterImpl(uint8, uint64);
	void InitializeImpl() override;
	void ReleaseImpl() override;
	void ResetImpl() override;
	void NotifyPreferencesChangedImpl() override;
	void FlipImpl() override;

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

	virtual void PresentBackbuffer() = 0;

	std::vector<VkPhysicalDevice> GetPhysicalDevices();
	std::vector<uint32_t> GetRenderQueueFamilies(VkPhysicalDevice);
	std::vector<VkSurfaceFormatKHR> GetDeviceSurfaceFormats(VkPhysicalDevice);

	void CreateDevice(VkPhysicalDevice);
	void CreateDescriptorPool();
	void CreateMemoryImage();
	void InitMemoryImage();

	void VertexKick(uint8, uint64);
	void SetRenderingContext(uint64);

	void Prim_Triangle();

	GSH_Vulkan::DrawPtr m_draw;
	GSH_Vulkan::PresentPtr m_present;

	//Draw context
	VERTEX m_vtxBuffer[3];
	uint32 m_vtxCount = 0;
	uint32 m_primitiveType = 0;
	PRMODE m_primitiveMode;
	float m_primOfsX = 0;
	float m_primOfsY = 0;

	//GS memory
	VkImage m_memoryImage;
	VkDeviceMemory m_memoryImageMemoryHandle;
};
