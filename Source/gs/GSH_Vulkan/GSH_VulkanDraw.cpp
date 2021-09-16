#include "GSH_VulkanDraw.h"
#include "GSH_VulkanMemoryUtils.h"
#include "MemStream.h"
#include "vulkan/StructDefs.h"
#include "vulkan/Utils.h"
#include "nuanceur/Builder.h"
#include "nuanceur/generators/SpirvShaderGenerator.h"
#include "../GSHandler.h"
#include "../GsPixelFormats.h"

using namespace GSH_Vulkan;

#define VERTEX_ATTRIB_LOCATION_POSITION 0
#define VERTEX_ATTRIB_LOCATION_DEPTH 1
#define VERTEX_ATTRIB_LOCATION_COLOR 2
#define VERTEX_ATTRIB_LOCATION_TEXCOORD 3
#define VERTEX_ATTRIB_LOCATION_FOG 4

#define MAX_VERTEX_COUNT 1024 * 512

CDraw::CDraw(const ContextPtr& context, const FrameCommandBufferPtr& frameCommandBuffer)
    : m_context(context)
    , m_frameCommandBuffer(frameCommandBuffer)
    , m_pipelineCache(context->device)
{
	for(auto& frame : m_frames)
	{
		frame.vertexBuffer = Framework::Vulkan::CBuffer(
		    m_context->device, m_context->physicalDeviceMemoryProperties,
		    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		    sizeof(PRIM_VERTEX) * MAX_VERTEX_COUNT);

		auto result = m_context->device.vkMapMemory(m_context->device, frame.vertexBuffer.GetMemory(),
		                                            0, VK_WHOLE_SIZE, 0, reinterpret_cast<void**>(&frame.vertexBufferPtr));
		CHECKVULKANERROR(result);
	}

	m_pipelineCaps <<= 0;
}

CDraw::~CDraw()
{
	for(auto& frame : m_frames)
	{
		m_context->device.vkUnmapMemory(m_context->device, frame.vertexBuffer.GetMemory());
	}
}

void CDraw::SetPipelineCaps(const PIPELINE_CAPS& caps)
{
	bool changed = static_cast<uint64>(caps) != static_cast<uint64>(m_pipelineCaps);
	if(!changed) return;
	if(caps.textureUseMemoryCopy)
	{
		FlushRenderPass();
	}
	FlushVertices();
	m_pipelineCaps = caps;
}

void CDraw::SetFramebufferParams(uint32 addr, uint32 width, uint32 writeMask)
{
	bool changed =
	    (m_pushConstants.fbBufAddr != addr) ||
	    (m_pushConstants.fbBufWidth != width) ||
	    (m_pushConstants.fbWriteMask != writeMask);
	if(!changed) return;
	FlushVertices();
	m_pushConstants.fbBufAddr = addr;
	m_pushConstants.fbBufWidth = width;
	m_pushConstants.fbWriteMask = writeMask;
}

void CDraw::SetDepthbufferParams(uint32 addr, uint32 width)
{
	bool changed =
	    (m_pushConstants.depthBufAddr != addr) ||
	    (m_pushConstants.depthBufWidth != width);
	if(!changed) return;
	FlushVertices();
	m_pushConstants.depthBufAddr = addr;
	m_pushConstants.depthBufWidth = width;
}

void CDraw::SetTextureParams(uint32 bufAddr, uint32 bufWidth, uint32 width, uint32 height, uint32 csa)
{
	bool changed =
	    (m_pushConstants.texBufAddr != bufAddr) ||
	    (m_pushConstants.texBufWidth != bufWidth) ||
	    (m_pushConstants.texWidth != width) ||
	    (m_pushConstants.texHeight != height) ||
	    (m_pushConstants.texCsa != csa);
	if(!changed) return;
	FlushVertices();
	m_pushConstants.texBufAddr = bufAddr;
	m_pushConstants.texBufWidth = bufWidth;
	m_pushConstants.texWidth = width;
	m_pushConstants.texHeight = height;
	m_pushConstants.texCsa = csa;
}

void CDraw::SetClutBufferOffset(uint32 clutBufferOffset)
{
	bool changed = m_clutBufferOffset != clutBufferOffset;
	if(!changed) return;
	FlushVertices();
	m_clutBufferOffset = clutBufferOffset;
}

void CDraw::SetTextureAlphaParams(uint32 texA0, uint32 texA1)
{
	bool changed =
	    (m_pushConstants.texA0 != texA0) ||
	    (m_pushConstants.texA1 != texA1);
	if(!changed) return;
	FlushVertices();
	m_pushConstants.texA0 = texA0;
	m_pushConstants.texA1 = texA1;
}

