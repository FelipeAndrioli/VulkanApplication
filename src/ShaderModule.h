#pragma once

#include <vector>
#include <stdexcept>
#include <string>
#include <fstream>

#include "Vulkan.h"
#include "LogicalDevice.h"

namespace Engine {
	class ShaderModule {
	public:
		ShaderModule(const char* shaderPath, LogicalDevice* logicalDevice, VkShaderStageFlagBits stage);
		~ShaderModule();
		
		inline VkPipelineShaderStageCreateInfo GetShaderStageInfo() { return m_ShaderStageInfo; };
	private:
		VkShaderModule createShaderModule(const std::vector<char>& code, LogicalDevice* p_LogicalDevice);
		std::vector<char> readFile(const std::string& filename);
	private:
		VkShaderModule m_ShaderModule;
		VkPipelineShaderStageCreateInfo m_ShaderStageInfo;

		LogicalDevice* p_LogicalDevice;
	};
}
