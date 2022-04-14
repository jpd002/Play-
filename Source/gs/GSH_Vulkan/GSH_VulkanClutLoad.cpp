#include "GSH_VulkanClutLoad.h"
#include "GSH_VulkanMemoryUtils.h"
#include "../GsPixelFormats.h"
#include "MemStream.h"
#include "nuanceur/Builder.h"
#include "nuanceur/generators/SpirvShaderGenerator.h"
#include "vulkan/StructDefs.h"

using namespace GSH_Vulkan;

#define DESCRIPTOR_LOCATION_MEMORY 0
#define DESCRIPTOR_LOCATION_CLUT 1
#define DESCRIPTOR_LOCATION_SWIZZLETABLE 2

CClutLoad::CClutLoad(const ContextPtr& context, const FrameCommandBufferPtr& frameCommandBuffer)
    : m_context(context)
    , m_frameCommandBuffer(frameCommandBuffer)
    , m_pipelines(context->device)
{
}

void CClutLoad::DoClutLoad(uint32 clutBufferOffset, const CGSHandler::TEX0& tex0, const CGSHandler::TEXCLUT& texClut)
{
	auto caps = make_convertible<PIPELINE_CAPS>(0);
	caps.idx8 = CGsPixelFormats::IsPsmIDTEX8(tex0.nPsm) ? 1 : 0;
	caps.csm = tex0.nCSM;
	caps.cpsm = tex0.nCPSM;

	auto loadPipeline = m_pipelines.TryGetPipeline(caps);
	if(!loadPipeline)
	{
		loadPipeline = m_pipelines.RegisterPipeline(caps, CreateLoadPipeline(caps));
	}

	assert((caps.csm == 0) || (tex0.nCSA == 0));

	LOAD_PARAMS loadParams;
	loadParams.clutBufPtr = tex0.GetCLUTPtr();
	loadParams.clutBufWidth = (tex0.nCSM == 0) ? 0x40 : texClut.GetBufWidth();
	loadParams.csa = tex0.nCSA;
	loadParams.clutOffsetX = texClut.GetOffsetU();
	loadParams.clutOffsetY = texClut.GetOffsetV();

	auto descriptorSet = PrepareDescriptorSet(loadPipeline->descriptorSetLayout, tex0.nCPSM);
	auto commandBuffer = m_frameCommandBuffer->GetCommandBuffer();

	//Add a barrier to ensure writes to GS memory are complete
	{
		auto memoryBarrier = Framework::Vulkan::MemoryBarrier();
		memoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		m_context->device.vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		                                       0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
	}

	m_context->device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, loadPipeline->pipelineLayout, 0, 1, &descriptorSet, 1, &clutBufferOffset);
	m_context->device.vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, loadPipeline->pipeline);
	m_context->device.vkCmdPushConstants(commandBuffer, loadPipeline->pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(LOAD_PARAMS), &loadParams);
	m_context->device.vkCmdDispatch(commandBuffer, 1, 1, 1);
}