void CDraw::SetTextureClampParams(uint32 clampMinU, uint32 clampMinV, uint32 clampMaxU, uint32 clampMaxV)
{
	bool changed =
	    (m_pushConstants.clampMin[0] != clampMinU) ||
	    (m_pushConstants.clampMin[1] != clampMinV) ||
	    (m_pushConstants.clampMax[0] != clampMaxU) ||
	    (m_pushConstants.clampMax[1] != clampMaxV);
	if(!changed) return;
	FlushVertices();
	m_pushConstants.clampMin[0] = clampMinU;
	m_pushConstants.clampMin[1] = clampMinV;
	m_pushConstants.clampMax[0] = clampMaxU;
	m_pushConstants.clampMax[1] = clampMaxV;
}

void CDraw::SetFogParams(float fogR, float fogG, float fogB)
{
	bool changed =
	    (m_pushConstants.fogColor[0] != fogR) ||
	    (m_pushConstants.fogColor[1] != fogG) ||
	    (m_pushConstants.fogColor[2] != fogB);
	if(!changed) return;
	FlushVertices();
	m_pushConstants.fogColor[0] = fogR;
	m_pushConstants.fogColor[1] = fogG;
	m_pushConstants.fogColor[2] = fogB;
	m_pushConstants.fogColor[3] = 0;
}

void CDraw::SetAlphaTestParams(uint32 alphaRef)
{
	bool changed = (m_pushConstants.alphaRef != alphaRef);
	if(!changed) return;
	FlushVertices();
	m_pushConstants.alphaRef = alphaRef;
}

void CDraw::SetAlphaBlendingParams(uint32 alphaFix)
{
	bool changed =
	    (m_pushConstants.alphaFix != alphaFix);
	if(!changed) return;
	FlushVertices();
	m_pushConstants.alphaFix = alphaFix;
}

void CDraw::SetScissor(uint32 scissorX, uint32 scissorY, uint32 scissorWidth, uint32 scissorHeight)
{
	bool changed =
	    (m_scissorX != scissorX) ||
	    (m_scissorY != scissorY) ||
	    (m_scissorWidth != scissorWidth) ||
	    (m_scissorHeight != scissorHeight);
	if(!changed) return;
	FlushVertices();
	m_scissorX = scissorX;
	m_scissorY = scissorY;
	m_scissorWidth = scissorWidth;
	m_scissorHeight = scissorHeight;
}

void CDraw::SetMemoryCopyParams(uint32 memoryCopyAddress, uint32 memoryCopySize)
{
	bool changed =
	    (memoryCopyAddress != m_memoryCopyAddress) ||
	    (memoryCopySize != m_memoryCopySize);
	if(!changed) return;
	FlushRenderPass();
	m_memoryCopyAddress = memoryCopyAddress;
	m_memoryCopySize = memoryCopySize;
}

void CDraw::AddVertices(const PRIM_VERTEX* vertexBeginPtr, const PRIM_VERTEX* vertexEndPtr)
{
	auto amount = vertexEndPtr - vertexBeginPtr;
	if((m_passVertexEnd + amount) > MAX_VERTEX_COUNT)
	{
		m_frameCommandBuffer->Flush();
		assert((m_passVertexEnd + amount) <= MAX_VERTEX_COUNT);
	}
	if(m_pipelineCaps.textureUseMemoryCopy)
	{
		//Check if sprite we are about to add overlaps with current region
		//Some games use tiny sprites to do full screen effects that requires
		//to keep a copy of RAM for texture sampling:
		//- Metal Gear Solid 3
		//- MK: Shaolin Monks
		//- Tales of Legendia
		const auto topLeftCorner = vertexBeginPtr;
		const auto bottomRightCorner = vertexBeginPtr + 5;
		CGsSpriteRect rect(topLeftCorner->x, topLeftCorner->y, bottomRightCorner->x, bottomRightCorner->y);
		if(m_memoryCopyRegion.Intersects(rect))
		{
			FlushRenderPass();
		}
		else
		{
			m_memoryCopyRegion.Insert(rect);
		}
	}
	auto& frame = m_frames[m_frameCommandBuffer->GetCurrentFrame()];
	memcpy(frame.vertexBufferPtr + m_passVertexEnd, vertexBeginPtr, amount * sizeof(PRIM_VERTEX));
	m_passVertexEnd += amount;
}

void CDraw::PreFlushFrameCommandBuffer()
{
	FlushRenderPass();
}

void CDraw::PostFlushFrameCommandBuffer()
{
	m_passVertexStart = m_passVertexEnd = 0;
}

