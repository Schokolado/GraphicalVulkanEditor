#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <cstdlib>
#include <vector>
#include <map>
#include <set>
#include <optional>
#include <string>


#include "VulkanProjectVariables.h"

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

// Creator class to initialize and setup Vulkan specific objects related to physical and logical devices, window surfaces, swap chains and image views
class VulkanPresentationDevicesInitializer {
    friend class VulkanApplication;
public:

private:

    ///////////////////////////////////////
    /*      Section for Window Surfaces, Swap Chains and Image Views  */
    ///////////////////////////////////////

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities; //min/max number of images in swap chain, min/max width and height of images
        std::vector<VkSurfaceFormatKHR> formats; //pixel format, color space
        std::vector<VkPresentModeKHR> presentationModes; //e.g. FIFO, IMMEDIATE
    };

    bool checkSwapChainSupport(VkSurfaceKHR surface, VkPhysicalDevice device) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(surface, device);
        //For simplicity it is sufficient for now to have at least one supported image format and one supported presentation mode.
            //add sophisticated swap chain selector here.
        return !swapChainSupport.formats.empty() && !swapChainSupport.presentationModes.empty();
    }

    SwapChainSupportDetails querySwapChainSupport(VkSurfaceKHR surface, VkPhysicalDevice device) {
        SwapChainSupportDetails details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
        if (formatCount != 0) {
            details.formats.resize(formatCount); //resize vector to hold all formats
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
        }

        uint32_t presentationModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentationModeCount, nullptr);
        if (presentationModeCount != 0) {
            details.presentationModes.resize(presentationModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentationModeCount, details.presentationModes.data());
        }

        return details;
    }

    //Querying for swap chain support can actually be omitted, because having a presentation queue implies the presence of a swap chain extension
    bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties>availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

        //check for all device extensions can also be performed like checking for validation layers using a nested for loop.
        for (const auto& extension : availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }

    void createSurface(VkSurfaceKHR* surface, GLFWwindow* window, VkInstance* instance) {
        if (glfwCreateWindowSurface(*instance, window, nullptr, surface) != VK_SUCCESS) {
            throw std::runtime_error("failed to create window surface!");
        }
    }

    ///////////////////////////////////////
    /* Section for Logical Devices and Queues */
    ///////////////////////////////////////

    void populateQueueCreateInfo(std::vector<VkDeviceQueueCreateInfo>& queueCreateInfos, VkSurfaceKHR surface, VkPhysicalDevice* physicalDevice) {
        QueueFamilyIndices indices = findQueueFamilies(surface, *physicalDevice);


        std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentationFamily.value() };

        float queuePriority = 1.0f;
       
        for (uint32_t queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }
    }

    void createLogicalDevice(VkSurfaceKHR* surface, VkQueue* presentationQueue, VkQueue* graphicsQueue, VkDevice* device, VkPhysicalDevice* physicalDevice) {

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        populateQueueCreateInfo(queueCreateInfos, *surface, physicalDevice);

        VkPhysicalDeviceFeatures deviceFeatures{};

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());

        createInfo.pEnabledFeatures = &deviceFeatures;
        createInfo.enabledExtensionCount = deviceExtensions.size();
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();

        // distinct between instance and device specific validation layers, used for legacy compliance
        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else {
            createInfo.enabledLayerCount = 0;
        }

        if (vkCreateDevice(*physicalDevice, &createInfo, nullptr, device) != VK_SUCCESS) {
            throw std::runtime_error("failed to create logical device!");
        }

        vkGetDeviceQueue(*device, queueCreateInfos.data()->queueFamilyIndex, 0, graphicsQueue);
        vkGetDeviceQueue(*device, queueCreateInfos.data()->queueFamilyIndex, 0, presentationQueue);
    }

    ///////////////////////////////////////
    /*      Section for Queue Families   */
    ///////////////////////////////////////

    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentationFamily;

        //return true if graphics family and presentation family is set.
        bool isComplete() {
            return graphicsFamily.has_value() && presentationFamily.has_value();
        }
    };

    QueueFamilyIndices findQueueFamilies(VkSurfaceKHR surface, VkPhysicalDevice device) {
        QueueFamilyIndices indices;

        // Logic to find queue family indices to populate struct with
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto& queueFamily : queueFamilies) {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphicsFamily = i;
            }

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

            if (presentSupport) {
                indices.presentationFamily = i;
            }


            if (indices.isComplete()) {
                break;
            }

            i++;
        }

        return indices;
    }

    ///////////////////////////////////////
    /*      Section for Physical Devices */
    ///////////////////////////////////////

    bool chooseStartUpGPU(VkSurfaceKHR surface, VkPhysicalDevice* physicalDevice, std::vector<VkPhysicalDevice> devices) {
        
        std::cout << "Select GPU to run the application" << std::endl;

        //remove unusable devices from list
        devices.erase(std::remove_if(
            devices.begin(), devices.end(),
            [surface, this](const auto &device) {
                return !isDeviceSuitable(surface, device);
            }), devices.end());

        if (devices.empty()) {
            throw std::runtime_error("failed to find a suitable GPU!");
        }

        //list all usable GPUs on console
        size_t count = 0;
        for (const auto& device : devices) {
            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(device, &deviceProperties);
            std::cout << "(" << count << ") " << deviceProperties.deviceName << std::endl;
            count++;
        }

        std::string selected;
        std::cin >> selected;

        //validate input as a number
        size_t inputIndex = -1;
        try {
            inputIndex = std::stoi(selected); //unsafe conversion if input starts with a number and continues with letters output will be the leading number.
        }
        catch (const std::exception& e) {
            std::cerr << "Input was not a number." << std::endl;
            std::cerr << "Fallback to automatic GPU selection." << std::endl;
            return false;
        }

        if (inputIndex >= 0 && inputIndex < devices.size()) {
            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(devices.at(inputIndex), &deviceProperties);
            std::cout << "Proceed with GPU: " << deviceProperties.deviceName << std::endl;
            *physicalDevice = devices.at(inputIndex);
            return true;
        }
        else {
            std::cerr << "Selected GPU number was out of bounds!" << std::endl;
            std::cerr << "Fallback to automatic GPU selection." << std::endl;
            return false;
        }
    }

    // Simple device chooser, unused if device rate suitability is used instead.
    bool isDeviceSuitable(VkSurfaceKHR surface, VkPhysicalDevice device) {

        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);

        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

        QueueFamilyIndices indices = findQueueFamilies(surface, device);

        bool extensionsSupported = checkDeviceExtensionSupport(device);
        bool swapChainAdequate = false;
        
        if (extensionsSupported) {
            swapChainAdequate = checkSwapChainSupport(surface, device);
        }

        return indices.isComplete() && deviceFeatures.geometryShader && extensionsSupported && swapChainAdequate;
    }

    void findBestCandidate(VkSurfaceKHR* surface, VkPhysicalDevice* physicalDevice, std::vector<VkPhysicalDevice> devices) {
        // Use an ordered map to automatically sort candidates by increasing score
        std::multimap<int, VkPhysicalDevice> candidates;

        for (const auto& device : devices) {
            int score = rateDeviceSuitability(*surface, device);
            candidates.insert(std::make_pair(score, device));
        }

        // Check if the best candidate is suitable at all
        if (candidates.rbegin()->first > 0) {
            *physicalDevice = candidates.rbegin()->second;
        }
        else {
            throw std::runtime_error("failed to find a suitable GPU!");
        }
    }

    // Choose Device by suitability, dedicated graphics gain higher scores than integrated and are favored.
    int rateDeviceSuitability(VkSurfaceKHR surface, VkPhysicalDevice device) {

        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);

        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

        QueueFamilyIndices indices = findQueueFamilies(surface, device);

        int score = 0;

        // Discrete GPUs have a significant performance advantage
        if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && indices.isComplete()) {
            score += 1000;
        }

        // Maximum possible size of textures affects graphics quality
        score += deviceProperties.limits.maxImageDimension2D;

        bool extensionsSupported = checkDeviceExtensionSupport(device);
        bool swapChainAdequate = false;

        if (extensionsSupported) {
            swapChainAdequate = checkSwapChainSupport(surface, device);
        }

        // Application can't function without geometry shaders or necessary extensions (for e.g. swapchains to present images) or inadequate swapchains
        if (!deviceFeatures.geometryShader || !extensionsSupported || !swapChainAdequate || !indices.isComplete()) {
            return 0;
        }

        return score;
    }

    void pickPhysicalDevice(VkSurfaceKHR* surface, VkPhysicalDevice* physicalDevice, VkInstance* instance) {

        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(*instance, &deviceCount, nullptr);

        if (deviceCount == 0) {
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(*instance, &deviceCount, devices.data());

        if (CHOOSE_GPU_ON_STARTUP) {
            if (!chooseStartUpGPU(*surface, physicalDevice, devices)) {
                findBestCandidate(surface, physicalDevice, devices);
                return;
            }
            else {
                return;
            };
        }

        findBestCandidate(surface, physicalDevice, devices);
    }
};

