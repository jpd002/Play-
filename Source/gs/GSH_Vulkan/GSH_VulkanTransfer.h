#pragma once

#include <memory>
#include <map>
#include "GSH_VulkanContext.h"
#include "GSH_VulkanPipelineCache.h"
#include "Convertible.h"
#include "vulkan/ShaderModule.h"
#include "nuanceur/Builder.h"

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
		typedef CPipelineCache<PipelineCapsInt> PipelineCache;

		VkDescriptorSet PrepareDescriptorSet(VkDescriptorSetLayout);

		void CreateXferBuffer();
		Framework::Vulkan::CShaderModule CreateXferShader(const PIPELINE_CAPS&);
		PIPELINE CreateXferPipeline(const PIPELINE_CAPS&);

		Nuanceur::CUintRvalue XferStream_Read32(Nuanceur::CShaderBuilder&, Nuanceur::CArrayUintValue, Nuanceur::CIntValue);
		Nuanceur::CUintRvalue XferStream_Read8(Nuanceur::CShaderBuilder&, Nuanceur::CArrayUintValue, Nuanceur::CIntValue);

		ContextPtr m_context;
		PipelineCache m_pipelineCache;

		VkBuffer m_xferBuffer = VK_NULL_HANDLE;
		VkDeviceMemory m_xferBufferMemory = VK_NULL_HANDLE;

		PIPELINE_CAPS m_pipelineCaps;
	};

	typedef std::shared_ptr<CTransfer> TransferPtr;
}