std::vector<VkVertexInputAttributeDescription> CDraw::GetVertexAttributes()
{
	std::vector<VkVertexInputAttributeDescription> vertexAttributes;

	{
		VkVertexInputAttributeDescription vertexAttributeDesc = {};
		vertexAttributeDesc.format = VK_FORMAT_R32G32_SFLOAT;
		vertexAttributeDesc.offset = offsetof(PRIM_VERTEX, x);
		vertexAttributeDesc.location = VERTEX_ATTRIB_LOCATION_POSITION;
		vertexAttributes.push_back(vertexAttributeDesc);
	}

	{
		VkVertexInputAttributeDescription vertexAttributeDesc = {};
		vertexAttributeDesc.format = VK_FORMAT_R32_UINT;
		vertexAttributeDesc.offset = offsetof(PRIM_VERTEX, z);
		vertexAttributeDesc.location = VERTEX_ATTRIB_LOCATION_DEPTH;
		vertexAttributes.push_back(vertexAttributeDesc);
	}

	{
		VkVertexInputAttributeDescription vertexAttributeDesc = {};
		vertexAttributeDesc.format = VK_FORMAT_R8G8B8A8_UNORM;
		vertexAttributeDesc.offset = offsetof(PRIM_VERTEX, color);
		vertexAttributeDesc.location = VERTEX_ATTRIB_LOCATION_COLOR;
		vertexAttributes.push_back(vertexAttributeDesc);
	}

	{
		VkVertexInputAttributeDescription vertexAttributeDesc = {};
		vertexAttributeDesc.format = VK_FORMAT_R32G32B32_SFLOAT;
		vertexAttributeDesc.offset = offsetof(PRIM_VERTEX, s);
		vertexAttributeDesc.location = VERTEX_ATTRIB_LOCATION_TEXCOORD;
		vertexAttributes.push_back(vertexAttributeDesc);
	}

	{
		VkVertexInputAttributeDescription vertexAttributeDesc = {};
		vertexAttributeDesc.format = VK_FORMAT_R32_SFLOAT;
		vertexAttributeDesc.offset = offsetof(PRIM_VERTEX, f);
		vertexAttributeDesc.location = VERTEX_ATTRIB_LOCATION_FOG;
		vertexAttributes.push_back(vertexAttributeDesc);
	}

	return vertexAttributes;
}

Framework::Vulkan::CShaderModule CDraw::CreateVertexShader(const PIPELINE_CAPS& caps)
{
	using namespace Nuanceur;

	auto b = CShaderBuilder();

	{
		//Vertex Inputs
		auto inputPosition = CFloat4Lvalue(b.CreateInput(Nuanceur::SEMANTIC_POSITION));
		auto inputDepth = CUint4Lvalue(b.CreateInputUint(Nuanceur::SEMANTIC_TEXCOORD, VERTEX_ATTRIB_LOCATION_DEPTH - 1));
		auto inputColor = CFloat4Lvalue(b.CreateInput(Nuanceur::SEMANTIC_TEXCOORD, VERTEX_ATTRIB_LOCATION_COLOR - 1));
		auto inputTexCoord = CFloat4Lvalue(b.CreateInput(Nuanceur::SEMANTIC_TEXCOORD, VERTEX_ATTRIB_LOCATION_TEXCOORD - 1));
		auto inputFog = CFloat4Lvalue(b.CreateInput(Nuanceur::SEMANTIC_TEXCOORD, VERTEX_ATTRIB_LOCATION_FOG - 1));

		//Outputs
		auto outputPosition = CFloat4Lvalue(b.CreateOutput(Nuanceur::SEMANTIC_SYSTEM_POSITION));
		auto outputPointSize = CFloatLvalue(b.CreateOutput(Nuanceur::SEMANTIC_SYSTEM_POINTSIZE));
		auto outputDepth = CFloat4Lvalue(b.CreateOutput(Nuanceur::SEMANTIC_TEXCOORD, 1));
		auto outputColor = CFloat4Lvalue(b.CreateOutput(Nuanceur::SEMANTIC_TEXCOORD, 2));
		auto outputTexCoord = CFloat4Lvalue(b.CreateOutput(Nuanceur::SEMANTIC_TEXCOORD, 3));
		auto outputFog = CFloat4Lvalue(b.CreateOutput(Nuanceur::SEMANTIC_TEXCOORD, 4));

		auto position = ((inputPosition->xy() + NewFloat2(b, 0.5f, 0.5f)) * NewFloat2(b, 2.f / DRAW_AREA_SIZE, 2.f / DRAW_AREA_SIZE) + NewFloat2(b, -1, -1));

		outputPosition = NewFloat4(position, NewFloat2(b, 0.f, 1.f));
		if(caps.primitiveType == PIPELINE_PRIMITIVE_POINT)
		{
			outputPointSize = NewFloat(b, 1.0f);
		}
		outputDepth = ToFloat(inputDepth) / NewFloat4(b, DEPTH_MAX, DEPTH_MAX, DEPTH_MAX, DEPTH_MAX);
		outputColor = inputColor->xyzw();
		outputTexCoord = inputTexCoord->xyzw();
		outputFog = inputFog->xyzw();
	}

	Framework::CMemStream shaderStream;
	Nuanceur::CSpirvShaderGenerator::Generate(shaderStream, b, Nuanceur::CSpirvShaderGenerator::SHADER_TYPE_VERTEX);
	shaderStream.Seek(0, Framework::STREAM_SEEK_SET);
	return Framework::Vulkan::CShaderModule(m_context->device, shaderStream);
}
