#pragma once

#include <algorithm>

#include "Vulkan.h"
#include "Common.h"
#include "PhysicalDevice.h"
#include "LogicalDevice.h"
#include "Window.h"

namespace Engine {
	class SwapChain {
	public:
		SwapChain(PhysicalDevice *physicalDevice, Window* window, LogicalDevice* logicalDevice, 
			VkSurfaceKHR& surface);
		~SwapChain();
	
		void ReCreate();
		void CleanUp();

		inline VkSwapchainKHR& GetHandle() { return m_SwapChain; };
		inline std::vector<VkImage> GetSwapChainImages() { return m_SwapChainImages; };
		inline VkFormat GetSwapChainImageFormat() { return m_SwapChainImageFormat; };
		inline VkExtent2D GetSwapChainExtent() { return m_SwapChainExtent; };
		inline std::vector<VkImageView> GetSwapChainImageViews() { return m_SwapChainImageViews; };

	private:
		void CreateSwapChain();
		void CreateImageViews();

		VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
		VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	private:
		VkSwapchainKHR m_SwapChain;
		std::vector<VkImage> m_SwapChainImages;
		VkFormat m_SwapChainImageFormat;
		VkExtent2D m_SwapChainExtent;
		std::vector<VkImageView> m_SwapChainImageViews;

		PhysicalDevice* p_PhysicalDevice;
		LogicalDevice* p_LogicalDevice;
		Window* p_Window;

		VkSurfaceKHR& p_Surface;
	};
}
