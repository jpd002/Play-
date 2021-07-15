#pragma once

#include <memory>
#include "GSH_VulkanContext.h"
#include "GSH_VulkanFrameCommandBuffer.h"
#include "GSH_VulkanPipelineCache.h"
#include "vulkan/ShaderModule.h"
#include "vulkan/Buffer.h"
#include "vulkan/Image.h"
#include "Convertible.h"
#include "../GsSpriteRegion.h"

namespace GSH_Vulkan
{
	class CDrawMobile : public IFrameCommandBufferWriter
	{
	public:
		enum
		{
			MAX_FRAMES = CFrameCommandBuffer::MAX_FRAMES,
		};

		typedef uint64 PipelineCapsInt;

		enum PIPELINE_PRIMITIVE_TYPE
		{
			PIPELINE_PRIMITIVE_TRIANGLE = 0,
			PIPELINE_PRIMITIVE_LINE = 1,
			PIPELINE_PRIMITIVE_POINT = 2,
		};

		struct PIPELINE_CAPS : public convertible<PipelineCapsInt>
		{
			uint32 primitiveType : 2;

			uint32 hasTexture : 1;
			uint32 textureUseMemoryCopy : 1;
			uint32 textureHasAlpha : 1;
			uint32 textureBlackIsTransparent : 1;
			uint32 textureFunction : 2;
			uint32 textureUseLinearFiltering : 1;
			uint32 texClampU : 2;
			uint32 texClampV : 2;

			uint32 hasFog : 1;

			uint32 maskColor : 1;
			uint32 writeDepth : 1;

			uint32 scanMask : 2;

			uint32 hasAlphaBlending : 1;
			uint32 alphaA : 2;
			uint32 alphaB : 2;
			uint32 alphaC : 2;
			uint32 alphaD : 2;

			uint32 depthTestFunction : 2;
			uint32 alphaTestFunction : 3;
			uint32 alphaTestFailAction : 2;

			uint32 hasDstAlphaTest : 1;
			uint32 dstAlphaTestRef : 1;

			uint32 textureFormat : 6;
			uint32 clutFormat : 6;
			uint32 framebufferFormat : 6;
			uint32 depthbufferFormat : 6;
		};
		static_assert(sizeof(PIPELINE_CAPS) <= sizeof(PipelineCapsInt), "PIPELINE_CAPS too big for PipelineCapsInt");

		struct PRIM_VERTEX
		{
			float x, y;
			uint32 z;
			uint32 color;
			float s, t, q;
			float f;
		};

		CDrawMobile(const ContextPtr&, const FrameCommandBufferPtr&);
		virtual ~CDrawMobile();

		void SetPipelineCaps(const PIPELINE_CAPS&);
		void SetFramebufferParams(uint32, uint32, uint32);
		void SetDepthbufferParams(uint32, uint32);
		void SetTextureParams(uint32, uint32, uint32, uint32, uint32);
		void SetClutBufferOffset(uint32);
		void SetTextureAlphaParams(uint32, uint32);
		void SetTextureClampParams(uint32, uint32, uint32, uint32);
		void SetFogParams(float, float, float);
		void SetAlphaBlendingParams(uint32);
		void SetAlphaTestParams(uint32);
		void SetScissor(uint32, uint32, uint32, uint32);
		void SetMemoryCopyParams(uint32, uint32);

		void AddVertices(const PRIM_VERTEX*, const PRIM_VERTEX*);
		void FlushVertices();
		void FlushRenderPass();

		void PreFlushFrameCommandBuffer() override;
		void PostFlushFrameCommandBuffer() override;

	private:
		struct FRAMECONTEXT
		{
			Framework::Vulkan::CBuffer vertexBuffer;
			PRIM_VERTEX* vertexBufferPtr = nullptr;
		};

		typedef uint32 DescriptorSetCapsInt;

		struct LOADSTORE_CAPS
		{
			uint32 framebufferFormat;
			uint32 depthbufferFormat;
		};

