#pragma once

#include "framework.hpp"

namespace NativeJS
{
	class Window;
	class Logger;

	namespace GFX
	{
		class Renderer
		{
		private:
			constexpr static int MAX_FRAMES_IN_FLIGHT = 2;

			static std::vector<char> readFile(const std::string& filename);

			static VkResult createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
			static void destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

			static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
			void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

			struct QueueFamilyIndices
			{
				std::optional<uint32_t> graphicsFamily;
				std::optional<uint32_t> presentFamily;

				bool isComplete();
			};

			struct SwapChainSupportDetails
			{
				VkSurfaceCapabilitiesKHR capabilities;
				std::vector<VkSurfaceFormatKHR> formats;
				std::vector<VkPresentModeKHR> presentModes;

				bool isComplete();
			};

			void createInstance();
			bool checkValidationLayerSupport();
#ifdef _DEBUG
			void setupDebugMessenger();
#endif
			bool isDeviceSuitable(VkPhysicalDevice device);
			void createPysicalDevice();
			QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
			void createLogicalDevice();
			void createQueues();
			void createSurface();
			bool checkDeviceExtensionSupport(VkPhysicalDevice device);
			SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
			VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
			VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
			VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
			void createSwapChain();
			void createImageViews();
			void createGraphicsPipeline();
			void createFramebuffers();
			void createRenderPass();
			void createCommandPool();
			void createCommandBuffer();
			void createSyncObjects();

			VkShaderModule createShaderModule(const std::vector<char>& code);

			void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

			size_t getNextFrameIndex();

		public:
			Renderer(Window& window);
			~Renderer();

			void initialize();
			bool isInitialized();

			void render();

			Logger& logger;

		private:
			VkInstance instance_;
#ifdef _DEBUG
			VkDebugUtilsMessengerEXT debugMessenger_;
#endif
			VkPhysicalDevice physicalDev_;
			QueueFamilyIndices familyIndices_;
			VkDevice dev_;
			VkQueue graphicsQueue_;
			VkQueue presentQueue_;
			VkSurfaceKHR surface_;

			VkSwapchainKHR swapChain_;
			VkFormat swapChainImageFormat_;
			VkExtent2D swapChainExtent_;
			std::vector<VkImage> swapChainImages_;
			std::vector<VkImageView> swapChainImageViews_;
			std::vector<VkFramebuffer> swapChainFramebuffers_;

			VkRenderPass renderPass_;

			VkPipelineLayout pipelineLayout_;
			VkPipeline graphicsPipeline_;

			VkCommandPool commandPool_;
			VkCommandBuffer commandBuffer_[MAX_FRAMES_IN_FLIGHT];

			VkSemaphore imageAvailableSemaphore_[MAX_FRAMES_IN_FLIGHT];
			VkSemaphore renderFinishedSemaphore_[MAX_FRAMES_IN_FLIGHT];
			VkFence inFlightFence_[MAX_FRAMES_IN_FLIGHT];

			Window& window_;
			size_t currentFrame_;
			bool isInitialized_;
		};
	}
}