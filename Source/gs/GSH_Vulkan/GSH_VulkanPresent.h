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

		void ValidateSwapChain(const CGSHandler::PRESENTATION_PARAMS&);
		void SetPresentationViewport(const CGSHandler::PRESENTATION_VIEWPORT&);
		void DoPresent(const CGSHandler::DISPLAY_INFO&);

	private:
		enum BLEND_MODE
		{
			BLEND_MODE_NONE,
			BLEND_MODE_SRC_ALPHA,
			BLEND_MODE_CST_ALPHA,
		};

		typedef uint32 PipelineCapsInt;

		struct PIPELINE_CAPS : public convertible<PipelineCapsInt>
		{
			uint32 bufPsm : 6;
			uint32 blendMode : 2;
		};

		typedef CPipelineCache<PipelineCapsInt> PipelineCache;
		typedef std::unordered_map<uint32, VkDescriptorSet> DescriptorSetCache;

		struct PRESENT_COMMANDBUFFER
		{
			VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
			VkFence execCompleteFence = VK_NULL_HANDLE;
		};
		typedef std::vector<PRESENT_COMMANDBUFFER> PresentCommandBufferArray;

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
			uint32 layerX;
			uint32 layerY;
			uint32 layerWidth;
			uint32 layerHeight;
		};

		void UpdateBackbuffer(uint32, const CGSHandler::DISPLAY_INFO&);
		PRESENT_COMMANDBUFFER PrepareCommandBuffer();
		VkDescriptorSet PrepareDescriptorSet(VkDescriptorSetLayout, uint32);

		void CreateSwapChain();
		void CreateSwapChainImageViews();
		void CreateSwapChainFramebuffers();
		void DestroySwapChain();

		void CreateRenderPass();
		void CreateVertexBuffer();

		PIPELINE CreateDrawPipeline(const PIPELINE_CAPS&);
		Framework::Vulkan::CShaderModule CreateVertexShader();
		Framework::Vulkan::CShaderModule CreateFragmentShader(const PIPELINE_CAPS&);

		static const PRESENT_VERTEX g_vertexBufferContents[4];

		ContextPtr m_context;

		VkExtent2D m_surfaceExtents;
		VkSwapchainKHR m_swapChain = VK_NULL_HANDLE;
		std::vector<VkImage> m_swapChainImages;
		std::vector<VkImageView> m_swapChainImageViews;
		std::vector<VkFramebuffer> m_swapChainFramebuffers;
		bool m_swapChainValid = false;
		CGSHandler::PRESENTATION_VIEWPORT m_presentationViewport;
		VkSemaphore m_imageAcquireSemaphore = VK_NULL_HANDLE;
		VkSemaphore m_renderCompleteSemaphore = VK_NULL_HANDLE;
		VkRenderPass m_renderPass = VK_NULL_HANDLE;
		Framework::Vulkan::CBuffer m_vertexBuffer;
		PresentCommandBufferArray m_presentCommandBuffers;
		PipelineCache m_pipelineCache;
		DescriptorSetCache m_descriptorSetCache;
	};

	typedef std::shared_ptr<CPresent> PresentPtr;
}
