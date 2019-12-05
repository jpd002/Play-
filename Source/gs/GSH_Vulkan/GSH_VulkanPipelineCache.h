#pragma once

#include "vulkan/Device.h"
#include <unordered_map>

namespace GSH_Vulkan
{
	struct PIPELINE
	{
		VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
		VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
		VkPipeline pipeline = VK_NULL_HANDLE;
	};

	template <typename KeyType>
	class CPipelineCache
	{
	public:
		CPipelineCache(Framework::Vulkan::CDevice& device)
			: m_device(&device)
		{
		
		}

		~CPipelineCache()
		{
			for(const auto& pipelinePair : m_pipelines)
			{
				auto& pipeline = pipelinePair.second;
				m_device->vkDestroyPipeline(*m_device, pipeline.pipeline, nullptr);
				m_device->vkDestroyPipelineLayout(*m_device, pipeline.pipelineLayout, nullptr);
				m_device->vkDestroyDescriptorSetLayout(*m_device, pipeline.descriptorSetLayout, nullptr);
			}
		}

		const PIPELINE* TryGetPipeline(const KeyType& key) const
		{
			auto pipelineIterator = m_pipelines.find(key);
			return (pipelineIterator == std::end(m_pipelines)) ? nullptr : &pipelineIterator->second;
		}

		const PIPELINE* RegisterPipeline(const KeyType& key, const PIPELINE& pipeline)
		{
			assert(TryGetPipeline(key) == nullptr);
			m_pipelines.insert(std::make_pair(key, pipeline));
			return TryGetPipeline(key);
		}

	private:
		typedef std::unordered_map<KeyType, PIPELINE> PipelineMap;

		const Framework::Vulkan::CDevice* m_device = nullptr;
		PipelineMap m_pipelines;
	};
}
