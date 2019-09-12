#pragma once

#include <vulkan/vulkan.h>
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

	VkInstance m_instance = VK_NULL_HANDLE;
	VkSurfaceKHR m_surface = VK_NULL_HANDLE;

private:
	virtual void PresentBackbuffer() = 0;

	std::vector<VkPhysicalDevice> GetPhysicalDevices();
	void CreateDevice(VkPhysicalDevice);
	void CreateSwapChain(VkSurfaceFormatKHR, VkExtent2D);

	VkDevice m_device = VK_NULL_HANDLE;
	VkSwapchainKHR m_swapChain = VK_NULL_HANDLE;
	std::vector<VkImage> m_swapChainImages;
};
