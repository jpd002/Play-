#pragma once

#include <memory>
#include <map>
#include "GSH_VulkanContext.h"
#include "Convertible.h"
#include "vulkan/ShaderModule.h"

namespace GSH_Vulkan
{
	class CTransfer
	{
	public:
		typedef uint32 PipelineCapsInt;
		typedef std::vector<uint8> XferBuffer;

		struct PIPELINE_CAPS : public convertible<PipelineCapsInt>
		{
			uint32 dstFormat : 6;
		};

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

		void SetPipelineCaps(const PIPELINE_CAPS&);

		void DoHostToLocalTransfer(const XferBuffer&);

		XFERPARAMS Params;

	private:
		struct XFER_PIPELINE
		{
			VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
			VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
			VkPipeline pipeline = VK_NULL_HANDLE;
		};

		VkDescriptorSet PrepareDescriptorSet(VkDescriptorSetLayout);

		void CreateXferBuffer();
		Framework::Vulkan::CShaderModule CreateXferShader(const PIPELINE_CAPS&);
		XFER_PIPELINE CreateXferPipeline(const PIPELINE_CAPS&);

		ContextPtr m_context;

		VkBuffer m_xferBuffer = VK_NULL_HANDLE;
		VkDeviceMemory m_xferBufferMemory = VK_NULL_HANDLE;
		std::map<PipelineCapsInt, XFER_PIPELINE> m_xferPipelines;

		PIPELINE_CAPS m_pipelineCaps;
	};

	typedef std::shared_ptr<CTransfer> TransferPtr;
}
