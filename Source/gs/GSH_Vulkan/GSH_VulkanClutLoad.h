#pragma once

#include "GSH_VulkanContext.h"
#include "GSH_VulkanFrameCommandBuffer.h"
#include "GSH_VulkanPipelineCache.h"
#include "vulkan/ShaderModule.h"
#include "Convertible.h"
#include "../GSHandler.h"

namespace GSH_Vulkan
{
	class CClutLoad
	{
	public:
		CClutLoad(const ContextPtr&, const FrameCommandBufferPtr&);

		void DoClutLoad(uint32, const CGSHandler::TEX0&, const CGSHandler::TEXCLUT&);

	private:
		typedef uint32 PipelineCapsInt;

		struct PIPELINE_CAPS : public convertible<PipelineCapsInt>
		{
			uint32 idx8 : 1;
			uint32 csm : 1;
			uint32 cpsm : 4;
		};

		typedef CPipelineCache<PipelineCapsInt> PipelineCache;
		typedef std::unordered_map<uint32, VkDescriptorSet> DescriptorSetCache;

		struct LOAD_PARAMS
		{
			uint32 clutBufPtr = 0;
			uint32 clutBufWidth = 0;
			uint32 csa = 0;
			uint32 padding0 = 0;

			uint32 clutOffsetX = 0;
			uint32 clutOffsetY = 0;
			uint32 padding1 = 0;
			uint32 padding2 = 0;
		};
		static_assert(sizeof(LOAD_PARAMS) == 0x20, "LOAD_PARAMS must be 32 bytes large.");

		VkDescriptorSet PrepareDescriptorSet(VkDescriptorSetLayout, uint32);
		Framework::Vulkan::CShaderModule CreateLoadShader(const PIPELINE_CAPS&);
		PIPELINE CreateLoadPipeline(const PIPELINE_CAPS&);

		ContextPtr m_context;
		FrameCommandBufferPtr m_frameCommandBuffer;
		PipelineCache m_pipelines;
		DescriptorSetCache m_descriptorSetCache;
	};

	typedef std::shared_ptr<CClutLoad> ClutLoadPtr;
}
