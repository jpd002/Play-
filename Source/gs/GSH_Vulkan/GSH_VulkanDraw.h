#pragma once

#include <memory>
#include "GSH_VulkanContext.h"
#include "GSH_VulkanFrameCommandBuffer.h"
#include "GSH_VulkanPipelineCache.h"
#include "vulkan/ShaderModule.h"
#include "vulkan/Buffer.h"
#include "vulkan/Image.h"
#include "Convertible.h"

namespace GSH_Vulkan
{
	class CDraw : public IFrameCommandBufferWriter
	{
	public:
		typedef uint64 PipelineCapsInt;

		struct PIPELINE_CAPS : public convertible<PipelineCapsInt>
		{
			uint32 hasTexture : 1;
			uint32 hasAlphaBlending : 1;
			uint32 writeDepth : 1;

			uint32 alphaA : 2;
			uint32 alphaB : 2;
			uint32 alphaC : 2;
			uint32 alphaD : 2;

			uint32 depthTestFunction : 2;
			uint32 alphaTestFunction : 3;
			uint32 alphaTestFailAction : 2;

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
		};

		CDraw(const ContextPtr&, const FrameCommandBufferPtr&);
		virtual ~CDraw();

		void SetPipelineCaps(const PIPELINE_CAPS&);
		void SetFramebufferParams(uint32, uint32);
		void SetDepthbufferParams(uint32, uint32);
		void SetTextureParams(uint32, uint32, uint32, uint32);
		void SetScissor(uint32, uint32, uint32, uint32);

		void AddVertices(const PRIM_VERTEX*, const PRIM_VERTEX*);
		void FlushVertices();

		void ResetDescriptorSets();

		void PreFlushFrameCommandBuffer() override;
		void PostFlushFrameCommandBuffer() override;

	private:
		typedef uint32 DescriptorSetCapsInt;

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

		struct DRAW_PIPELINE_PUSHCONSTANTS
		{
			float projMatrix[16];
			uint32 fbBufAddr = 0;
			uint32 fbBufWidth = 0;
			uint32 depthBufAddr = 0;
			uint32 depthBufWidth = 0;
			uint32 texBufAddr = 0;
			uint32 texBufWidth = 0;
			uint32 texWidth = 0;
			uint32 texHeight = 0;
		};
		static_assert(sizeof(DRAW_PIPELINE_PUSHCONSTANTS) <= 128, "Push constants size can't exceed 128 bytes.");

		VkDescriptorSet PrepareDescriptorSet(VkDescriptorSetLayout, const DESCRIPTORSET_CAPS&);

		void CreateFramebuffer();
		void CreateRenderPass();
		void CreateDrawImage();

		PIPELINE CreateDrawPipeline(const PIPELINE_CAPS&);
		Framework::Vulkan::CShaderModule CreateVertexShader();
		Framework::Vulkan::CShaderModule CreateFragmentShader(const PIPELINE_CAPS&);

		ContextPtr m_context;
		FrameCommandBufferPtr m_frameCommandBuffer;
		PipelineCache m_pipelineCache;
		DescriptorSetCache m_descriptorSetCache;

		VkRenderPass m_renderPass = VK_NULL_HANDLE;
		VkFramebuffer m_framebuffer = VK_NULL_HANDLE;

		Framework::Vulkan::CBuffer m_vertexBuffer;
		PRIM_VERTEX* m_vertexBufferPtr = nullptr;

		Framework::Vulkan::CImage m_drawImage;
		VkImageView m_drawImageView = VK_NULL_HANDLE;

		uint32 m_passVertexStart = 0;
		uint32 m_passVertexEnd = 0;

		PIPELINE_CAPS m_pipelineCaps;
		DRAW_PIPELINE_PUSHCONSTANTS m_pushConstants;
		uint32 m_scissorX = 0;
		uint32 m_scissorY = 0;
		uint32 m_scissorWidth = 0;
		uint32 m_scissorHeight = 0;
	};

	typedef std::shared_ptr<CDraw> DrawPtr;
}
