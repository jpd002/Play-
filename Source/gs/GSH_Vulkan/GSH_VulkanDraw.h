#pragma once

#include <memory>
#include "GSH_VulkanContext.h"
#include "vulkan/ShaderModule.h"
#include "vulkan/Buffer.h"

namespace GSH_Vulkan
{
	class CDraw
	{
	public:
		struct PRIM_VERTEX
		{
			float x, y;
			uint32 z;
			uint32 color;
		};

		CDraw(const ContextPtr&);
		virtual ~CDraw();

		void SetFramebuffer(uint32, uint32);

		void AddVertices(const PRIM_VERTEX*, const PRIM_VERTEX*);
		void FlushVertices();

		void FlushCommands();

	private:
		struct FRAMEBUFFER
		{
			uint32 bufAddress = 0;
			uint32 bufWidth = 0;
		};

		struct VERTEX_SHADER_CONSTANTS
		{
			float projMatrix[16];
		};

		VkDescriptorSet PrepareDescriptorSet();
		void StartRecording();

		void CreateFramebuffer();
		void CreateRenderPass();
		void CreateDrawPipeline();
		void CreateVertexShader();
		void CreateFragmentShader();
		void CreateDrawImage();

		ContextPtr m_context;

		VkRenderPass m_renderPass = VK_NULL_HANDLE;
		VkFramebuffer m_framebuffer = VK_NULL_HANDLE;
		VkDescriptorSetLayout m_drawDescriptorSetLayout = VK_NULL_HANDLE;
		VkPipelineLayout m_drawPipelineLayout = VK_NULL_HANDLE;
		VkPipeline m_drawPipeline = VK_NULL_HANDLE;
		Framework::Vulkan::CShaderModule m_vertexShader;
		Framework::Vulkan::CShaderModule m_fragmentShader;
		Framework::Vulkan::CBuffer m_frameBufferUniform;
		Framework::Vulkan::CBuffer m_vertexBuffer;
		PRIM_VERTEX* m_vertexBufferPtr = nullptr;

		VkImage m_drawImage = VK_NULL_HANDLE;
		VkDeviceMemory m_drawImageMemoryHandle = VK_NULL_HANDLE;
		VkImageView m_drawImageView = VK_NULL_HANDLE;

		VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;
		uint32 m_passVertexStart = 0;
		uint32 m_passVertexEnd = 0;

		FRAMEBUFFER m_fbBuffer;
		VERTEX_SHADER_CONSTANTS m_vertexShaderConstants;
	};

	typedef std::shared_ptr<CDraw> DrawPtr;
}
