#include "framework.hpp"
#include "gfx/Renderer.hpp"
#include "Window.hpp"
#include "App.hpp"
#include "Logger.hpp"

#define CHECK_VK_RESULT(_STMT_, _ERROR_) if(_STMT_ != VK_SUCCESS) { throw std::runtime_error(_ERROR_); }

namespace NativeJS
{
	namespace GFX
	{
		VkResult Renderer::createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
		{
			auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
			if (func != nullptr)
			{
				return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
			}
			else
			{
				return VK_ERROR_EXTENSION_NOT_PRESENT;
			}
		}

		void Renderer::destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
		{
			auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
			if (func != nullptr)

				func(instance, debugMessenger, pAllocator);

		}

		VKAPI_ATTR VkBool32 VKAPI_CALL Renderer::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
		{
			if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
				static_cast<Renderer*>(pUserData)->logger.debug(pCallbackData->pMessage);
			return VK_FALSE;
		}

		void Renderer::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
		{
			createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
			createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
			createInfo.pfnUserCallback = debugCallback;
			createInfo.pUserData = this;
		}

		bool Renderer::QueueFamilyIndices::isComplete()
		{
			return graphicsFamily.has_value() && presentFamily.has_value();
		}

		bool Renderer::SwapChainSupportDetails::isComplete()
		{
			return !formats.empty() && !presentModes.empty();
		}

		static const std::vector<const char*> validationLayers = {
#ifdef _DEBUG
			"VK_LAYER_KHRONOS_validation",
#endif
		};

		static const std::vector<const char*> deviceExtensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};

		std::vector<char> Renderer::readFile(const std::string& filename)
		{
			std::ifstream file(filename, std::ios::ate | std::ios::binary);

			if (!file.is_open())
				throw std::runtime_error("failed to open file!");

			size_t fileSize = (size_t)file.tellg();

			std::vector<char> buffer(fileSize);

			file.seekg(0);
			file.read(buffer.data(), fileSize);
			file.close();

			return buffer;
		}

		Renderer::Renderer(Window& window) :
			logger(window.windowManager().app().logger()),
			instance_(VK_NULL_HANDLE),
#ifdef _DEBUG
			debugMessenger_(VK_NULL_HANDLE),
#endif
			physicalDev_(VK_NULL_HANDLE),
			familyIndices_(),
			dev_(VK_NULL_HANDLE),
			graphicsQueue_(VK_NULL_HANDLE),
			presentQueue_(VK_NULL_HANDLE),
			surface_(VK_NULL_HANDLE),
			swapChain_(VK_NULL_HANDLE),
			swapChainImageFormat_(),
			swapChainExtent_(),
			swapChainImages_(),
			swapChainImageViews_(),
			swapChainFramebuffers_(),
			renderPass_(VK_NULL_HANDLE),
			pipelineLayout_(VK_NULL_HANDLE),
			graphicsPipeline_(VK_NULL_HANDLE),
			commandPool_(VK_NULL_HANDLE),
			commandBuffer_(),
			imageAvailableSemaphore_(),
			renderFinishedSemaphore_(),
			inFlightFence_(),
			window_(window),
			currentFrame_(0),
			isInitialized_(false)
		{

		}