VkDescriptorSet CClutLoad::PrepareDescriptorSet(VkDescriptorSetLayout descriptorSetLayout, uint32 cpsm)
{
	auto descriptorSetIterator = m_descriptorSetCache.find(cpsm);
	if(descriptorSetIterator != std::end(m_descriptorSetCache))
	{
		return descriptorSetIterator->second;
	}

	VkResult result = VK_SUCCESS;
	VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

	//Allocate descriptor set
	{
		auto setAllocateInfo = Framework::Vulkan::DescriptorSetAllocateInfo();
		setAllocateInfo.descriptorPool = m_context->descriptorPool;
		setAllocateInfo.descriptorSetCount = 1;
		setAllocateInfo.pSetLayouts = &descriptorSetLayout;

		result = m_context->device.vkAllocateDescriptorSets(m_context->device, &setAllocateInfo, &descriptorSet);
		CHECKVULKANERROR(result);
	}

	//Update descriptor set
	{
		std::vector<VkWriteDescriptorSet> writes;

		VkDescriptorBufferInfo descriptorMemoryBufferInfo = {};
		descriptorMemoryBufferInfo.buffer = m_context->memoryBuffer;
		descriptorMemoryBufferInfo.range = VK_WHOLE_SIZE;

		VkDescriptorBufferInfo descriptorClutBufferInfo = {};
		descriptorClutBufferInfo.buffer = m_context->clutBuffer;
		descriptorClutBufferInfo.range = sizeof(uint32) * CGSHandler::CLUTENTRYCOUNT;

		VkDescriptorImageInfo descriptorSwizzleTableImageInfo = {};
		descriptorSwizzleTableImageInfo.imageView = m_context->GetSwizzleTable(cpsm);
		descriptorSwizzleTableImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		//Memory Image Descriptor
		{
			auto writeSet = Framework::Vulkan::WriteDescriptorSet();
			writeSet.dstSet = descriptorSet;
			writeSet.dstBinding = DESCRIPTOR_LOCATION_MEMORY;
			writeSet.descriptorCount = 1;
			writeSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			writeSet.pBufferInfo = &descriptorMemoryBufferInfo;
			writes.push_back(writeSet);
		}

		//CLUT Image Descriptor
		{
			auto writeSet = Framework::Vulkan::WriteDescriptorSet();
			writeSet.dstSet = descriptorSet;
			writeSet.dstBinding = DESCRIPTOR_LOCATION_CLUT;
			writeSet.descriptorCount = 1;
			writeSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
			writeSet.pBufferInfo = &descriptorClutBufferInfo;
			writes.push_back(writeSet);
		}

		//Swizzle Table Descriptor
		{
			auto writeSet = Framework::Vulkan::WriteDescriptorSet();
			writeSet.dstSet = descriptorSet;
			writeSet.dstBinding = DESCRIPTOR_LOCATION_SWIZZLETABLE;
			writeSet.descriptorCount = 1;
			writeSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			writeSet.pImageInfo = &descriptorSwizzleTableImageInfo;
			writes.push_back(writeSet);
		}

		m_context->device.vkUpdateDescriptorSets(m_context->device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
	}

	m_descriptorSetCache.insert(std::make_pair(cpsm, descriptorSet));

	return descriptorSet;
}

Framework::Vulkan::CShaderModule CClutLoad::CreateLoadShader(const PIPELINE_CAPS& caps)
{
	using namespace Nuanceur;

	auto b = CShaderBuilder();

	assert((caps.csm == 0) || (caps.cpsm == CGSHandler::PSMCT16));

	if(caps.csm == 0)
	{
		if(caps.idx8)
		{
			b.SetMetadata(CShaderBuilder::METADATA_LOCALSIZE_X, 16);
			b.SetMetadata(CShaderBuilder::METADATA_LOCALSIZE_Y, 16);
		}
		else
		{
			b.SetMetadata(CShaderBuilder::METADATA_LOCALSIZE_X, 8);
			b.SetMetadata(CShaderBuilder::METADATA_LOCALSIZE_Y, 2);
		}
	}
	else
	{
		b.SetMetadata(CShaderBuilder::METADATA_LOCALSIZE_X, (caps.idx8 == 0) ? 16 : 256);
		b.SetMetadata(CShaderBuilder::METADATA_LOCALSIZE_Y, 1);
	}

	{
		auto inputInvocationId = CInt4Lvalue(b.CreateInputInt(Nuanceur::SEMANTIC_SYSTEM_GIID));
		auto memoryBuffer = CArrayUintValue(b.CreateUniformArrayUint("memoryBuffer", DESCRIPTOR_LOCATION_MEMORY));
		auto clutBuffer = CArrayUintValue(b.CreateUniformArrayUint("clutBuffer", DESCRIPTOR_LOCATION_CLUT));
		auto swizzleTable = CImageUint2DValue(b.CreateImage2DUint(DESCRIPTOR_LOCATION_SWIZZLETABLE));

		auto loadParams1 = CInt4Lvalue(b.CreateUniformInt4("loadParams1", Nuanceur::UNIFORM_UNIT_PUSHCONSTANT));
		auto loadParams2 = CInt4Lvalue(b.CreateUniformInt4("loadParams2", Nuanceur::UNIFORM_UNIT_PUSHCONSTANT));

		auto clutBufPtr = loadParams1->x();
		auto clutBufWidth = loadParams1->y();
		auto csa = loadParams1->z();

		auto clutOffset = loadParams2->xy();

		auto colorPixel = CUintLvalue(b.CreateTemporaryUint());
		{
			auto colorPos = CInt2Lvalue(b.CreateTemporaryInt());
			if(caps.csm == 0)
			{
				colorPos = inputInvocationId->xy();
			}
			else
			{
				colorPos = clutOffset + NewInt2(inputInvocationId->x(), NewInt(b, 0));
			}

			switch(caps.cpsm)
			{
			case CGSHandler::PSMCT32:
			case CGSHandler::PSMCT24:
			{
				auto colorAddress = CMemoryUtils::GetPixelAddress<CGsPixelFormats::STORAGEPSMCT32>(
				    b, swizzleTable, clutBufPtr, clutBufWidth, colorPos);
				colorPixel = CMemoryUtils::Memory_Read32(b, memoryBuffer, colorAddress);
			}
			break;
			case CGSHandler::PSMCT16:
			case CGSHandler::PSMCT16S:
			{
				auto colorAddress = CMemoryUtils::GetPixelAddress<CGsPixelFormats::STORAGEPSMCT16>(
				    b, swizzleTable, clutBufPtr, clutBufWidth, colorPos);
				colorPixel = CMemoryUtils::Memory_Read16(b, memoryBuffer, colorAddress);
			}
			break;
			default:
				assert(false);
				break;
			}
		}

		auto clutIndex = CIntLvalue(b.CreateTemporaryInt());
		if(caps.csm == 0)
		{
			if(caps.idx8)
			{
				clutIndex = inputInvocationId->x() + (inputInvocationId->y() * NewInt(b, 16));
				clutIndex = (clutIndex & NewInt(b, ~0x18)) | ((clutIndex & NewInt(b, 0x08)) << NewInt(b, 1)) | ((clutIndex & NewInt(b, 0x10)) >> NewInt(b, 1));
			}
			else
			{
				clutIndex = inputInvocationId->x() + (inputInvocationId->y() * NewInt(b, 8));
				clutIndex = clutIndex + (csa * NewInt(b, 16));
			}
		}
		else
		{
			clutIndex = inputInvocationId->x();
		}

		switch(caps.cpsm)
		{
		case CGSHandler::PSMCT32:
		case CGSHandler::PSMCT24:
		{
			auto colorPixelLo = colorPixel & NewUint(b, 0xFFFF);
			auto colorPixelHi = (colorPixel >> NewUint(b, 16)) & NewUint(b, 0xFFFF);
			auto clutIndexLo = clutIndex;
			auto clutIndexHi = clutIndex + NewInt(b, 0x100);
			Store(clutBuffer, clutIndexLo, colorPixelLo);
			Store(clutBuffer, clutIndexHi, colorPixelHi);
		}
		break;
		case CGSHandler::PSMCT16:
		case CGSHandler::PSMCT16S:
		{
			Store(clutBuffer, clutIndex, colorPixel);
		}
		break;
		default:
			assert(false);
			break;
		}
	}

	Framework::CMemStream shaderStream;
	Nuanceur::CSpirvShaderGenerator::Generate(shaderStream, b, Nuanceur::CSpirvShaderGenerator::SHADER_TYPE_COMPUTE);
	shaderStream.Seek(0, Framework::STREAM_SEEK_SET);
	return Framework::Vulkan::CShaderModule(m_context->device, shaderStream);
}

PIPELINE CClutLoad::CreateLoadPipeline(const PIPELINE_CAPS& caps)
{
	PIPELINE loadPipeline;

	auto loadShader = CreateLoadShader(caps);

	VkResult result = VK_SUCCESS;

	{
		std::vector<VkDescriptorSetLayoutBinding> bindings;

		//GS memory
		{
			VkDescriptorSetLayoutBinding binding = {};
			binding.binding = DESCRIPTOR_LOCATION_MEMORY;
			binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			binding.descriptorCount = 1;
			binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
			bindings.push_back(binding);
		}

		//CLUT buffer
		{
			VkDescriptorSetLayoutBinding binding = {};
			binding.binding = DESCRIPTOR_LOCATION_CLUT;
			binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
			binding.descriptorCount = 1;
			binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
			bindings.push_back(binding);
		}

		//Swizzle table
		{
			VkDescriptorSetLayoutBinding binding = {};
			binding.binding = DESCRIPTOR_LOCATION_SWIZZLETABLE;
			binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			binding.descriptorCount = 1;
			binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
			bindings.push_back(binding);
		}

		auto createInfo = Framework::Vulkan::DescriptorSetLayoutCreateInfo();
		createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		createInfo.pBindings = bindings.data();
		result = m_context->device.vkCreateDescriptorSetLayout(m_context->device, &createInfo, nullptr, &loadPipeline.descriptorSetLayout);
		CHECKVULKANERROR(result);
	}

	{
		VkPushConstantRange pushConstantInfo = {};
		pushConstantInfo.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		pushConstantInfo.offset = 0;
		pushConstantInfo.size = sizeof(LOAD_PARAMS);

		auto pipelineLayoutCreateInfo = Framework::Vulkan::PipelineLayoutCreateInfo();
		pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
		pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantInfo;
		pipelineLayoutCreateInfo.setLayoutCount = 1;
		pipelineLayoutCreateInfo.pSetLayouts = &loadPipeline.descriptorSetLayout;

		result = m_context->device.vkCreatePipelineLayout(m_context->device, &pipelineLayoutCreateInfo, nullptr, &loadPipeline.pipelineLayout);
		CHECKVULKANERROR(result);
	}

	{
		auto createInfo = Framework::Vulkan::ComputePipelineCreateInfo();
		createInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		createInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		createInfo.stage.pName = "main";
		createInfo.stage.module = loadShader;
		createInfo.layout = loadPipeline.pipelineLayout;

		result = m_context->device.vkCreateComputePipelines(m_context->device, VK_NULL_HANDLE, 1, &createInfo, nullptr, &loadPipeline.pipeline);
		CHECKVULKANERROR(result);
	}

	return loadPipeline;
}
