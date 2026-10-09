#include "Renderer.h"
#define GLFW_INCLUDE_VULKAN

#include <iostream>
#include "GLFW/glfw3.h"
#include "Debug.h"
#include <map>

void Renderer::CreateInstance()
{
	constexpr vk::ApplicationInfo application_info
	{

		.pApplicationName = "Hello",
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		.pEngineName = "GC Vulkan",
		.engineVersion = VK_MAKE_VERSION(1, 0, 0),
		.apiVersion = vk::ApiVersion14
	};
	
	std::vector requiredExtensions = GetRequiredExtensions();


	auto extensionProperties = context.enumerateInstanceExtensionProperties();

	for (uint32_t i = 0; i < requiredExtensions.size(); i++)
	{
		auto compareExtensions = [glfwExtension = requiredExtensions[i]](auto const& extensionProperty) {
			return strcmp(extensionProperty.extensionName, glfwExtension) == 0; };

		if (std::ranges::none_of(extensionProperties, compareExtensions))
		{
			throw std::runtime_error("Required GLFW extension not supported : " + std::string(requiredExtensions[i]));
		}
	}

	std::vector <char const*> requiredLayers = GetRequiredLayers();
	auto layerProperties = context.enumerateInstanceLayerProperties();

	auto unsupportedLayerIt = std::ranges::find_if(requiredLayers, [&layerProperties](auto const& requiredLayer) 
		{ 
			return std::ranges::none_of(layerProperties, [requiredLayer](auto const& layerProperty)
				{
					return strcmp(layerProperty.layerName, requiredLayer) == 0;
				});
		});

	if (unsupportedLayerIt != requiredLayers.end())
	{
		throw std::runtime_error("Required Layer not supported :" + std::string(*unsupportedLayerIt));
	}

	vk::InstanceCreateInfo create_info{
		.pApplicationInfo = &application_info,
		.enabledLayerCount = static_cast<uint32_t>(requiredLayers.size()),
		.ppEnabledLayerNames = requiredLayers.data(),
		.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size()),
		.ppEnabledExtensionNames = requiredExtensions.data()
	};

	instance = vk::raii::Instance(context, create_info);

	SetupDebugCallback();
}

std::vector<const char*> Renderer::GetRequiredExtensions()
{
	uint32_t glfwExtensionCount = 0;
	auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<char const*> requiredExtensions = { vk::EXTDebugUtilsExtensionName };
	std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
	extensions.append_range(requiredExtensions);
	return extensions;
}

std::vector<const char*> Renderer::GetRequiredLayers()
{
	const std::vector<char const*> validationLayers = { "VK_LAYER_KHRONOS_validation" }; //pas obliger de mettre <char const*>

	std::vector<char const*> requiredLayers = {};
	if (Debug::EnableDebug)
		requiredLayers.append_range(validationLayers);
	return requiredLayers;
}

void Renderer::SetupDebugCallback()
{
	vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
	vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);

	vk::DebugUtilsMessengerCreateInfoEXT debugInfo{
		.messageSeverity = severityFlags,
		.messageType = messageTypeFlags,
		.pfnUserCallback = &DebugCallBack
	};

	debugMessenger = instance.createDebugUtilsMessengerEXT(debugInfo);
}

bool Renderer::isDeviceSuitable(vk::raii::PhysicalDevice const& physicalDevice)
{
	bool supportsVulkan1_4 = physicalDevice.getProperties().apiVersion >= vk::ApiVersion13;

	auto queueFamilies = physicalDevice.getQueueFamilyProperties();
	bool supportsGraphics = std::ranges::any_of(queueFamilies, [](auto const& qfp) { return !!(qfp.queueFlags & vk::QueueFlagBits::eGraphics); });

	auto availableDeviceExtensions = physicalDevice.enumerateDeviceExtensionProperties();
	bool supportsAllRequiredExtensions =
		std::ranges::all_of(requiredDeviceExtension,
			[&availableDeviceExtensions](auto const& requiredDeviceExtension)
			{
				return std::ranges::any_of(availableDeviceExtensions,
					[requiredDeviceExtension](auto const& availableDeviceExtension)
					{ return strcmp(availableDeviceExtension.extensionName, requiredDeviceExtension) == 0; });
			});

	auto features = physicalDevice.template getFeatures2<vk::PhysicalDeviceFeatures2,
		vk::PhysicalDeviceVulkan11Features,
		vk::PhysicalDeviceVulkan13Features,
		vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
	bool supportsRequiredFeatures = features.template get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters &&
		features.template get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
		features.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;

	return supportsVulkan1_4 && supportsGraphics && supportsAllRequiredExtensions && supportsRequiredFeatures;
}

void Renderer::pickPhysicalDevice()
{
	auto physicalDevices = instance.enumeratePhysicalDevices(); //enumerer les GPU
	if (physicalDevices.empty())
		throw std::runtime_error("failed to find GPUs with Vulkan support !"); //si il y en a pas -> erreur
	auto const bestDevice = std::ranges::find_if(physicalDevices, [&](auto const& physicalDevice) {
		return isDeviceSuitable(physicalDevice);
		});

	if (bestDevice == physicalDevices.end())
		throw std::runtime_error("failed to find a suitable GPU!");
	physicalDevice = *bestDevice;
}


VKAPI_ATTR vk::Bool32 Renderer::DebugCallBack(vk::DebugUtilsMessageSeverityFlagBitsEXT severity, vk::DebugUtilsMessageTypeFlagsEXT type, const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
	std::cout << "Validation layer: Type [" << vk::to_string(type) << "] - Message : " << pCallbackData->pMessage << std::endl;

	return vk::False;
}
