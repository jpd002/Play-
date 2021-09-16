#pragma once

#include "GSH_VulkanDraw.h"

namespace GSH_Vulkan
{
	class CDrawDesktop : public CDraw
	{
	public:
		CDrawDesktop(const ContextPtr&, const FrameCommandBufferPtr&);
		virtual ~CDrawDesktop();

		void FlushVertices() override;
		void FlushRenderPass() override;

	private:
		void CreateRenderPass();
		void CreateFramebuffer();
		void CreateDrawImage();

		PIPELINE CreateDrawPipeline(const PIPELINE_CAPS&);
		VkDescriptorSet PrepareDescriptorSet(VkDescriptorSetLayout, const DESCRIPTORSET_CAPS&);
		Framework::Vulkan::CShaderModule CreateFragmentShader(const PIPELINE_CAPS&);

		VkRenderPass m_renderPass = VK_NULL_HANDLE;
		VkFramebuffer m_framebuffer = VK_NULL_HANDLE;

		Framework::Vulkan::CImage m_drawImage;
		VkImageView m_drawImageView = VK_NULL_HANDLE;
	};
}