// Creator class to initialize and setup Vulkan specific objects related to Instances
class VulkanInstanceInitializer {
friend class VulkanApplication;
public:

private:
    ///////////////////////////////////////
    /*      Section for Debug Messenger  */
    ///////////////////////////////////////
    void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
        //extension functions are not loaded automatically and need to be called via vkGetInstanceProcAddr
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(instance, debugMessenger, pAllocator);
        }
    }
    
    VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
        //extension functions are not loaded automatically and need to be called via vkGetInstanceProcAddr
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr) {
            return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        }
        else {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    // Populate DebugMessengerInfo to get debug messages even after messenger is destroyed before instance
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
        createInfo.pUserData = nullptr; // optional
    }

    void setupDebugMessenger(VkDebugUtilsMessengerEXT* debugMessenger, VkInstance* instance) {
        if (!enableValidationLayers) return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        populateDebugMessengerCreateInfo(createInfo);
 
        if (CreateDebugUtilsMessengerEXT(*instance, &createInfo, nullptr, debugMessenger) != VK_SUCCESS) {
            throw std::runtime_error("failed to set up debug messenger!");
        }
    }

    //Callback returns boolean to indicate if a Vulkan call that triggered a validation layer message should be aborted. Should only trturn true for validation Layer testing.
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData) {

        /*
        if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
            // Message is important enough to show
        }
        */
        
        if (SHOW_VALIDATION_LAYER_DEBUG_INFO) {
            std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
        }

        return VK_FALSE;
    }

    ///////////////////////////////////////
    /*      Section for VK Instance      */
    ///////////////////////////////////////

    std::vector<const char*> getRequiredExtensions() {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (!checkRequiredGlfwExtensionsSupport(extensions)) {
            throw std::runtime_error("extensions requested for glfw, but not available!");
        };

        if (enableValidationLayers) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }
        if (RUN_ON_MACOS) {
            extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
        }

        return extensions;
    }

    bool checkRequiredGlfwExtensionsSupport(std::vector<const char*> extensions) {
        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> foundExtensions(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, foundExtensions.data());
        std::cout << "required instance extensions by glfw:\n";
        bool foundAllRequiredExtensions = false;

        for (const auto& extension : extensions) {

           auto found = std::find_if(foundExtensions.begin(), foundExtensions.end(), [extension](const auto& foundExtension) {
               return strcmp(extension, foundExtension.extensionName) == 0;
                });

            if (found != foundExtensions.end()) {
                std::cout << "\tExtension " << extension << " found" << std::endl;
                foundAllRequiredExtensions = true;
            }
            else {
                std::cout << "\tExtension " << extension << " not found" << std::endl;
                return false;
            }
        }

        return foundAllRequiredExtensions;
    }

    bool checkValidationLayerSupport() {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char* layerName : validationLayers) {
            bool layerFound = false;

            for (const auto& layerProperties : availableLayers) {
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

    void createInstance(VkInstance* instance) {
        if (enableValidationLayers && !checkValidationLayerSupport()) {
            throw std::runtime_error("validation layers requested, but not available!");
        }

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = APPLICATION_NAME;
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        auto extensions = getRequiredExtensions();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        if(RUN_ON_MACOS) createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
        
        // this debug messenger will be used only for creation and desctruction of Instance and cleaned up afterwards
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();

            populateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
        }
        else {
            createInfo.enabledLayerCount = 0;
            createInfo.pNext = nullptr;
        }

        if (vkCreateInstance(&createInfo, nullptr, instance) != VK_SUCCESS) {
            throw std::runtime_error("failed to create instance!");
        };
    };
};

