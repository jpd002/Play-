#pragma once

#include <cfloat>
#include "GSH_VulkanDraw.h"

namespace GSH_Vulkan
{
	class CDrawMobile : public CDraw
	{
	public:
		CDrawMobile(const ContextPtr&, const FrameCommandBufferPtr&);
		virtual ~CDrawMobile();

		void SetPipelineCaps(const PIPELINE_CAPS&) override;
		void SetFramebufferParams(uint32, uint32, uint32) override;
		void SetDepthbufferParams(uint32, uint32) override;
		void SetScissor(uint32, uint32, uint32, uint32) override;

		void FlushVertices() override;
		void FlushRenderPass() override;

	private:
		VkDescriptorSet PrepareDescriptorSet(VkDescriptorSetLayout, const DESCRIPTORSET_CAPS&);

		void CreateFramebuffer();
		void CreateRenderPass();
		void CreateDrawImages();

		PIPELINE CreateDrawPipeline(const PIPELINE_CAPS&);
		Framework::Vulkan::CShaderModule CreateDrawFragmentShader(const PIPELINE_CAPS&);

		static PIPELINE_CAPS MakeLoadStorePipelineCaps(const PIPELINE_CAPS&);

		PIPELINE CreateLoadPipeline(const PIPELINE_CAPS&);
		PIPELINE CreateStorePipeline(const PIPELINE_CAPS&);
		Framework::Vulkan::CShaderModule CreateLoadStoreVertexShader();
		Framework::Vulkan::CShaderModule CreateLoadFragmentShader(const PIPELINE_CAPS&);
		Framework::Vulkan::CShaderModule CreateStoreFragmentShader(const PIPELINE_CAPS&);

		PipelineCache m_loadPipelineCache;
		PipelineCache m_storePipelineCache;

		VkRenderPass m_renderPass = VK_NULL_HANDLE;
		VkFramebuffer m_framebuffer = VK_NULL_HANDLE;

		Framework::Vulkan::CImage m_drawColorImage;
		VkImageView m_drawColorImageView = VK_NULL_HANDLE;

		Framework::Vulkan::CImage m_drawDepthImage;
		VkImageView m_drawDepthImageView = VK_NULL_HANDLE;

		float m_renderPassMinX = FLT_MAX;
		float m_renderPassMinY = FLT_MAX;
		float m_renderPassMaxX = -FLT_MAX;
		float m_renderPassMaxY = -FLT_MAX;
	};
}
