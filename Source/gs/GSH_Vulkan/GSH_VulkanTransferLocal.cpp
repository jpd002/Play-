#include "GSH_VulkanTransferLocal.h"
#include "GSH_VulkanMemoryUtils.h"
#include "MemStream.h"
#include "vulkan/StructDefs.h"
#include "nuanceur/generators/SpirvShaderGenerator.h"

using namespace GSH_Vulkan;

#define DESCRIPTOR_LOCATION_MEMORY 0
#define DESCRIPTOR_LOCATION_SWIZZLETABLE_SRC 1
#define DESCRIPTOR_LOCATION_SWIZZLETABLE_DST 2
#define DESCRIPTOR_LOCATION_MEMORY_8BIT 3
#define DESCRIPTOR_LOCATION_MEMORY_16BIT 4

#define LOCAL_SIZE_X 32
#define LOCAL_SIZE_Y 32

CTransferLocal::CTransferLocal(const ContextPtr& context, const FrameCommandBufferPtr& frameCommandBuffer)
    : m_context(context)
    , m_frameCommandBuffer(frameCommandBuffer)
    , m_pipelineCache(context->device)
{
	m_pipelineCaps <<= 0;
}

void CTransferLocal::SetPipelineCaps(const PIPELINE_CAPS& pipelineCaps)
{
	m_pipelineCaps = pipelineCaps;
}

void CTransferLocal::DoTransfer()
{
	//Find pipeline and create it if we've never encountered it before
	auto xferPipeline = m_pipelineCache.TryGetPipeline(m_pipelineCaps);
	if(!xferPipeline)
	{
		xferPipeline = m_pipelineCache.RegisterPipeline(m_pipelineCaps, CreatePipeline(m_pipelineCaps));
	}

	auto descriptorSetCaps = make_convertible<DESCRIPTORSET_CAPS>(0);
	descriptorSetCaps.srcPsm = m_pipelineCaps.srcFormat;
	descriptorSetCaps.dstPsm = m_pipelineCaps.dstFormat;

	auto descriptorSet = PrepareDescriptorSet(xferPipeline->descriptorSetLayout, descriptorSetCaps);
	auto commandBuffer = m_frameCommandBuffer->GetCommandBuffer();

	uint32 workUnitsX = (Params.rrw + LOCAL_SIZE_X - 1) / LOCAL_SIZE_X;
	uint32 workUnitsY = (Params.rrh + LOCAL_SIZE_Y - 1) / LOCAL_SIZE_Y;

	//Add a barrier to ensure reads are complete before writing to GS memory
	if(false)
	{
		auto memoryBarrier = Framework::Vulkan::MemoryBarrier();
		memoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;

		m_context->device.vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		                                       0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
	}

	m_context->device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, xferPipeline->pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
	m_context->device.vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, xferPipeline->pipeline);
	m_context->device.vkCmdPushConstants(commandBuffer, xferPipeline->pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(XFERPARAMS), &Params);
	m_context->device.vkCmdDispatch(commandBuffer, workUnitsX, workUnitsY, 1);
}

VkDescriptorSet CTransferLocal::PrepareDescriptorSet(VkDescriptorSetLayout descriptorSetLayout, const DESCRIPTORSET_CAPS& caps)
{
	auto descriptorSetIterator = m_descriptorSetCache.find(caps);
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

		VkDescriptorImageInfo descriptorSrcSwizzleTableInfo = {};
		descriptorSrcSwizzleTableInfo.imageView = m_context->GetSwizzleTable(caps.srcPsm);
		descriptorSrcSwizzleTableInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		VkDescriptorImageInfo descriptorDstSwizzleTableInfo = {};
		descriptorDstSwizzleTableInfo.imageView = m_context->GetSwizzleTable(caps.dstPsm);
		descriptorDstSwizzleTableInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

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

		//Memory Image Descriptor 8 bit
		if(false)
		{
			auto writeSet = Framework::Vulkan::WriteDescriptorSet();
			writeSet.dstSet = descriptorSet;
			writeSet.dstBinding = DESCRIPTOR_LOCATION_MEMORY_8BIT;
			writeSet.descriptorCount = 1;
			writeSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			writeSet.pBufferInfo = &descriptorMemoryBufferInfo;
			writes.push_back(writeSet);
		}

		//Memory Image Descriptor 16 bit
		if(false)
		{
			auto writeSet = Framework::Vulkan::WriteDescriptorSet();
			writeSet.dstSet = descriptorSet;
			writeSet.dstBinding = DESCRIPTOR_LOCATION_MEMORY_16BIT;
			writeSet.descriptorCount = 1;
			writeSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			writeSet.pBufferInfo = &descriptorMemoryBufferInfo;
			writes.push_back(writeSet);
		}

		//Src Swizzle Table
		{
			auto writeSet = Framework::Vulkan::WriteDescriptorSet();
			writeSet.dstSet = descriptorSet;
			writeSet.dstBinding = DESCRIPTOR_LOCATION_SWIZZLETABLE_SRC;
			writeSet.descriptorCount = 1;
			writeSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			writeSet.pImageInfo = &descriptorSrcSwizzleTableInfo;
			writes.push_back(writeSet);
		}

		//Dst Swizzle Table
		{
			auto writeSet = Framework::Vulkan::WriteDescriptorSet();
			writeSet.dstSet = descriptorSet;
			writeSet.dstBinding = DESCRIPTOR_LOCATION_SWIZZLETABLE_DST;
			writeSet.descriptorCount = 1;
			writeSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			writeSet.pImageInfo = &descriptorDstSwizzleTableInfo;
			writes.push_back(writeSet);
		}

		m_context->device.vkUpdateDescriptorSets(m_context->device, writes.size(), writes.data(), 0, nullptr);
	}

	m_descriptorSetCache.insert(std::make_pair(caps, descriptorSet));

	return descriptorSet;
}

