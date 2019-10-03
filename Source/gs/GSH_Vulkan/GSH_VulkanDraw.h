#pragma once

#include <memory>
#include <vector>
#include "GSH_VulkanContext.h"
#include "vulkan/ShaderModule.h"

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

		void AddVertices(const PRIM_VERTEX*, const PRIM_VERTEX*);
		void FlushVertices();

	private:
		struct VERTEX_SHADER_CONSTANTS
		{
			float projMatrix[16];
		};

		typedef std::vector<PRIM_VERTEX> PrimVertexArray;

		void UpdateVertexBuffer();

		void CreateFramebuffer();
		void CreateRenderPass();
		void CreateDrawPipeline();
		void CreateVertexBuffer();
		void CreateVertexShader();
		void CreateFragmentShader();

		ContextPtr m_context;

		VkRenderPass m_renderPass = VK_NULL_HANDLE;
		VkFramebuffer m_framebuffer = VK_NULL_HANDLE;
		VkDescriptorSetLayout m_drawDescriptorSetLayout = VK_NULL_HANDLE;
		VkPipelineLayout m_drawPipelineLayout = VK_NULL_HANDLE;
		VkPipeline m_drawPipeline = VK_NULL_HANDLE;
		Framework::Vulkan::CShaderModule m_vertexShader;
		Framework::Vulkan::CShaderModule m_fragmentShader;
		VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
		VkDeviceMemory m_vertexBufferMemory = VK_NULL_HANDLE;
		PrimVertexArray m_primVertices;

		VERTEX_SHADER_CONSTANTS m_vertexShaderConstants;
	};

	typedef std::shared_ptr<CDraw> DrawPtr;
}
