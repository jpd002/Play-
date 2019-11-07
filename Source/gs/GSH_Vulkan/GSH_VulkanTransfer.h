#pragma once

#include <memory>
#include "GSH_VulkanContext.h"
#include "vulkan/ShaderModule.h"

namespace GSH_Vulkan
{
	class CTransfer
	{
	public:
		CTransfer(const ContextPtr&);
		virtual ~CTransfer();

		void DoHostToLocalTransfer();

	private:
		VkDescriptorSet PrepareDescriptorSet();

		void CreateXferShader();
		void CreatePipeline();

		ContextPtr m_context;

		Framework::Vulkan::CShaderModule m_xferShader;
		VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
		VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
		VkPipeline m_pipeline = VK_NULL_HANDLE;
		VkBuffer m_xferBuffer = VK_NULL_HANDLE;
	};

	typedef std::shared_ptr<CTransfer> TransferPtr;
}
