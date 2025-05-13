#include "gfx/vulkan/instance.hpp"

#include "gfx/gfx.hpp"

#include <GLFW/glfw3.h>
#include <cstring>

#include "gfx/vulkan/vk_check.hpp"

static const std::vector validationLayers = {"VK_LAYER_KHRONOS_validation"};

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                                                     VkDebugUtilsMessageTypeFlagsEXT,
                                                     const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
                                                     void *) {
    std::string context = "FAILED TO PARSE MESSAGE";
    std::string message_id = "FAILED TO PARSE MESSAGE";
    std::string message_text = callback_data->pMessage;

    const std::string message = callback_data->pMessage;
    const auto message_ids = stringutils::split(message, {'|'});

    if (message_ids.size() >= 3) {
        const auto message_id_vect = stringutils::split(message_ids[1], {'='});

        context = message_ids[0];
        message_id = message_id_vect.size() == 2 ? message_id_vect[1] : "FAILED TO PARSE MESSAGE ID";
        message_text = "";
        for (size_t i = 2; i < message_ids.size(); ++i) {
            message_text += message_ids[i];
            if (i != message_ids.size() - 1)
                message_text += "|";
        }
    } else {
        message_text = message;
    }

    if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
        LOG_TRACE("VALIDATION MESSAGE : {}", message);
    else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
        LOG_TRACE("VALIDATION INFO : \n\tcontext : {}\n\tmessage id : {}\n\n\t{}", context, message_id, message_text);
    else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        LOG_WARNING("VALIDATION WARNING : \n\tcontext : {}\n\tmessage id : {}\n\n\t{}", context, message_id,
                message_text);
    else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        if (message_text.find("obs-vulkan64") == std::string::npos)
            LOG_FATAL("VALIDATION ERROR : \n\tcontext : {}\n\tmessage id : {}\n\n\t{}", context, message_id,
                  message_text);
    } else {
        LOG_ERROR("VULKAN VALIDATION LAYER - UNKOWN VERBOSITY : {}", callback_data->pMessage);
    }

    return VK_FALSE;
}

VkResult create_debug_utils_messenger_ext(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                          const VkAllocationCallbacks *pAllocator,
                                          VkDebugUtilsMessengerEXT *pDebugMessenger) {
    auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(
        instance, "vkCreateDebugUtilsMessengerEXT"));
    if (func != nullptr)
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                   const VkAllocationCallbacks *pAllocator) {
    auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(
        instance, "vkDestroyDebugUtilsMessengerEXT"));
    if (func != nullptr)
        func(instance, debugMessenger, pAllocator);
}

namespace Eng::Gfx {
    Instance::Instance(GfxConfig &config) {
        glfwInit();
        if (config.enable_validation_layers) {
            LOG_INFO("Enable validation layers");
            if (!are_validation_layer_supported()) {
                config.enable_validation_layers = false;
                LOG_ERROR("Validation layers are enabled but not available");
            }
        }

        VkApplicationInfo appInfo{
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = config.app_name.c_str(),
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName = "Taranis",
            .engineVersion = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion = VK_API_VERSION_1_4,
        };

        uint32_t glfw_extension_count = 0;
        const char **glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

        VkInstanceCreateInfo instance_infos{
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, .pApplicationInfo = &appInfo,
            .enabledExtensionCount = glfw_extension_count, .ppEnabledExtensionNames = glfw_extensions
        };

        VkDebugUtilsMessengerCreateInfoEXT debug_messenger_infos = {};
        VkValidationFeaturesEXT additional_features = {};
        std::array enabled_validation_layers = {
            VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT, VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT
        };

        if (config.enable_validation_layers) {
            instance_infos.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            instance_infos.ppEnabledLayerNames = validationLayers.data();

            VkDebugUtilsMessageTypeFlagsEXT message_type =
                    VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                    VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            if (get_supported_extensions().contains(VK_EXT_DEVICE_ADDRESS_BINDING_REPORT_EXTENSION_NAME)) {
                message_type |= VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT;
            } else {
                LOG_WARNING("Device address binding message are not available on the current platform");
            }

            debug_messenger_infos = {
                .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
                .messageSeverity =
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
                .messageType = message_type,
                .pfnUserCallback = debug_callback,
                .pUserData = nullptr, // Optional
            };

            if (config.aggressive_validation_layers) {
                additional_features.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
                additional_features.enabledValidationFeatureCount = static_cast<uint32_t>(enabled_validation_layers.
                    size());
                additional_features.pEnabledValidationFeatures = enabled_validation_layers.data();
                additional_features.pNext = &debug_messenger_infos;
                instance_infos.pNext = &additional_features;
            } else
                instance_infos.pNext = &debug_messenger_infos;
        }

        auto extensions = get_required_extensions(config);
        instance_infos.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        instance_infos.ppEnabledExtensionNames = extensions.data();

        VK_CHECK(vkCreateInstance(&instance_infos, nullptr, &ptr), "Failed to create instance")

        if (config.enable_validation_layers)
            VK_CHECK(create_debug_utils_messenger_ext(ptr, &debug_messenger_infos, nullptr, &debug_messenger),
                 "Failed to setup debug messenger")
    }