		Renderer::~Renderer()
		{
			logger.debug("Waiting for vulkan device idle...");
			vkDeviceWaitIdle(dev_);

			logger.debug("Destroying sync objects...");
			for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
			{
				vkDestroySemaphore(dev_, imageAvailableSemaphore_[i], nullptr);
				vkDestroySemaphore(dev_, renderFinishedSemaphore_[i], nullptr);
				vkDestroyFence(dev_, inFlightFence_[i], nullptr);
			}

			logger.debug("Destroying command pool...");
			vkDestroyCommandPool(dev_, commandPool_, nullptr);

			logger.debug("Destroying frame buffers...");
			for (auto framebuffer : swapChainFramebuffers_)
				vkDestroyFramebuffer(dev_, framebuffer, nullptr);

			logger.debug("Destroying graphics pipeline...");
			vkDestroyPipeline(dev_, graphicsPipeline_, nullptr);

			logger.debug("Destroying graphics pipeline layout...");
			vkDestroyPipelineLayout(dev_, pipelineLayout_, nullptr);

			logger.debug("Destroying render passs...");
			vkDestroyRenderPass(dev_, renderPass_, nullptr);

			logger.debug("Destroying swapchain image views...");
			for (auto imageView : swapChainImageViews_)
				vkDestroyImageView(dev_, imageView, nullptr);

			logger.debug("Destroying swapchain...");
			vkDestroySwapchainKHR(dev_, swapChain_, nullptr);

			logger.debug("Destroying logical device...");
			vkDestroyDevice(dev_, nullptr);

			logger.debug("Destroying vulkan surface...");
			vkDestroySurfaceKHR(instance_, surface_, nullptr);

#ifdef _DEBUG
			destroyDebugUtilsMessengerEXT(instance_, debugMessenger_, nullptr);
#endif

			logger.debug("Destroying vulkan instance...");
			vkDestroyInstance(instance_, nullptr);
		}

		bool Renderer::isInitialized()
		{
			return isInitialized_;
		}

		void Renderer::initialize()
		{
			if (instance_ != VK_NULL_HANDLE)
				throw std::runtime_error("Renderer is already initialized!");

			createInstance();
#ifdef _DEBUG
			setupDebugMessenger();
#endif
			createSurface();
			createPysicalDevice();
			createLogicalDevice();
			createSwapChain();
			createImageViews();
			createRenderPass();
			createGraphicsPipeline();
			createFramebuffers();
			createCommandPool();
			createCommandBuffer();
			createSyncObjects();

			isInitialized_ = true;
		}

		void Renderer::createInstance()
		{
			logger.debug("Creating vulkan instance...");
#ifdef _DEBUG
			if (!checkValidationLayerSupport())
				throw std::runtime_error("Validation layers not available!");
#endif
			VkApplicationInfo appInfo = {};
			appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
			appInfo.pApplicationName = "Hello Triangle";
			appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
			appInfo.pEngineName = "No Engine";
			appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
			appInfo.apiVersion = VK_API_VERSION_1_0;

			std::vector<const char*> enabledExtensions;
			enabledExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
#ifdef _WINDOWS
			enabledExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#else
			enabledExtensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#endif

#ifdef _DEBUG
			enabledExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

			VkInstanceCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
			createInfo.pApplicationInfo = &appInfo;
			createInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
			createInfo.ppEnabledExtensionNames = enabledExtensions.data();

#ifdef _DEBUG
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();

#else
			createInfo.enabledLayerCount = 0;
#endif


			VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
#ifdef _DEBUG
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
			populateDebugMessengerCreateInfo(debugCreateInfo);
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
#else
			createInfo.enabledLayerCount = 0;
			createInfo.pNext = nullptr;
#endif
			CHECK_VK_RESULT(vkCreateInstance(&createInfo, nullptr, &instance_), "Failed to create vulkan instance!");
		}

		bool Renderer::checkValidationLayerSupport()
		{
			logger.debug("Checking validation layer support...");

			uint32_t layerCount;
			CHECK_VK_RESULT(vkEnumerateInstanceLayerProperties(&layerCount, nullptr), "Failed to enumerate instance layer properties!");

			std::vector<VkLayerProperties> availableLayers(layerCount);
			CHECK_VK_RESULT(vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data()), "Failed to enumerate instance layer properties!");

			for (const char* layerName : validationLayers)
			{
				bool layerFound = false;

				for (const auto& layerProperties : availableLayers)
				{
					if (strcmp(layerName, layerProperties.layerName) == 0)
					{
						layerFound = true;
						break;
					}
				}

				if (!layerFound)
				{
					return false;
				}
			}

			return true;
		}

#ifdef _DEBUG
		void Renderer::setupDebugMessenger()
		{
			VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
			populateDebugMessengerCreateInfo(createInfo);
			CHECK_VK_RESULT(createDebugUtilsMessengerEXT(instance_, &createInfo, nullptr, &debugMessenger_), "Failed to setup debug messenger!");
		}
