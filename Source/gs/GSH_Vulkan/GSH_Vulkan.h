#pragma once

#include "vulkan/VulkanDef.h"
#include "vulkan/Instance.h"
#include "vulkan/Device.h"
#include "vulkan/ShaderModule.h"
#include "vulkan/CommandBufferPool.h"
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
	void InitializeImpl() override;
	void ReleaseImpl() override;
	void ResetImpl() override;
	void NotifyPreferencesChangedImpl() override;
	void FlipImpl() override;

	Framework::Vulkan::CInstance m_instance;
	VkSurfaceKHR m_surface = VK_NULL_HANDLE;

private:
	virtual void PresentBackbuffer() = 0;
	void UpdateBackbuffer(uint32);

	std::vector<VkPhysicalDevice> GetPhysicalDevices();
	std::vector<uint32_t> GetRenderQueueFamilies(VkPhysicalDevice);
	std::vector<VkSurfaceFormatKHR> GetDeviceSurfaceFormats(VkPhysicalDevice);

	void CreateDevice(VkPhysicalDevice);
	void CreateSwapChain(VkSurfaceFormatKHR, VkExtent2D);
	void CreateSwapChainImageViews(VkFormat);
	void CreateSwapChainFramebuffers(VkRenderPass, VkExtent2D);

	Framework::Vulkan::CDevice m_device;
	Framework::Vulkan::CCommandBufferPool m_commandBufferPool;
	VkQueue m_queue = VK_NULL_HANDLE;
	VkExtent2D m_surfaceExtents;
	VkSwapchainKHR m_swapChain = VK_NULL_HANDLE;
	std::vector<VkImage> m_swapChainImages;
	std::vector<VkImageView> m_swapChainImageViews;
	std::vector<VkFramebuffer> m_swapChainFramebuffers;
	VkSemaphore m_imageAcquireSemaphore = VK_NULL_HANDLE;
	VkSemaphore m_renderCompleteSemaphore = VK_NULL_HANDLE;

	//Present Stuff
	struct PRESENT_VERTEX
	{
		float x, y;
	};

	void InitializePresent(VkFormat);
	void DestroyPresent();

	void CreatePresentRenderPass(VkFormat);
	void CreatePresentDrawPipeline();
	void CreatePresentVertexShader();
	void CreatePresentFragmentShader();

	VkRenderPass m_presentRenderPass = VK_NULL_HANDLE;
	VkPipelineLayout m_presentDrawPipelineLayout = VK_NULL_HANDLE;
	VkPipeline m_presentDrawPipeline = VK_NULL_HANDLE;
	Framework::Vulkan::CShaderModule m_presentVertexShader;
	Framework::Vulkan::CShaderModule m_presentFragmentShader;
};
