#include "Renderer.h"
#define GLFW_INCLUDE_VULKAN

#include "GLFW/glfw3.h"
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
	
	uint32_t glfwExtensionCount = 0;
	auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	auto extensionProperties = context.enumerateInstanceExtensionProperties();

	for (uint32_t i = 0; i < glfwExtensionCount; i++)
	{
		auto compareExtensions = [glfwExtension = glfwExtensions[i]](auto const& extensionProperty) {
			return strcmp(extensionProperty.extensionName, glfwExtension) == 0; };

		if (std::ranges::none_of(extensionProperties, compareExtensions))
		{
			throw std::runtime_error("Required GLFW extension not supported : " + std::string(glfwExtensions[i]));
		}
	}

	const std::vector<char const*> validationLayers = { "VK_LAYER_KHRONOS_validation" }; //pas obliger de mettre <char const*>

#ifdef _DEBUG
	const bool enableValidationLayers = true;
#else
	const bool enableValidationLayers = false;
#endif

	std::vector<char const*> requiredLayers;
	if (enableValidationLayers)
		requiredLayers.assign_range(validationLayers);

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
		.enabledExtensionCount = glfwExtensionCount,
		.ppEnabledExtensionNames = glfwExtensions
	};

	instance = vk::raii::Instance(context, create_info);
}