#endif

		void Renderer::createPysicalDevice()
		{
			logger.debug("Creating vulkan device...");
			uint32_t deviceCount = 0;
			vkEnumeratePhysicalDevices(instance_, &deviceCount, nullptr);

			if (deviceCount == 0)
				throw std::runtime_error("Vulkan is not supported!");

			std::vector<VkPhysicalDevice> devices(deviceCount);
			vkEnumeratePhysicalDevices(instance_, &deviceCount, devices.data());

			for (const auto& device : devices)
			{
				if (isDeviceSuitable(device))
				{
					physicalDev_ = device;
					break;
				}
			}

			if (physicalDev_ == VK_NULL_HANDLE)
				throw std::runtime_error("failed to find a suitable GPU!");
		}

		bool Renderer::isDeviceSuitable(VkPhysicalDevice device)
		{
			VkPhysicalDeviceProperties deviceProperties;
			vkGetPhysicalDeviceProperties(device, &deviceProperties);

			VkPhysicalDeviceFeatures deviceFeatures;
			vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

			familyIndices_ = findQueueFamilies(device);

			const bool extensionsSupported = checkDeviceExtensionSupport(device);

			bool swapChainAdequate = false;

			if (extensionsSupported)
			{
				Renderer::SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
				swapChainAdequate = swapChainSupport.isComplete();
			}

			const bool propertiesSupported = deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU || deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
			const bool featuresSupported = deviceFeatures.geometryShader;

			return propertiesSupported && featuresSupported && familyIndices_.isComplete() && extensionsSupported && swapChainAdequate;
		}

		Renderer::QueueFamilyIndices Renderer::findQueueFamilies(VkPhysicalDevice device)
		{
			QueueFamilyIndices indices;

			uint32_t queueFamilyCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

			std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

			int i = 0;
			for (const auto& queueFamily : queueFamilies)
			{
				if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
					indices.graphicsFamily = i;

				VkBool32 presentSupport = false;
				vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface_, &presentSupport);
				if (presentSupport)
					indices.presentFamily = i;

				i++;
			}

			return indices;
		}

		void Renderer::createLogicalDevice()
		{
			logger.debug("Creating logical device...");

			std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
			std::set<uint32_t> queueFamilies = { familyIndices_.graphicsFamily.value(), familyIndices_.presentFamily.value() };

			float queuePriority = 1.0f;

			for (uint32_t queueFamily : queueFamilies)
			{
				VkDeviceQueueCreateInfo queueCreateInfo{};
				queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queueCreateInfo.queueFamilyIndex = queueFamily;
				queueCreateInfo.queueCount = 1;
				queueCreateInfo.pQueuePriorities = &queuePriority;
				queueCreateInfos.push_back(queueCreateInfo);
			}

			VkPhysicalDeviceFeatures deviceFeatures = {};

			VkDeviceCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
			createInfo.pQueueCreateInfos = queueCreateInfos.data();
			createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
			createInfo.pEnabledFeatures = &deviceFeatures;
			createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
			createInfo.ppEnabledExtensionNames = deviceExtensions.data();
#ifdef _DEBUG
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
#else
			createInfo.enabledLayerCount = 0;
#endif

			CHECK_VK_RESULT(vkCreateDevice(physicalDev_, &createInfo, nullptr, &dev_), "Failed to create logical device!");

			vkGetDeviceQueue(dev_, familyIndices_.graphicsFamily.value(), 0, &graphicsQueue_);
			vkGetDeviceQueue(dev_, familyIndices_.presentFamily.value(), 0, &presentQueue_);
		}

		void Renderer::createSurface()
		{
			logger.debug("Creating vulkan surface...");
#ifdef _WINDOWS
			VkWin32SurfaceCreateInfoKHR createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
			createInfo.hwnd = window_.handle();
			createInfo.hinstance = GetModuleHandle(nullptr);

			CHECK_VK_RESULT(vkCreateWin32SurfaceKHR(instance_, &createInfo, nullptr, &surface_), "Could not create window surface!");
#else
			throw std::runtime_error("createSurface() is not implemented!");
#endif
		}

		bool Renderer::checkDeviceExtensionSupport(VkPhysicalDevice device)
		{
			uint32_t extensionCount;
			vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

			std::vector<VkExtensionProperties> availableExtensions(extensionCount);
			vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

			std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

			for (const auto& extension : availableExtensions)
			{
				requiredExtensions.erase(extension.extensionName);
			}

			return requiredExtensions.empty();
		}

		Renderer::SwapChainSupportDetails Renderer::querySwapChainSupport(VkPhysicalDevice device)
		{
			SwapChainSupportDetails details;

			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface_, &details.capabilities);

			uint32_t formatCount;
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &formatCount, nullptr);

			if (formatCount != 0)
			{
				details.formats.resize(formatCount);
				vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &formatCount, details.formats.data());
			}

			uint32_t presentModeCount;
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &presentModeCount, nullptr);

			if (presentModeCount != 0)
			{
				details.presentModes.resize(presentModeCount);
				vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &presentModeCount, details.presentModes.data());
			}

			return details;
		}

		VkSurfaceFormatKHR Renderer::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
		{
			if (availableFormats.size() == 0)
				throw std::runtime_error("No available foramts to choose a swapchain surface format from!");

			for (const VkSurfaceFormatKHR& availableFormat : availableFormats)
			{
				if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
				{
					return availableFormat;
				}
			}
			return availableFormats[0];
		}

		VkPresentModeKHR Renderer::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
		{
			for (const VkPresentModeKHR& availablePresentMode : availablePresentModes)
			{
				if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
				{
					return availablePresentMode;
				}
			}

			return VK_PRESENT_MODE_FIFO_KHR;
		}

		VkExtent2D Renderer::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
		{
			if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
			{
				return capabilities.currentExtent;
			}
			else
			{
				size_t width = 0;
				size_t height = 0;

				VkExtent2D actualExtent = {};
				window_.getSize(actualExtent);

				actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
				actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

				return actualExtent;
			}
		}

		void Renderer::createSwapChain()
		{
			logger.debug("Creating swapchain...");

			SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDev_);

			VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
			VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
			swapChainExtent_ = chooseSwapExtent(swapChainSupport.capabilities);

			uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

			if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
			{
				imageCount = swapChainSupport.capabilities.maxImageCount;
			}

			VkSwapchainCreateInfoKHR createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
			createInfo.surface = surface_;
			createInfo.minImageCount = imageCount;
			createInfo.imageFormat = surfaceFormat.format;
			createInfo.imageColorSpace = surfaceFormat.colorSpace;
			createInfo.imageExtent = swapChainExtent_;
			createInfo.imageArrayLayers = 1;
			createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

			uint32_t queueFamilyIndices[] = { familyIndices_.graphicsFamily.value(), familyIndices_.presentFamily.value() };

			if (familyIndices_.graphicsFamily != familyIndices_.presentFamily)
			{
				createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
				createInfo.queueFamilyIndexCount = 2;
				createInfo.pQueueFamilyIndices = queueFamilyIndices;
			}
			else
			{
				createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
				createInfo.queueFamilyIndexCount = 0;
				createInfo.pQueueFamilyIndices = nullptr;
			}

			createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
			createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
			createInfo.presentMode = presentMode;
			createInfo.clipped = VK_TRUE;
			createInfo.oldSwapchain = VK_NULL_HANDLE;

			CHECK_VK_RESULT(vkCreateSwapchainKHR(dev_, &createInfo, nullptr, &swapChain_), "Failed to create the swapchain!");

			swapChainImages_.resize(imageCount);
			vkGetSwapchainImagesKHR(dev_, swapChain_, &imageCount, swapChainImages_.data());
			swapChainImageFormat_ = surfaceFormat.format;
		}

		void Renderer::createImageViews()
		{
			logger.debug("Creating swapchain image views...");

			swapChainImageViews_.resize(swapChainImages_.size());

			for (size_t i = 0; i < swapChainImages_.size(); i++)
			{
				VkImageViewCreateInfo createInfo{};
				createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
				createInfo.image = swapChainImages_[i];
				createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
				createInfo.format = swapChainImageFormat_;
				createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
				createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
				createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
				createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
				createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				createInfo.subresourceRange.baseMipLevel = 0;
				createInfo.subresourceRange.levelCount = 1;
				createInfo.subresourceRange.baseArrayLayer = 0;
				createInfo.subresourceRange.layerCount = 1;
				CHECK_VK_RESULT(vkCreateImageView(dev_, &createInfo, nullptr, &swapChainImageViews_[i]), "Failed to create swapchain image views!");
			}
		}

		void Renderer::createGraphicsPipeline()
		{
			logger.debug("Creating graphics pipeline...");

#ifdef _DEBUG
			logger.debug("TODO: Fix shader imports...");

			auto vertShaderCode = readFile("resources/shaders/out/vert.spv");
			auto fragShaderCode = readFile("resources/shaders/out/frag.spv");
#else
			auto vertShaderCode = readFile("resources/shaders/out/vert.spv");
			auto fragShaderCode = readFile("resources/shaders/out/frag.spv");
#endif

			VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
			VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

			VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
			vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
			vertShaderStageInfo.module = vertShaderModule;
			vertShaderStageInfo.pName = "main";

			VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
			fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			fragShaderStageInfo.module = fragShaderModule;
			fragShaderStageInfo.pName = "main";

			VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

			VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
			vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			vertexInputInfo.vertexBindingDescriptionCount = 0;
			vertexInputInfo.vertexAttributeDescriptionCount = 0;

			VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
			inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			inputAssembly.primitiveRestartEnable = VK_FALSE;

			VkPipelineViewportStateCreateInfo viewportState = {};
			viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			viewportState.viewportCount = 1;
			viewportState.scissorCount = 1;

			VkPipelineRasterizationStateCreateInfo rasterizer = {};
			rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			rasterizer.depthClampEnable = VK_FALSE;
			rasterizer.rasterizerDiscardEnable = VK_FALSE;
			rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
			rasterizer.lineWidth = 1.0f;
			rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
			rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
			rasterizer.depthBiasEnable = VK_FALSE;

			VkPipelineMultisampleStateCreateInfo multisampling = {};
			multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			multisampling.sampleShadingEnable = VK_FALSE;
			multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

			VkPipelineColorBlendAttachmentState colorBlendAttachment{};
			colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			colorBlendAttachment.blendEnable = VK_FALSE;

			VkPipelineColorBlendStateCreateInfo colorBlending{};
			colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			colorBlending.logicOpEnable = VK_FALSE;
			colorBlending.logicOp = VK_LOGIC_OP_COPY;
			colorBlending.attachmentCount = 1;
			colorBlending.pAttachments = &colorBlendAttachment;
			colorBlending.blendConstants[0] = 0.0f;
			colorBlending.blendConstants[1] = 0.0f;
			colorBlending.blendConstants[2] = 0.0f;
			colorBlending.blendConstants[3] = 0.0f;

			std::vector<VkDynamicState> dynamicStates = {
				VK_DYNAMIC_STATE_VIEWPORT,
				VK_DYNAMIC_STATE_SCISSOR
			};
			VkPipelineDynamicStateCreateInfo dynamicState{};
			dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
			dynamicState.pDynamicStates = dynamicStates.data();

			VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
			pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutInfo.setLayoutCount = 0;
			pipelineLayoutInfo.pushConstantRangeCount = 0;

			CHECK_VK_RESULT(vkCreatePipelineLayout(dev_, &pipelineLayoutInfo, nullptr, &pipelineLayout_), "Failed to create pipeline layout!");

			VkGraphicsPipelineCreateInfo pipelineInfo{};
			pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			pipelineInfo.stageCount = 2;
			pipelineInfo.pStages = shaderStages;
			pipelineInfo.pVertexInputState = &vertexInputInfo;
			pipelineInfo.pInputAssemblyState = &inputAssembly;
			pipelineInfo.pViewportState = &viewportState;
			pipelineInfo.pRasterizationState = &rasterizer;
			pipelineInfo.pMultisampleState = &multisampling;
			pipelineInfo.pColorBlendState = &colorBlending;
			pipelineInfo.pDynamicState = &dynamicState;
			pipelineInfo.layout = pipelineLayout_;
			pipelineInfo.renderPass = renderPass_;
			pipelineInfo.subpass = 0;
			pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

			CHECK_VK_RESULT(vkCreateGraphicsPipelines(dev_, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline_), "Failed to create graphics pipeline!");

			vkDestroyShaderModule(dev_, fragShaderModule, nullptr);
			vkDestroyShaderModule(dev_, vertShaderModule, nullptr);
		}

		VkShaderModule Renderer::createShaderModule(const std::vector<char>& code)
		{
			logger.debug("Creating shader module...");

			VkShaderModuleCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			createInfo.codeSize = code.size();
			createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
			VkShaderModule shaderModule;
			CHECK_VK_RESULT(vkCreateShaderModule(dev_, &createInfo, nullptr, &shaderModule), "Failed to create shader module!");
			return shaderModule;
		}

		void Renderer::createRenderPass()
		{
			logger.debug("Destroying render pass...");

			VkAttachmentDescription colorAttachment = {};
			colorAttachment.format = swapChainImageFormat_;
			colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

			VkAttachmentReference colorAttachmentRef = {};
			colorAttachmentRef.attachment = 0;
			colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			VkSubpassDescription subpass = {};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.colorAttachmentCount = 1;
			subpass.pColorAttachments = &colorAttachmentRef;

			VkRenderPassCreateInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassInfo.attachmentCount = 1;
			renderPassInfo.pAttachments = &colorAttachment;
			renderPassInfo.subpassCount = 1;
			renderPassInfo.pSubpasses = &subpass;

			VkSubpassDependency dependency = {};
			dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
			dependency.dstSubpass = 0;
			dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependency.srcAccessMask = 0;
			dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			renderPassInfo.dependencyCount = 1;
			renderPassInfo.pDependencies = &dependency;

			CHECK_VK_RESULT(vkCreateRenderPass(dev_, &renderPassInfo, nullptr, &renderPass_), "Failed to create render pass!");
		}

		void Renderer::createFramebuffers()
		{
			logger.debug("Creating frame buffers...");

			swapChainFramebuffers_.resize(swapChainImageViews_.size());

			for (size_t i = 0; i < swapChainImageViews_.size(); i++)
			{
				VkImageView attachments[] = {
					swapChainImageViews_[i]
				};

				VkFramebufferCreateInfo framebufferInfo = {};
				framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
				framebufferInfo.renderPass = renderPass_;
				framebufferInfo.attachmentCount = 1;
				framebufferInfo.pAttachments = attachments;
				framebufferInfo.width = swapChainExtent_.width;
				framebufferInfo.height = swapChainExtent_.height;
				framebufferInfo.layers = 1;

				CHECK_VK_RESULT(vkCreateFramebuffer(dev_, &framebufferInfo, nullptr, &swapChainFramebuffers_[i]), "Failed to create frame buffers!");
			}
		}

		void Renderer::createCommandPool()
		{
			logger.debug("Creating command pool...");

			VkCommandPoolCreateInfo poolInfo = {};
			poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			poolInfo.queueFamilyIndex = familyIndices_.graphicsFamily.value();
			CHECK_VK_RESULT(vkCreateCommandPool(dev_, &poolInfo, nullptr, &commandPool_), "Failed to create command pool!");
		}

		void Renderer::createCommandBuffer()
		{
			logger.debug("Creating command buffer...");

			VkCommandBufferAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			allocInfo.commandPool = commandPool_;
			allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			allocInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

			CHECK_VK_RESULT(vkAllocateCommandBuffers(dev_, &allocInfo, commandBuffer_), "Failed to allocate command buffers!");
		}

		void Renderer::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
		{
			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

			CHECK_VK_RESULT(vkBeginCommandBuffer(commandBuffer, &beginInfo), "Failed to begin recording command buffer!");

			VkRenderPassBeginInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = renderPass_;
			renderPassInfo.framebuffer = swapChainFramebuffers_[imageIndex];
			renderPassInfo.renderArea.offset = { 0, 0 };
			renderPassInfo.renderArea.extent = swapChainExtent_;

			VkClearValue clearColor = { { { 0.025f, 0.025f, 0.025f, 1.0f } } };
			renderPassInfo.clearValueCount = 1;
			renderPassInfo.pClearValues = &clearColor;

			vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline_);

			VkViewport viewport = {};
			viewport.x = 0.0f;
			viewport.y = 0.0f;
			viewport.width = static_cast<float>(swapChainExtent_.width);
			viewport.height = static_cast<float>(swapChainExtent_.height);
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;
			vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

			VkRect2D scissor = {};
			scissor.offset = { 0, 0 };
			scissor.extent = swapChainExtent_;
			vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

			vkCmdDraw(commandBuffer, 3, 1, 0, 0);

			vkCmdEndRenderPass(commandBuffer);

			CHECK_VK_RESULT(vkEndCommandBuffer(commandBuffer), "Failed to record command buffer!");
		}

		void Renderer::createSyncObjects()
		{
			VkSemaphoreCreateInfo semaphoreInfo{};
			semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

			VkFenceCreateInfo fenceInfo{};
			fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
			for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
			{
				CHECK_VK_RESULT(vkCreateSemaphore(dev_, &semaphoreInfo, nullptr, &imageAvailableSemaphore_[i]), "Failed to create image available semaphore!");
				CHECK_VK_RESULT(vkCreateSemaphore(dev_, &semaphoreInfo, nullptr, &renderFinishedSemaphore_[i]), "Failed to create render finished semaphore!");
				CHECK_VK_RESULT(vkCreateFence(dev_, &fenceInfo, nullptr, &inFlightFence_[i]), "Failed to create in fligh fence!");
			}
		}

		size_t Renderer::getNextFrameIndex()
		{
			return currentFrame_++ % MAX_FRAMES_IN_FLIGHT;
		}

		void Renderer::render()
		{
			const size_t index = getNextFrameIndex();

			vkWaitForFences(dev_, 1, &inFlightFence_[index], VK_TRUE, UINT64_MAX);
			vkResetFences(dev_, 1, &inFlightFence_[index]);

			uint32_t imageIndex;
			vkAcquireNextImageKHR(dev_, swapChain_, UINT64_MAX, imageAvailableSemaphore_[index], VK_NULL_HANDLE, &imageIndex);

			vkResetCommandBuffer(commandBuffer_[index], 0);
			recordCommandBuffer(commandBuffer_[index], imageIndex);

			VkSubmitInfo submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

			VkSemaphore waitSemaphores[] = { imageAvailableSemaphore_[index] };
			VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
			submitInfo.waitSemaphoreCount = 1;
			submitInfo.pWaitSemaphores = waitSemaphores;
			submitInfo.pWaitDstStageMask = waitStages;

			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &commandBuffer_[index];

			VkSemaphore signalSemaphores[] = { renderFinishedSemaphore_[index] };
			submitInfo.signalSemaphoreCount = 1;
			submitInfo.pSignalSemaphores = signalSemaphores;

			CHECK_VK_RESULT(vkQueueSubmit(graphicsQueue_, 1, &submitInfo, inFlightFence_[index]), "Failed to submit draw command buffer!");

			VkPresentInfoKHR presentInfo = {};
			presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

			presentInfo.waitSemaphoreCount = 1;
			presentInfo.pWaitSemaphores = signalSemaphores;

			VkSwapchainKHR swapChains[] = { swapChain_ };
			presentInfo.swapchainCount = 1;
			presentInfo.pSwapchains = swapChains;

			presentInfo.pImageIndices = &imageIndex;

			vkQueuePresentKHR(presentQueue_, &presentInfo);
		}
	}
}