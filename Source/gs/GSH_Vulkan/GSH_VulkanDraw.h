#pragma once

#include <memory>
#include <map>
#include "GSH_VulkanContext.h"
#include "vulkan/ShaderModule.h"
#include "vulkan/Buffer.h"
#include "Convertible.h"

namespace GSH_Vulkan
{
	class CDraw
	{
	public:
		typedef uint64 PipelineCapsInt;

		struct PIPELINE_CAPS : public convertible<PipelineCapsInt>
		{
			uint32 hasTexture : 1;

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

		CDraw(const ContextPtr&);
		virtual ~CDraw();

		void SetPipelineCaps(const PIPELINE_CAPS&);
		void SetFramebufferBufferInfo(uint32, uint32);
		void SetTextureParams(uint32, uint32, uint32, uint32);

		void AddVertices(const PRIM_VERTEX*, const PRIM_VERTEX*);
		void FlushVertices();

		void FlushCommands();

	private:
		struct DRAW_PIPELINE
		{
			VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
			VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
			VkPipeline pipeline = VK_NULL_HANDLE;
		};

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
		void StartRecording();

		void CreateFramebuffer();
		void CreateRenderPass();
		void CreateDrawImage();

		DRAW_PIPELINE CreateDrawPipeline(const PIPELINE_CAPS&);
		Framework::Vulkan::CShaderModule CreateVertexShader();
		Framework::Vulkan::CShaderModule CreateFragmentShader(const PIPELINE_CAPS&);

		ContextPtr m_context;

		VkRenderPass m_renderPass = VK_NULL_HANDLE;
		VkFramebuffer m_framebuffer = VK_NULL_HANDLE;

		std::map<PipelineCapsInt, DRAW_PIPELINE> m_drawPipelines;

		Framework::Vulkan::CBuffer m_vertexBuffer;
		PRIM_VERTEX* m_vertexBufferPtr = nullptr;

		VkImage m_drawImage = VK_NULL_HANDLE;
		VkDeviceMemory m_drawImageMemoryHandle = VK_NULL_HANDLE;
		VkImageView m_drawImageView = VK_NULL_HANDLE;

		VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;
		uint32 m_passVertexStart = 0;
		uint32 m_passVertexEnd = 0;

		PIPELINE_CAPS m_pipelineCaps;
		DRAW_PIPELINE_PUSHCONSTANTS m_pushConstants;
	};

	typedef std::shared_ptr<CDraw> DrawPtr;
}