Framework::Vulkan::CShaderModule CTransferLocal::CreateShader(const PIPELINE_CAPS& caps)
{
	using namespace Nuanceur;

	auto b = CShaderBuilder();

	b.SetMetadata(CShaderBuilder::METADATA_LOCALSIZE_X, LOCAL_SIZE_X);
	b.SetMetadata(CShaderBuilder::METADATA_LOCALSIZE_Y, LOCAL_SIZE_Y);

	{
		auto inputInvocationId = CInt4Lvalue(b.CreateInputInt(Nuanceur::SEMANTIC_SYSTEM_GIID));
		auto memoryBuffer = CArrayUintValue(b.CreateUniformArrayUint("memoryBuffer", DESCRIPTOR_LOCATION_MEMORY));
		//auto memoryBuffer8 = CArrayUcharValue(b.CreateUniformArrayUchar("memoryBuffer8", DESCRIPTOR_LOCATION_MEMORY_8BIT));
		//auto memoryBuffer16 = CArrayUshortValue(b.CreateUniformArrayUshort("memoryBuffer16", DESCRIPTOR_LOCATION_MEMORY_16BIT));
		auto srcSwizzleTable = CImageUint2DValue(b.CreateImage2DUint(DESCRIPTOR_LOCATION_SWIZZLETABLE_SRC));
		auto dstSwizzleTable = CImageUint2DValue(b.CreateImage2DUint(DESCRIPTOR_LOCATION_SWIZZLETABLE_DST));

		auto bufferParams = CInt4Lvalue(b.CreateUniformInt4("bufferParams", Nuanceur::UNIFORM_UNIT_PUSHCONSTANT));
		auto offsetParams = CInt4Lvalue(b.CreateUniformInt4("offsetParams", Nuanceur::UNIFORM_UNIT_PUSHCONSTANT));
		auto sizeParams = CInt4Lvalue(b.CreateUniformInt4("sizeParams", Nuanceur::UNIFORM_UNIT_PUSHCONSTANT));

		auto srcBufAddress = bufferParams->x();
		auto srcBufWidth = bufferParams->y();
		auto dstBufAddress = bufferParams->z();
		auto dstBufWidth = bufferParams->w();

		auto srcOffset = offsetParams->xy();
		auto dstOffset = offsetParams->zw();

		auto size = sizeParams->xy();

		BeginIf(b, inputInvocationId->x() >= size->x());
		{
			Return(b);
		}
		EndIf(b);

		BeginIf(b, inputInvocationId->y() >= size->y());
		{
			Return(b);
		}
		EndIf(b);

		auto srcPos = inputInvocationId->xy() + srcOffset;
		auto dstPos = inputInvocationId->xy() + dstOffset;

		auto pixel = CUintLvalue(b.CreateTemporaryUint());

		switch(caps.srcFormat)
		{
		case CGSHandler::PSMCT32:
		{
			auto address = CMemoryUtils::GetPixelAddress<CGsPixelFormats::STORAGEPSMCT32>(
			    b, srcSwizzleTable, srcBufAddress, srcBufWidth, srcPos);
			pixel = CMemoryUtils::Memory_Read32(b, memoryBuffer, address);
		}
		break;
		case CGSHandler::PSMCT16:
		{
			auto address = CMemoryUtils::GetPixelAddress<CGsPixelFormats::STORAGEPSMCT16>(
			    b, srcSwizzleTable, srcBufAddress, srcBufWidth, srcPos);
			pixel = CMemoryUtils::Memory_Read16(b, memoryBuffer, address);
		}
		break;
		case CGSHandler::PSMT8:
		{
			auto address = CMemoryUtils::GetPixelAddress<CGsPixelFormats::STORAGEPSMT8>(
			    b, srcSwizzleTable, srcBufAddress, srcBufWidth, srcPos);
			pixel = CMemoryUtils::Memory_Read8(b, memoryBuffer, address);
		}
		break;
		case CGSHandler::PSMT4:
		{
			auto texAddress = CMemoryUtils::GetPixelAddress_PSMT4(
			    b, srcSwizzleTable, srcBufAddress, srcBufWidth, srcPos);
			pixel = CMemoryUtils::Memory_Read4(b, memoryBuffer, texAddress);
		}
		break;
		default:
			assert(false);
			break;
		}

		switch(caps.dstFormat)
		{
		case CGSHandler::PSMCT32:
		{
			auto address = CMemoryUtils::GetPixelAddress<CGsPixelFormats::STORAGEPSMCT32>(
			    b, dstSwizzleTable, dstBufAddress, dstBufWidth, dstPos);
			CMemoryUtils::Memory_Write32(b, memoryBuffer, address, pixel);
		}
		break;
		case CGSHandler::PSMCT24:
		{
			auto dstPixel = pixel & NewUint(b, 0xFFFFFF);
			auto address = CMemoryUtils::GetPixelAddress<CGsPixelFormats::STORAGEPSMCT32>(
			    b, dstSwizzleTable, dstBufAddress, dstBufWidth, dstPos);
			CMemoryUtils::Memory_Write24(b, memoryBuffer, address, dstPixel);
		}
		break;
		case CGSHandler::PSMCT16:
		{
			auto address = CMemoryUtils::GetPixelAddress<CGsPixelFormats::STORAGEPSMCT16>(
			    b, dstSwizzleTable, dstBufAddress, dstBufWidth, dstPos);
			CMemoryUtils::Memory_Write16(b, memoryBuffer, address, pixel);
		}
		break;
		case CGSHandler::PSMT8:
		{
			auto address = CMemoryUtils::GetPixelAddress<CGsPixelFormats::STORAGEPSMT8>(
			    b, dstSwizzleTable, dstBufAddress, dstBufWidth, dstPos);
			CMemoryUtils::Memory_Write8(b, memoryBuffer, address, pixel);
		}
		break;
		case CGSHandler::PSMT4HL:
		{
			auto address = CMemoryUtils::GetPixelAddress<CGsPixelFormats::STORAGEPSMCT32>(
			    b, dstSwizzleTable, dstBufAddress, dstBufWidth, dstPos);
			auto nibAddress = (address + NewInt(b, 3)) * NewInt(b, 2);
			CMemoryUtils::Memory_Write4(b, memoryBuffer, nibAddress, pixel);
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

PIPELINE CTransferLocal::CreatePipeline(const PIPELINE_CAPS& caps)
{
	PIPELINE xferPipeline;

	auto xferShader = CreateShader(caps);

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

		//GS memory 8 bit
		if(false)
		{
			VkDescriptorSetLayoutBinding binding = {};
			binding.binding = DESCRIPTOR_LOCATION_MEMORY_8BIT;
			binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			binding.descriptorCount = 1;
			binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
			bindings.push_back(binding);
		}

		//GS memory 16 bit
		if(false)
		{
			VkDescriptorSetLayoutBinding binding = {};
			binding.binding = DESCRIPTOR_LOCATION_MEMORY_16BIT;
			binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			binding.descriptorCount = 1;
			binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
			bindings.push_back(binding);
		}

		//Src Swizzle Table
		{
			VkDescriptorSetLayoutBinding binding = {};
			binding.binding = DESCRIPTOR_LOCATION_SWIZZLETABLE_SRC;
			binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			binding.descriptorCount = 1;
			binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
			bindings.push_back(binding);
		}

		//Dst Swizzle Table
		{
			VkDescriptorSetLayoutBinding binding = {};
			binding.binding = DESCRIPTOR_LOCATION_SWIZZLETABLE_DST;
			binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			binding.descriptorCount = 1;
			binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
			bindings.push_back(binding);
		}

		auto createInfo = Framework::Vulkan::DescriptorSetLayoutCreateInfo();
		createInfo.bindingCount = bindings.size();
		createInfo.pBindings = bindings.data();
		result = m_context->device.vkCreateDescriptorSetLayout(m_context->device, &createInfo, nullptr, &xferPipeline.descriptorSetLayout);
		CHECKVULKANERROR(result);
	}

	{
		VkPushConstantRange pushConstantInfo = {};
		pushConstantInfo.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		pushConstantInfo.offset = 0;
		pushConstantInfo.size = sizeof(XFERPARAMS);

		auto pipelineLayoutCreateInfo = Framework::Vulkan::PipelineLayoutCreateInfo();
		pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
		pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantInfo;
		pipelineLayoutCreateInfo.setLayoutCount = 1;
		pipelineLayoutCreateInfo.pSetLayouts = &xferPipeline.descriptorSetLayout;

		result = m_context->device.vkCreatePipelineLayout(m_context->device, &pipelineLayoutCreateInfo, nullptr, &xferPipeline.pipelineLayout);
		CHECKVULKANERROR(result);
	}

	{
		auto createInfo = Framework::Vulkan::ComputePipelineCreateInfo();
		createInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		createInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		createInfo.stage.pName = "main";
		createInfo.stage.module = xferShader;
		createInfo.layout = xferPipeline.pipelineLayout;

		result = m_context->device.vkCreateComputePipelines(m_context->device, VK_NULL_HANDLE, 1, &createInfo, nullptr, &xferPipeline.pipeline);
		CHECKVULKANERROR(result);
	}

	return xferPipeline;
}
