#include "gfx/vulkan/instance.hpp"

#include "gfx/gfx.hpp"

#include <GLFW/glfw3.h>

#include "gfx/vulkan/vk_check.hpp"

static const std::vector validationLayers = {"VK_LAYER_KHRONOS_validation"};

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT, const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void*)
{
    std::string context      = "FAILED TO PARSE MESSAGE";
    std::string message_id   = "FAILED TO PARSE MESSAGE";
    std::string message_text = callback_data->pMessage;

    const std::string message     = callback_data->pMessage;
    const auto        message_ids = stringutils::split(message, {'|'});

    if (message_ids.size() == 3)
    {
        const auto message_id_vect = stringutils::split(message_ids[1], {'='});

        context      = message_ids[0];
        message_id   = message_id_vect.size() == 2 ? message_id_vect[1] : "FAILED TO PARSE MESSAGE ID";
        message_text = message_ids[2];
    }
    else
    {
        message_text = message;
    }

    if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
        LOG_DEBUG("VALIDATION MESSAGE : {}", message);
    else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
        LOG_INFO("VALIDATION INFO : \n\tcontext : {}\n\tmessage id : {}\n\n\t{}", context, message_id, message_text);
    else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        LOG_WARNING("VALIDATION WARNING : \n\tcontext : {}\n\tmessage id : {}\n\n\t{}", context, message_id, message_text);
    else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        LOG_FATAL("VALIDATION ERROR : \n\tcontext : {}\n\tmessage id : {}\n\n\t{}", context, message_id, message_text)
    }
    else
    {
        LOG_ERROR("VULKAN VALIDATION LAYER - UNKOWN VERBOSITY : {}", callback_data->pMessage);
    }

    return VK_FALSE;
}

VkResult create_debug_utils_messenger_ext(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
    auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
    if (func != nullptr)
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
    auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
    if (func != nullptr)
        func(instance, debugMessenger, pAllocator);
}

namespace Engine::Gfx
{
Instance::Instance(GfxConfig& config)
{
    glfwInit();
    if (config.enable_validation_layers && !are_validation_layer_supported())
    {
        config.enable_validation_layers = false;
        LOG_ERROR("Validation layers are enabled but not available");
    }

    VkApplicationInfo appInfo{
        .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName   = config.app_name.c_str(),
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName        = "Ashwga",
        .engineVersion      = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion         = VK_API_VERSION_1_3,
    };

    uint32_t     glfw_extension_count = 0;
    const char** glfw_extensions      = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    VkInstanceCreateInfo instance_infos{.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, .pApplicationInfo = &appInfo, .enabledExtensionCount = glfw_extension_count, .ppEnabledExtensionNames = glfw_extensions};

    VkDebugUtilsMessengerCreateInfoEXT debug_messenger_infos = {};
    if (config.enable_validation_layers)
    {
        instance_infos.enabledLayerCount   = static_cast<uint32_t>(validationLayers.size());
        instance_infos.ppEnabledLayerNames = validationLayers.data();

        debug_messenger_infos = {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .messageSeverity =
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
            .messageType =
                VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT,
            .pfnUserCallback = debug_callback,
            .pUserData       = nullptr, // Optional
        };
        instance_infos.pNext = &debug_messenger_infos;
    }

    auto extensions                        = get_required_extensions(config);
    instance_infos.enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
    instance_infos.ppEnabledExtensionNames = extensions.data();

    VK_CHECK(vkCreateInstance(&instance_infos, nullptr, &ptr), "Failed to create instance")

    VK_CHECK(create_debug_utils_messenger_ext(ptr, &debug_messenger_infos, nullptr, &debug_messenger), "Failed to setup debug messenger")
}

Instance::~Instance()
{
    if (debug_messenger)
    {
        DestroyDebugUtilsMessengerEXT(ptr, debug_messenger, nullptr);
    }
    vkDestroyInstance(ptr, nullptr);
    glfwTerminate();
}

const std::vector<const char*>& Instance::validation_layers()
{
    return validationLayers;
}

bool Instance::are_validation_layer_supported()
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

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

std::vector<const char*> Instance::get_required_extensions(const GfxConfig& config)
{
    uint32_t     glfw_extension_count = 0;
    const char** glfw_extensions      = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    std::vector  extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

    if (config.enable_validation_layers)
    {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}
} // namespace Engine::Gfx
