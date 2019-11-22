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
		void SetTextureBufferInfo(uint32, uint32);

		void AddVertices(const PRIM_VERTEX*, const PRIM_VERTEX*);
		void FlushVertices();

		void FlushCommands();

	private:
		enum
		{
			CMDBUF_FRAMEBUFFER_BUFFERINFO_SET = (1 << 0),
			CMDBUF_TEXTURE_BUFFERINFO_SET = (1 << 1)
		};

		struct DRAW_PIPELINE
		{
			VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
			VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
			VkPipeline pipeline = VK_NULL_HANDLE;
		};

		struct VERTEX_SHADER_CONSTANTS
		{
			float projMatrix[16];
		};

		struct BUFFERINFO_FRAMEBUFFER
		{
			uint32 addr = 0;
			uint32 width = 0;
		};

		struct BUFFERINFO_TEXTURE
		{
			uint32 addr = 0;
			uint32 width = 0;
		};

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

		Framework::Vulkan::CBuffer m_framebufferBufferInfoUniform;
		Framework::Vulkan::CBuffer m_textureBufferInfoUniform;
		Framework::Vulkan::CBuffer m_vertexBuffer;
		PRIM_VERTEX* m_vertexBufferPtr = nullptr;

		VkImage m_drawImage = VK_NULL_HANDLE;
		VkDeviceMemory m_drawImageMemoryHandle = VK_NULL_HANDLE;
		VkImageView m_drawImageView = VK_NULL_HANDLE;

		VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;
		uint32 m_commandBufferStatus = 0;
		uint32 m_passVertexStart = 0;
		uint32 m_passVertexEnd = 0;

		PIPELINE_CAPS m_pipelineCaps;
		VERTEX_SHADER_CONSTANTS m_vertexShaderConstants;
		BUFFERINFO_FRAMEBUFFER m_framebufferBufferInfo;
		BUFFERINFO_TEXTURE m_textureBufferInfo;
	};

	typedef std::shared_ptr<CDraw> DrawPtr;
}
