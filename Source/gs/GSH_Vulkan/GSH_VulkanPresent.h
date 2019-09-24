#pragma once

#include <memory>
#include "GSH_VulkanContext.h"
#include "vulkan/ShaderModule.h"

namespace GSH_Vulkan
{
	class CPresent
	{
	public:
		CPresent(const ContextPtr&);
		virtual ~CPresent();

		void DoPresent();

	private:
		struct PRESENT_VERTEX
		{
			float x, y;
			float u, v;
		};

		void UpdateBackbuffer(uint32);
		void CreateSwapChain();
		void CreateSwapChainImageViews();
		void CreateSwapChainFramebuffers();
		void CreateRenderPass();
		void CreateDrawPipeline();
		void CreateVertexShader();
		void CreateFragmentShader();
		void CreateVertexBuffer();

		static const PRESENT_VERTEX g_vertexBufferContents[3];

		ContextPtr m_context;

		VkSwapchainKHR m_swapChain = VK_NULL_HANDLE;
		std::vector<VkImage> m_swapChainImages;
		std::vector<VkImageView> m_swapChainImageViews;
		std::vector<VkFramebuffer> m_swapChainFramebuffers;
		VkSemaphore m_imageAcquireSemaphore = VK_NULL_HANDLE;
		VkSemaphore m_renderCompleteSemaphore = VK_NULL_HANDLE;
		VkRenderPass m_renderPass = VK_NULL_HANDLE;
		VkDescriptorSetLayout m_drawDescriptorSetLayout = VK_NULL_HANDLE;
		VkPipelineLayout m_drawPipelineLayout = VK_NULL_HANDLE;
		VkPipeline m_drawPipeline = VK_NULL_HANDLE;
		VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
		VkDeviceMemory m_vertexBufferMemory = VK_NULL_HANDLE;
		Framework::Vulkan::CShaderModule m_vertexShader;
		Framework::Vulkan::CShaderModule m_fragmentShader;
	};

	typedef std::shared_ptr<CPresent> PresentPtr;
}
