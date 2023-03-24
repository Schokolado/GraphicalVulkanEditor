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
#include <fstream>
#include <algorithm>

#include <shaderc/shaderc.hpp>
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

// Creator class to initialize and setup Vulkan specific objects related to graphics pipeline
class VulkanGraphicsPipelineInitializer {
    friend class VulkanApplication;
public:

private:

    /////////////////////////////////////////////////
    /*    Section for Graphics Pipeline Objects    */
    /////////////////////////////////////////////////


    //read compiled spv file
    static std::vector<char> readSPVFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error("failed to open file!");
        }
        //start reading from end to determine the size of the file and allocate a buffer
        size_t fileSize = (size_t)file.tellg();
        std::vector<char> buffer(fileSize);
        file.seekg(0); //go back to start and read all data at once.
        file.read(buffer.data(), fileSize);
        file.close();

        return buffer;
    }

    //read shader file for compiling to spv inside the app
    static std::string readShaderFile(const std::string& filename) {
        std::ifstream file(filename);

        if (!file.is_open()) {
            throw std::runtime_error("failed to open!" + filename);
        }

        std::string shaderText((std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>());

        return shaderText;

    }
    //FIXME: remember to add information to use shaderc_combinedd.lib for debug and shaderc_combinedd.lib for release in the linker Input @properties of VS!!
    shaderc::SpvCompilationResult compileShader(std::string source_text, const char* shader_type){

        shaderc_shader_kind shader_kind;
        if (strcmp(shader_type, "vertex") == 0) {
            shader_kind = shaderc_glsl_vertex_shader;
        } else if (strcmp(shader_type, "fragment") == 0) {
            shader_kind = shaderc_glsl_fragment_shader;
        }
        else {
            throw std::runtime_error("provided shader type not usable:" + (std::string)shader_type);
        }

        shaderc::Compiler compiler;
        shaderc::CompileOptions options;
        if (REDUCE_SPIRV_CODE_SIZE) {
            options.SetOptimizationLevel(shaderc_optimization_level_size);
        }
        shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(source_text, shader_kind, shader_type, options);

        if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
            throw std::runtime_error(result.GetErrorMessage());
        }

        return result ;
    }

    void createGraphicsPipeline() {
        auto vertShaderCode = readSPVFile("shaders/not_needed/compiled_shaders/vert.spv");
        auto fragShaderCode = readSPVFile("shaders/not_needed/compiled_shaders/frag.spv");

        std::string vertexShaderText = readShaderFile(VERTEX_SHADER_FILE);
        std::string fragmentShaderText = readShaderFile(FRAGMENT_SHADER_FILE);

        auto vertexShaderCode = compileShader(vertexShaderText,"vertex");
        auto fragmentShaderCode = compileShader(fragmentShaderText, "fragment");

        //assign pCode of createInfo using this:  std::vector<uint32_t> spirv = {vertexShaderCode.cbegin(),vertexShaderCode.cend()};
    };
};

// Creator class to initialize and setup Vulkan specific objects related to physical and logical devices, window surfaces, swap chains and image views
class VulkanPresentationDevicesInitializer {
    friend class VulkanApplication;
public:

private:

    ///////////////////////////////////////
    /*      Section for Window Surfaces, Swap Chains and Image Views  */
    ///////////////////////////////////////