class VulkanApplication {
public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    VulkanPresentationDevicesInitializer* presentationDeviceCreator;
    VulkanInstanceInitializer* instanceCreator;

    GLFWwindow* window;
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;
    VkQueue graphicsQueue;

    VkSurfaceKHR surface;
    VkQueue presentQueue;




    void initWindow() {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); //dont create opengl context
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); //disable resizable windows

        window = glfwCreateWindow(WIDTH, HEIGHT, APPLICATION_NAME, nullptr, nullptr);
    }

    void initVulkan() {
        instanceCreator->createInstance(&instance);
        instanceCreator->setupDebugMessenger(&debugMessenger, &instance);

        presentationDeviceCreator->createSurface(&surface, window, &instance);
        presentationDeviceCreator->pickPhysicalDevice(&surface, &physicalDevice, &instance);
        presentationDeviceCreator->createLogicalDevice(&surface, &presentQueue, &graphicsQueue, &device, &physicalDevice);
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }
    }

    void cleanupGlfw() {
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void cleanupInstances() {
        vkDestroyInstance(instance, nullptr);
    }

    void cleanupDebugMessengers() {
        if (enableValidationLayers) {
            instanceCreator->DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        }
    }

    void cleanupSurfaces() {
        vkDestroySurfaceKHR(instance, surface, nullptr);
    }

    void cleanupDevices() {
        // Nothing to do for physical devices. 
        // Will be destroyed on instance destruction
        // 
        // Nothing to do for device queues. 
        // Will be destroyed on logical device destruction
        vkDestroyDevice(device, nullptr);
    }



    void cleanup() {
        cleanupDevices();
        cleanupSurfaces();
        cleanupDebugMessengers();
        cleanupInstances();
        cleanupGlfw();
    }



};


