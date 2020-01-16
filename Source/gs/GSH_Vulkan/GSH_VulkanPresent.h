#pragma once

#include <memory>
#include "GSH_VulkanContext.h"
#include "GSH_VulkanPipelineCache.h"
#include "vulkan/ShaderModule.h"
#include "vulkan/Buffer.h"

namespace GSH_Vulkan
{
	class CPresent
	{
	public:
		CPresent(const ContextPtr&);
		virtual ~CPresent();

		void DoPresent(uint32, uint32, uint32, uint32, uint32);

	private:
		typedef std::unordered_map<uint32, VkDescriptorSet> DescriptorSetCache;

		typedef CPipelineCache<uint32> PipelineCache;

		struct PRESENT_VERTEX
		{
			float x, y;
			float u, v;
		};

		struct PRESENT_PARAMS
		{
			uint32 bufAddress;
			uint32 bufWidth;
			uint32 dispWidth;
			uint32 dispHeight;
		};

		void UpdateBackbuffer(uint32, uint32, uint32, uint32, uint32, uint32);
		VkDescriptorSet PrepareDescriptorSet(VkDescriptorSetLayout, uint32);

		void CreateSwapChain();
		void CreateSwapChainImageViews();
		void CreateSwapChainFramebuffers();
		void DestroySwapChain();

		void CreateRenderPass();
		void CreateVertexBuffer();

		PIPELINE CreateDrawPipeline(uint32);
		Framework::Vulkan::CShaderModule CreateVertexShader();
		Framework::Vulkan::CShaderModule CreateFragmentShader(uint32);

		static const PRESENT_VERTEX g_vertexBufferContents[3];

		ContextPtr m_context;

		VkExtent2D m_surfaceExtents;
		VkSwapchainKHR m_swapChain = VK_NULL_HANDLE;
		std::vector<VkImage> m_swapChainImages;
		std::vector<VkImageView> m_swapChainImageViews;
		std::vector<VkFramebuffer> m_swapChainFramebuffers;
		VkSemaphore m_imageAcquireSemaphore = VK_NULL_HANDLE;
		VkSemaphore m_renderCompleteSemaphore = VK_NULL_HANDLE;
		VkRenderPass m_renderPass = VK_NULL_HANDLE;
		Framework::Vulkan::CBuffer m_vertexBuffer;
		PipelineCache m_pipelineCache;
		DescriptorSetCache m_descriptorSetCache;
	};

	typedef std::shared_ptr<CPresent> PresentPtr;
}