    void createImageViews(std::vector<VkImageView>* swapChainImageViews, VkFormat* swapChainImageFormat, std::vector<VkImage>* swapChainImages, VkDevice* device) {
        swapChainImageViews->resize(swapChainImages->size());
        for (size_t i = 0; i < swapChainImages->size(); i++) {
            VkImageViewCreateInfo createInfo{};
           
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = swapChainImages->at(i); //color/render target
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; //treat images as 1D textures, 2D textures, 3D textures and cube maps.
            createInfo.format = *swapChainImageFormat; //pixel format, color space
           
            //Shift color channels to prefered likings e.g. map all channels to red to get monochrome textures etc.
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            
            //subresource describes what the image's purpose is and which part of the image should be accessed.
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; //Use color information
            createInfo.subresourceRange.baseMipLevel = MIPMAP_LEVEL; //choose mipmap level for image views
            createInfo.subresourceRange.levelCount = 1; //no multilayered images
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1; //for stereoscopic 3D applications, use multiple layers to access views for left and right eye

            if (vkCreateImageView(*device, &createInfo, nullptr, &swapChainImageViews->at(i)) != VK_SUCCESS) {
                throw std::runtime_error("failed to create image views!");
            }
        }
    }

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities; //min/max number of images in swap chain, min/max width and height of images
        std::vector<VkSurfaceFormatKHR> formats; //pixel format, color space
        std::vector<VkPresentModeKHR> presentationModes; //conditions for "swapping" images to the screen. e.g. FIFO, IMMEDIATE
    };

    void createSwapChain(VkExtent2D* swapChainExtent, VkFormat* swapChainImageFormat,  std::vector<VkImage>* swapChainImages, VkSwapchainKHR* swapchain, VkSurfaceKHR* surface, VkDevice* device, VkPhysicalDevice* physicalDevice, GLFWwindow* window) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(*surface, *physicalDevice);

        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentationMode = chooseSwapPresentMode(swapChainSupport.presentationModes);
        VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities, window);

        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
        //make sure to not exceed the maximum number of images while doing this, where 0 is a special value that means that there is no maximum
        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = *surface;

        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1; //specifies the amount of layers each image consists of, always 1 for 2D Applications. Increase if using 3D stereoscopic images.
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; //specifies what kind of operations we'll use the images in the swap chain for.
                                                                    //VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT specifies to render directly into images
                                                                    //VK_IMAGE_USAGE_TRANSFER_DST_BIT may be used to render into separate image and perform post processing. Perform memory operation then to transport image to swap chain.
        QueueFamilyIndices indices = findQueueFamilies(*surface, *physicalDevice);
        uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentationFamily.value()};

        if (indices.graphicsFamily != indices.presentationFamily) {
            //drawing on the images in the swap chain from the graphics queue and then submitting them on the presentation queue if they are not the same
            // --> Avoid ownership management.
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT; //Images can be used across multiple queue families without explicit ownership transfers.
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; //image is owned by one queue family at a time and ownership must be explicitly transferred before using it in another queue family. --> Best performance
            createInfo.queueFamilyIndexCount = 0; //Optional
            createInfo.pQueueFamilyIndices = nullptr; //Optional
        }

        createInfo.preTransform = swapChainSupport.capabilities.currentTransform; //change this to apply transformations to each image e.g. 90 degree rotations.
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; //use to blend in with other images on window - change if opacity of alpha channel is relevant.
        createInfo.presentMode = presentationMode;
        createInfo.clipped = VK_TRUE; //enable clipping if hidden pixels are not relevant or should not be read.
        createInfo.oldSwapchain = VK_NULL_HANDLE; //leave for now - resizing windows will create new swapchains based on old ones --> later topic.

        if (vkCreateSwapchainKHR(*device, &createInfo, nullptr, swapchain) != VK_SUCCESS) {
            throw std::runtime_error("failed to create swap chain!");
        }

        //potential to put this code into separate function or method and call with struct instead of such many parameters
        vkGetSwapchainImagesKHR(*device, *swapchain, &imageCount, nullptr);
        swapChainImages->resize(imageCount);
        vkGetSwapchainImagesKHR(*device, *swapchain, &imageCount, swapChainImages->data());

        *swapChainImageFormat = surfaceFormat.format;
        *swapChainExtent = extent;
    }

    //The swap extent is the resolution of the swap chain images and it's almost always exactly equal to the resolution of the window that we're drawing to in pixels (more on that in a moment).
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window) {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        }
        else {
            //Vulkan works with pixels instead of screen coordinates.
            //But the conversion for high-DPI screens such as Apple Retina Displays does not match a 1:1 conversion --> use glfwFrameBufferSize
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);

            VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };

            actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }

    // presentation modes:
    // VK_PRESENT_MODE_IMMEDIATE_KHR: created images will be transferred immediately to the screen, may cause tearing
    // VK_PRESENT_MODE_FIFO_KHR: created images will be stored in queue, first in first out principle, if queue is full program stops to create new images - guaranteed to be available
    // VK_PRESENT_MODE_FIFO_RELAXED_KHR: same as FIFO but if queue is empty, application transfers image to screen without waiting for display refresh/vertical blank
    // VK_PRESENT_MODE_MAILBOX_KHR: new created images replace already present images in the queue if its full, instead of stopping the program to create frames. Reduces latency issues but consumes a lot of energy - don't use on mobile devices.
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentationModes) {
        for (const auto& availablePresentationMode : availablePresentationModes) {
            if (!SAVE_ENERGY_FOR_MOBILE) {
                if (availablePresentationMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                    return availablePresentationMode;
                }
            }
        }
        
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
        for (const auto& availableFormat : availableFormats) {
            //prefer 32 Bit srgb color cormat (8 bit per channel) and srgb color space for more accurate colors
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

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
    VulkanGraphicsPipelineInitializer* graphicsPipelineCreator;
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

    VkSwapchainKHR swapchain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;




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
        presentationDeviceCreator->createSwapChain(&swapChainExtent, &swapChainImageFormat, &swapChainImages, &swapchain, &surface, &device, &physicalDevice, window);
        presentationDeviceCreator->createImageViews(&swapChainImageViews, &swapChainImageFormat, &swapChainImages, &device);

        graphicsPipelineCreator->createGraphicsPipeline();
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

    void cleanupSwapchain() {
        vkDestroySwapchainKHR(device, swapchain, nullptr);
    }

    void cleanupImageViews() {
        for (auto imageView : swapChainImageViews) {
            vkDestroyImageView(device, imageView, nullptr);
        }
    }


    void cleanup() {
        cleanupImageViews();
        cleanupSwapchain();
        cleanupDevices();
        cleanupSurfaces();
        cleanupDebugMessengers();
        cleanupInstances();
        cleanupGlfw();
    }



};


