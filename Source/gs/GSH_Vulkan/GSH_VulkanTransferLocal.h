#pragma once

#include <memory>
#include "GSH_VulkanContext.h"
#include "GSH_VulkanFrameCommandBuffer.h"
#include "GSH_VulkanPipelineCache.h"
#include "Convertible.h"
#include "vulkan/ShaderModule.h"
#include "nuanceur/Builder.h"

namespace GSH_Vulkan
{
	class CTransferLocal
	{
	public:
		typedef uint32 PipelineCapsInt;

		struct PIPELINE_CAPS : public convertible<PipelineCapsInt>
		{
			uint32 srcFormat : 6;
			uint32 dstFormat : 6;
		};

		struct XFERPARAMS
		{
			uint32 srcBufAddress = 0;
			uint32 srcBufWidth = 0;
			uint32 dstBufAddress = 0;
			uint32 dstBufWidth = 0;
			uint32 ssax = 0;
			uint32 ssay = 0;
			uint32 dsax = 0;
			uint32 dsay = 0;
			uint32 rrw = 0;
			uint32 rrh = 0;
			uint32 padding0 = 0;
			uint32 padding1 = 0;
		};
		static_assert(sizeof(XFERPARAMS) == 0x30, "XFERPARAMS must be 48 bytes large.");

		CTransferLocal(const ContextPtr&, const FrameCommandBufferPtr&);

		void SetPipelineCaps(const PIPELINE_CAPS&);

		void DoTransfer();

		XFERPARAMS Params;

	private:
		typedef CPipelineCache<PipelineCapsInt> PipelineCache;

		typedef uint32 DescriptorSetCapsInt;

		struct DESCRIPTORSET_CAPS : public convertible<DescriptorSetCapsInt>
		{
			uint32 srcPsm : 6;
			uint32 dstPsm : 6;
		};
		static_assert(sizeof(DESCRIPTORSET_CAPS) == sizeof(DescriptorSetCapsInt));
		typedef std::unordered_map<DescriptorSetCapsInt, VkDescriptorSet> DescriptorSetCache;

		VkDescriptorSet PrepareDescriptorSet(VkDescriptorSetLayout, const DESCRIPTORSET_CAPS&);

		Framework::Vulkan::CShaderModule CreateShader(const PIPELINE_CAPS&);
		PIPELINE CreatePipeline(const PIPELINE_CAPS&);

		ContextPtr m_context;
		FrameCommandBufferPtr m_frameCommandBuffer;
		PipelineCache m_pipelineCache;
		DescriptorSetCache m_descriptorSetCache;
		uint32 m_localSize = 0;

		PIPELINE_CAPS m_pipelineCaps;
	};

	typedef std::shared_ptr<CTransferLocal> TransferLocalPtr;
}
