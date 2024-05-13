#include "ShaderModule.h"

namespace Engine {
	ShaderModule::ShaderModule(const char* shaderPath, LogicalDevice& logicalDevice, VkShaderStageFlagBits stage) : m_LogicalDevice(logicalDevice) {
		auto shaderCode = readFile(shaderPath);

		m_ShaderModule = createShaderModule(shaderCode, m_LogicalDevice);

		m_ShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		m_ShaderStageInfo.stage = stage;
		m_ShaderStageInfo.module = m_ShaderModule;
		m_ShaderStageInfo.pName = "main";
	}

	ShaderModule::~ShaderModule() {
		if (m_ShaderModule) {
			vkDestroyShaderModule(m_LogicalDevice.GetHandle(), m_ShaderModule, nullptr);
		}
	}

	VkShaderModule ShaderModule::createShaderModule(const std::vector<char>& code, LogicalDevice& m_LogicalDevice) {
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(m_LogicalDevice.GetHandle(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create shader module!");
		}

		return shaderModule;
	}

	std::vector<char> ShaderModule::readFile(const std::string& filename) {
		std::ifstream file(filename, std::ios::ate | std::ios::binary);

		if (!file.is_open()) {
			throw std::runtime_error("Failed to open file!");
		}

		size_t fileSize = (size_t)file.tellg();
		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);

		file.close();

		return buffer;
	}

}