    Instance::~Instance() {
        if (debug_messenger) {
            DestroyDebugUtilsMessengerEXT(ptr, debug_messenger, nullptr);
        }
        vkDestroyInstance(ptr, nullptr);
        glfwTerminate();
    }

    const std::vector<const char *> &Instance::validation_layers() {
        return validationLayers;
    }

    void Instance::begin_debug_marker(const VkCommandBuffer &cmd, const std::string &name,
                                      const std::array<float, 4> &color) const {
        if (debug_messenger != VK_NULL_HANDLE) {
            auto func = reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>(vkGetInstanceProcAddr(
                ptr, "vkCmdBeginDebugUtilsLabelEXT"));
            if (func != nullptr) {
                VkDebugUtilsLabelEXT infos{
                    .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT, .pLabelName = name.c_str(),
                    .color = {color[0], color[1], color[2], color[3]}
                };
                func(cmd, &infos);
            }
        }
    }

    void Instance::end_debug_marker(const VkCommandBuffer &cmd) const {
        if (debug_messenger != VK_NULL_HANDLE) {
            auto func = reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>(vkGetInstanceProcAddr(
                ptr, "vkCmdEndDebugUtilsLabelEXT"));
            if (func != nullptr) {
                func(cmd);
            }
        }
    }

    ankerl::unordered_dense::set<std::string> Instance::get_supported_extensions() {
        VkResult result = VK_SUCCESS;

        uint32_t count = 0;
        result = vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
        if (result != VK_SUCCESS) {
            LOG_FATAL("Failed to enumerate instance extensions");
        }

        std::vector<VkExtensionProperties> extension_properties(count);
        result = vkEnumerateInstanceExtensionProperties(nullptr, &count, extension_properties.data());
        if (result != VK_SUCCESS) {
            LOG_FATAL("Failed to enumerate instance extensions");
        }

        ankerl::unordered_dense::set<std::string> extensions;
        for (auto &extension: extension_properties) {
            extensions.insert(extension.extensionName);
        }

        return extensions;
    }

    bool Instance::are_validation_layer_supported() {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char *layerName: validationLayers) {
            bool layerFound = false;

            for (const auto &layerProperties: availableLayers) {
                if (strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound) {
                return false;
            }
        }
        return true;
    }

    std::vector<const char *> Instance::get_required_extensions(const GfxConfig &config) {
        uint32_t glfw_extension_count = 0;
        const char **glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
        std::vector extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

        if (config.enable_validation_layers) {
            if (get_supported_extensions().contains(VK_EXT_DEVICE_ADDRESS_BINDING_REPORT_EXTENSION_NAME)) {
                extensions.push_back(VK_EXT_DEVICE_ADDRESS_BINDING_REPORT_EXTENSION_NAME);
            }
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
    }
} // namespace Eng::Gfx
