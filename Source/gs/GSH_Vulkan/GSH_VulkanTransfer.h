#pragma once

#include <memory>
#include "GSH_VulkanContext.h"
#include "vulkan/ShaderModule.h"

namespace GSH_Vulkan
{
	class CTransfer
	{
	public:
		typedef std::vector<uint8> XferBuffer;

		struct XFERPARAMS
		{
			uint32 bufAddress = 0;
			uint32 bufWidth = 0;
			uint32 rrw = 0;
			uint32 dsax = 0;
			uint32 dsay = 0;
		};

		CTransfer(const ContextPtr&);
		virtual ~CTransfer();

		void DoHostToLocalTransfer(const XferBuffer&);

		XFERPARAMS Params;

	private:
		VkDescriptorSet PrepareDescriptorSet();

		void CreateXferShader();
		void CreateXferBuffer();
		void CreatePipeline();

		ContextPtr m_context;

		Framework::Vulkan::CShaderModule m_xferShader;
		VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
		VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
		VkPipeline m_pipeline = VK_NULL_HANDLE;
		VkBuffer m_xferBuffer = VK_NULL_HANDLE;
		VkDeviceMemory m_xferBufferMemory = VK_NULL_HANDLE;
	};

	typedef std::shared_ptr<CTransfer> TransferPtr;
}
