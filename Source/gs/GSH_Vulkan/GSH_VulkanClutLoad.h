#pragma once

#include "GSH_VulkanContext.h"
#include "GSH_VulkanPipelineCache.h"
#include "vulkan/ShaderModule.h"
#include "Convertible.h"
#include "../GSHandler.h"

namespace GSH_Vulkan
{
	class CClutLoad
	{
	public:
		CClutLoad(const ContextPtr&);

		void DoClutLoad(const CGSHandler::TEX0&);

	private:
		typedef uint32 PipelineCapsInt;

		struct PIPELINE_CAPS : public convertible<PipelineCapsInt>
		{
			uint32 idx8 : 1;
			uint32 csm : 1;
			uint32 cpsm : 4;
		};

		typedef CPipelineCache<PipelineCapsInt> PipelineCache;

		struct LOAD_PARAMS
		{
			uint32 clutBufPtr;
			uint32 csa;
		};

		VkDescriptorSet PrepareDescriptorSet(VkDescriptorSetLayout);
		Framework::Vulkan::CShaderModule CreateLoadShader(const PIPELINE_CAPS&);
		PIPELINE CreateLoadPipeline(const PIPELINE_CAPS&);

		ContextPtr m_context;
		PipelineCache m_pipelines;
	};

	typedef std::shared_ptr<CClutLoad> ClutLoadPtr;
}