		struct DESCRIPTORSET_CAPS : public convertible<DescriptorSetCapsInt>
		{
			uint32 hasTexture : 1;
			uint32 textureFormat : 6;
			uint32 framebufferFormat : 6;
			uint32 depthbufferFormat : 6;
		};
		static_assert(sizeof(DESCRIPTORSET_CAPS) == sizeof(DescriptorSetCapsInt));
		typedef std::unordered_map<DescriptorSetCapsInt, VkDescriptorSet> DescriptorSetCache;

		typedef CPipelineCache<PipelineCapsInt> PipelineCache;

		struct LOAD_STORE_PIPELINE_PUSHCONSTANTS
		{
			//fbDepthParams
			uint32 fbBufAddr = 0;
			uint32 fbBufWidth = 0;
			uint32 depthBufAddr = 0;
			uint32 depthBufWidth = 0;
		};
		static_assert(sizeof(LOAD_STORE_PIPELINE_PUSHCONSTANTS) <= 128, "Push constants size can't exceed 128 bytes.");

		struct DRAW_PIPELINE_PUSHCONSTANTS
		{
			//fbDepthParams - Unused
			uint32 fbBufAddr = 0;
			uint32 fbBufWidth = 0;
			uint32 depthBufAddr = 0;
			uint32 depthBufWidth = 0;

			//texParams0
			uint32 texBufAddr = 0;
			uint32 texBufWidth = 0;
			uint32 texWidth = 0;
			uint32 texHeight = 0;

			//texParams1
			uint32 texCsa = 0;
			uint32 texA0 = 0;
			uint32 texA1 = 0;
			uint32 texParams1Reserved = 0;

			//texParams2
			uint32 clampMin[2];
			uint32 clampMax[2];

			//alphaFbParams
			uint32 fbWriteMask = 0;
			uint32 alphaFix = 0;
			uint32 alphaRef = 0;
			uint32 alphaFbParamsReserved = 0;

			//fogColor
			float fogColor[4];
		};
		static_assert(sizeof(DRAW_PIPELINE_PUSHCONSTANTS) <= 128, "Push constants size can't exceed 128 bytes.");

		VkDescriptorSet PrepareDescriptorSet(VkDescriptorSetLayout, const DESCRIPTORSET_CAPS&);

		void CreateFramebuffer();
		void CreateRenderPass();
		void CreateDrawImages();

		PIPELINE CreateDrawPipeline(const PIPELINE_CAPS&);
		Framework::Vulkan::CShaderModule CreateDrawVertexShader();
		Framework::Vulkan::CShaderModule CreateDrawFragmentShader(const PIPELINE_CAPS&);

		PIPELINE CreateLoadPipeline();
		PIPELINE CreateStorePipeline();
		Framework::Vulkan::CShaderModule CreateLoadStoreVertexShader();
		Framework::Vulkan::CShaderModule CreateLoadFragmentShader();
		Framework::Vulkan::CShaderModule CreateStoreFragmentShader();

		ContextPtr m_context;
		FrameCommandBufferPtr m_frameCommandBuffer;

		PipelineCache m_drawPipelineCache;
		DescriptorSetCache m_drawDescriptorSetCache;

		PIPELINE m_loadPipeline;
		PIPELINE m_storePipeline;

		VkRenderPass m_renderPass = VK_NULL_HANDLE;
		VkFramebuffer m_framebuffer = VK_NULL_HANDLE;

		FRAMECONTEXT m_frames[MAX_FRAMES];

		Framework::Vulkan::CImage m_drawColorImage;
		VkImageView m_drawColorImageView = VK_NULL_HANDLE;

		Framework::Vulkan::CImage m_drawDepthImage;
		VkImageView m_drawDepthImageView = VK_NULL_HANDLE;

		uint32 m_passVertexStart = 0;
		uint32 m_passVertexEnd = 0;
		bool m_renderPassBegun = false;

		PIPELINE_CAPS m_pipelineCaps;
		DRAW_PIPELINE_PUSHCONSTANTS m_drawPushConstants;
		LOAD_STORE_PIPELINE_PUSHCONSTANTS m_loadStorePushConstants;
		uint32 m_scissorX = 0;
		uint32 m_scissorY = 0;
		uint32 m_scissorWidth = 0;
		uint32 m_scissorHeight = 0;
		uint32 m_clutBufferOffset = 0;
	};

	typedef std::shared_ptr<CDrawMobile> DrawPtr;
}
