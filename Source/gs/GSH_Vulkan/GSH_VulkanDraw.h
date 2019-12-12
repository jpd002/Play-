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

			uint32 alphaA : 2;
			uint32 alphaB : 2;
			uint32 alphaC : 2;
			uint32 alphaD : 2;

			uint32 textureFormat : 6;
			uint32 framebufferFormat : 6;
			uint32 depthbufferFormat : 6;

			uint32 reserved;
		};

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
		void SetFramebufferBufferInfo(uint32, uint32);
		void SetTextureParams(uint32, uint32, uint32, uint32);

		void AddVertices(const PRIM_VERTEX*, const PRIM_VERTEX*);
		void FlushVertices();

		void PreFlushFrameCommandBuffer() override;
		void PostFlushFrameCommandBuffer() override;

	private:
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

		VkDescriptorSet PrepareDescriptorSet(VkDescriptorSetLayout);

		void CreateFramebuffer();
		void CreateRenderPass();
		void CreateDrawImage();

		PIPELINE CreateDrawPipeline(const PIPELINE_CAPS&);
		Framework::Vulkan::CShaderModule CreateVertexShader();
		Framework::Vulkan::CShaderModule CreateFragmentShader(const PIPELINE_CAPS&);

		ContextPtr m_context;
		FrameCommandBufferPtr m_frameCommandBuffer;
		PipelineCache m_pipelineCache;

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
	};

	typedef std::shared_ptr<CDraw> DrawPtr;
}
