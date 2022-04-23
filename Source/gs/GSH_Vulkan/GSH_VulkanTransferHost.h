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
	class CTransferHost : public IFrameCommandBufferWriter
	{
	public:
		enum
		{
			MAX_FRAMES = CFrameCommandBuffer::MAX_FRAMES,
		};

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
			uint32 xferBufferOffset = 0;
			uint32 pixelCount = 0;
			uint32 padding2 = 0;
		};
		static_assert(sizeof(XFERPARAMS) == 0x20, "XFERPARAMS must be 32 bytes large.");

		CTransferHost(const ContextPtr&, const FrameCommandBufferPtr&);
		virtual ~CTransferHost();

		void SetPipelineCaps(const PIPELINE_CAPS&);

		void DoTransfer(const XferBuffer&);

		void PreFlushFrameCommandBuffer() override;
		void PostFlushFrameCommandBuffer() override;

		XFERPARAMS Params;

	private:
		struct FRAMECONTEXT
		{
			Framework::Vulkan::CBuffer xferBuffer;
			uint8* xferBufferPtr = nullptr;
		};

		typedef CPipelineCache<PipelineCapsInt> PipelineCache;

		typedef uint32 DescriptorSetCapsInt;

		struct DESCRIPTORSET_CAPS : public convertible<DescriptorSetCapsInt>
		{
			uint32 dstPsm : 6;
			uint32 frameIdx : 26;
		};
		static_assert(sizeof(DESCRIPTORSET_CAPS) == sizeof(DescriptorSetCapsInt));
		typedef std::unordered_map<DescriptorSetCapsInt, VkDescriptorSet> DescriptorSetCache;

		VkDescriptorSet PrepareDescriptorSet(VkDescriptorSetLayout, const DESCRIPTORSET_CAPS&);

		Framework::Vulkan::CShaderModule CreateXferShader(const PIPELINE_CAPS&);
		PIPELINE CreateXferPipeline(const PIPELINE_CAPS&);

		Nuanceur::CUintRvalue XferStream_Read32(Nuanceur::CShaderBuilder&, Nuanceur::CArrayUintValue, Nuanceur::CIntValue, Nuanceur::CIntValue);
		Nuanceur::CUintRvalue XferStream_Read24(Nuanceur::CShaderBuilder&, Nuanceur::CArrayUintValue, Nuanceur::CIntValue, Nuanceur::CIntValue);
		Nuanceur::CUintRvalue XferStream_Read16(Nuanceur::CShaderBuilder&, Nuanceur::CArrayUintValue, Nuanceur::CIntValue, Nuanceur::CIntValue);
		Nuanceur::CUintRvalue XferStream_Read8(Nuanceur::CShaderBuilder&, Nuanceur::CArrayUintValue, Nuanceur::CIntValue, Nuanceur::CIntValue);
		Nuanceur::CUintRvalue XferStream_Read4(Nuanceur::CShaderBuilder&, Nuanceur::CArrayUintValue, Nuanceur::CIntValue, Nuanceur::CIntValue);

		ContextPtr m_context;
		FrameCommandBufferPtr m_frameCommandBuffer;
		PipelineCache m_pipelineCache;
		DescriptorSetCache m_descriptorSetCache;
		uint32 m_localSize = 0;

		FRAMECONTEXT m_frames[MAX_FRAMES];

		uint32 m_xferBufferOffset = 0;

		PIPELINE_CAPS m_pipelineCaps;
	};

	typedef std::shared_ptr<CTransferHost> TransferHostPtr;
}
