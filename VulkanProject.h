#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES // This will force GLM to use a version of vec2 and mat4 that has the alignment requirements already specified, does not work for nested structures
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // OpenGL default is -1 to 1, but Vulkan uses 0 to 1
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <shaderc/shaderc.hpp>

#include <iostream>
#include <cstdlib>
#include <stdexcept>
#include <vector>
#include <map>
#include <set>
#include <optional>
#include <string>
#include <fstream>
#include <algorithm>
#include <array>
#include <chrono>

#include "VulkanProjectVariables.h"


#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

// Uniform object to pass to shaders
struct UniformBufferObject {
    // glm types must match shader binding types for easy memcpy of ubo into a VkBuffer
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    static const uint32_t attributeCount = 3; // set amount of attributes 

    // vertex binding describes at which rate to load data from memory throughout the vertices. It specifies the number of bytes between data entries and whether to move to the next data entry after each vertex or after each instance.
    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        // binding : specifies the index of the binding in the array of bindings.
        // stride : specifies the number of bytes from one entry to the next
        // inputRate : can have one of the following values:
        // VK_VERTEX_INPUT_RATE_VERTEX: Move to the next data entry after each vertex
        // VK_VERTEX_INPUT_RATE_INSTANCE : Move to the next data entry after each instance
        bindingDescription.binding = 0; 
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, attributeCount> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, attributeCount> attributeDescriptions{};
        // binding : tells Vulkan from which binding the per-vertex data comes.
        // location : references the location directive of the input in the vertex shader
        // The format parameter describes the type of data for the attribute, implicitly defines the byte size of attribute data: 
        // float: VK_FORMAT_R32_SFLOAT
        // vec2 : VK_FORMAT_R32G32_SFLOAT
        // vec3 : VK_FORMAT_R32G32B32_SFLOAT
        // vec4 : VK_FORMAT_R32G32B32A32_SFLOAT
        // ivec2: VK_FORMAT_R32G32_SINT, a 2 - component vector of 32 - bit signed integers
        // uvec4 : VK_FORMAT_R32G32B32A32_UINT, a 4 - component vector of 32 - bit unsigned integers
        // double : VK_FORMAT_R64_SFLOAT,
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0; // layout(location = 0) in vec2 inPosition;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT; // The input in the vertex shader with location 0 is the position, which has two 32-bit float components.
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1; // layout(location = 1) in vec3 inColor;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2; // layout(location = 2) in vec2 inTexCoord;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

        // add more attribute descriptions for more shader input variables

        return attributeDescriptions;
    }
};


std::vector<Vertex> vertices;
std::vector<uint32_t> indices;

//const std::vector<Vertex> vertices = {
//    {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
//    {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
//    {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
//    {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
//
//    {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
//    {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
//    {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
//    {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
//};
//
//const std::vector<uint16_t> indices = {
//    0, 1, 2, 2, 3, 0,
//    4, 5, 6, 6, 7, 4
//};


// Creator class to initialize and setup Vulkan specific objects related to drawing, such as framebuffers and command buffers/pools
class VulkanDrawingInitializer {
    friend class VulkanApplication;
public:
    // simple helper function that tells if the chosen depth format contains a stencil component:
    static bool hasStencilComponent(VkFormat format) {
        return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
    }

    //  helper function to select a format with a depth component that supports usage as depth attachment:
    static VkFormat findDepthFormat(VkPhysicalDevice* physicalDevice) {
        return findSupportedFormat(
            { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT,
            physicalDevice
        );
    }

    // find a supported format other than the most common VK_FORMAT_D32_SFLOAT for flexibility
    static VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features, VkPhysicalDevice* physicalDevice) {
        for (VkFormat format : candidates) {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(*physicalDevice, format, &props);

            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
                return format;
            }
            else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
                return format;
            }
        }
        // If none of the candidate formats support the desired usage, then just throw an exception
        throw std::runtime_error("failed to find supported format!");
    }
private:

    /////////////////////////////////////////////////
    /*         Section for loading Models                  */
    /////////////////////////////////////////////////

    void loadModel() {
        tinyobj::attrib_t attrib; // holds all of the positions, normals and texture coordinates in its attrib.vertices, attrib.normals and attrib.texcoords vectors
        std::vector<tinyobj::shape_t> shapes; // contains all of the separate objects and their faces. Each face consists of an array of vertices, and each vertex contains the indices of the position, normal and texture coordinate attributes.
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_FILE.c_str())) {
            throw std::runtime_error(warn + err);
        }

        for (const auto& shape : shapes) {
            for (const auto& index : shape.mesh.indices) {
                Vertex vertex{};

                vertex.pos = { // attrib.vertices array is an array of float values instead of something like glm::vec3, therefore multiplying by 3 is necessary
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
                };

                vertex.texCoord = { // multiplying by 2 is necessary here because of single floats for glm::vec2 UVs
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1.0f - attrib.texcoords[2 * index.texcoord_index + 1] // flip vertical component to match Vulkan's 0-up convention
                };

                vertex.color = { 1.0f, 1.0f, 1.0f };

                vertices.push_back(vertex);
                indices.push_back(indices.size());
            }
        }
    }

    /////////////////////////////////////////////////
    /*         Section for (depth) Images                  */
    /////////////////////////////////////////////////

    //Depth images should have the same resolution as the color attachment, defined by the swap chain extent, an image usage appropriate for a depth attachment, optimal tiling and device local memory.

    void createDepthResources(VkImage* depthImage, VkDeviceMemory* depthImageMemory, VkImageView* depthImageView, VkExtent2D* swapChainExtent, VkDevice* device, VkPhysicalDevice* physicalDevice) {
        VkFormat depthFormat = findDepthFormat(physicalDevice);

        createImage(swapChainExtent->width, swapChainExtent->height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, *depthImage, *depthImageMemory, device, physicalDevice);
        *depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, device);

        // layout transition not explicitly necessary as it is taken care of in the render pass
    }


    /////////////////////////////////////////////////
    /*         Section for (texture) Images                  */
    /////////////////////////////////////////////////

    void createTextureSampler(VkSampler* textureSampler, VkDevice* device, VkPhysicalDevice* physicalDevice) {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR; // specify how to interpolate texels that are magnified, NEAREST or LINEAR
        samplerInfo.minFilter = VK_FILTER_LINEAR; // specify how to interpolate texels that are minified, NEAREST or LINEAR

        // specify per-axis addressing
        // VK_SAMPLER_ADDRESS_MODE_REPEAT: Repeat the texture when going beyond the image dimensions. useful for e.g. walls or floors
        // VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT: Like repeat, but inverts the coordinates to mirror the image when going beyond the dimensions.
        // VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE : Take the color of the edge closest to the coordinate beyond the image dimensions.
        // VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE : Like clamp to edge, but instead uses the edge opposite to the closest edge.
        // VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER : Return a solid color when sampling beyond the dimensions of the image.
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

        // anisotropic filtering, use maximum available anisotropic value (= amount of texel samples that can be used to calculate the final color) for best results (at cost of performace)
        samplerInfo.anisotropyEnable = ENABLE_ANISOTRIPIC_FILTER;
        samplerInfo.maxAnisotropy = 1.0f;
        if (ENABLE_ANISOTRIPIC_FILTER) {
            VkPhysicalDeviceProperties properties{};
            vkGetPhysicalDeviceProperties(*physicalDevice, &properties);
            samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
        }

        // border color beyond sampling area of clamp to border, can be black, white or transparent
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

        // unnormalizedCoordinates specifies which coordinate system addresses texels in an image, if true [0, texHeight), if false [0, 1) range on all axes.
        // set TRUE to use textures of varying resolutions with the exact same coordinates.
        samplerInfo.unnormalizedCoordinates = VK_FALSE;

        // If a comparison function is enabled, then texels will first be compared to a value, and the result of that comparison is used in filtering operations.
        // used for shadow-maps 
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

        // mip-mapping filter
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;

        if (vkCreateSampler(*device, &samplerInfo, nullptr, textureSampler) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture sampler!");
        }
    }

    //FIXME: refactor code to call create image view from presentationdevices-initializer to remove duplicated code
    VkImageView createImageView(VkImage* image, VkFormat format, VkImageAspectFlags aspectFlags, VkDevice* device) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = *image; // color/render target
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; // treat images as 1D textures, 2D textures, 3D textures and cube maps.
        viewInfo.format = format; // pixel format, color space

        // Shift color channels to prefered likings e.g. map all channels to red to get monochrome textures etc.
        viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        // subresource describes what the image's purpose is and which part of the image should be accessed.
        viewInfo.subresourceRange.aspectMask = aspectFlags; // Use color or depth information
        viewInfo.subresourceRange.baseMipLevel = MIPMAP_LEVEL; // choose mipmap level for image views
        viewInfo.subresourceRange.levelCount = 1; // no multilayered images
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1; // for stereoscopic 3D applications, use multiple layers to access views for left and right eye

        VkImageView imageView;
        if (vkCreateImageView(*device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture image view!");
        }

        return imageView;
    }
       
    void createTextureImageView(VkImageView* textureImageView, VkImage* textureImage, VkDevice* device) {
        *textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, device);
    }

    // helper function, contents may be recorded into setup buffer and flused as single commandbuffer
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, VkCommandPool* commandPool, VkQueue* graphicsQueue, VkDevice* device) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands(commandPool, device);

        VkBufferImageCopy region{};
        region.bufferOffset = 0; // byte offset of pixel start-points
        region.bufferRowLength = 0; // specify in-memory layout, e.g. padding or tightly packed
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;

        region.imageOffset = { 0, 0, 0 };
        region.imageExtent = {
            width,
            height,
            1
        };

        vkCmdCopyBufferToImage(
            commandBuffer,
            buffer,
            image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, // speficy layout of currently used image
            1,
            &region // may contain an array of pixels to copy from buffer into image
        );

        endSingleTimeCommands(commandBuffer, commandPool, graphicsQueue, device);
    }

    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, VkDevice* device, VkPhysicalDevice* physicalDevice) {

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D; // e.g. use 1D Images for data or gradients, 2D images for textures and 3D for voxel volumes
        imageInfo.extent.width = static_cast<uint32_t>(width);
        imageInfo.extent.height = static_cast<uint32_t>(height);
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format; // use the same image format for texel as the pixels in the buffer, else copy will fail
        imageInfo.tiling = tiling; // optimal:  Texels are laid out in an implementation defined order for optimal access; linear: Texels are laid out in row-major order like our pixels array, use linear for direct access of texels in memory 
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; //undefined: discard texels on first transition, use when image is a transfer destination and texel data is copied into it from a buffer, preinitialized: keep texels on first transition, useful when having staging images 
        imageInfo.usage = usage | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; //used only by one queue family, which is graphics supporting
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT; //used for multisampling
        imageInfo.flags = 0; // Optional

        if (vkCreateImage(*device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image!");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(*device, image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, physicalDevice);

        if (vkAllocateMemory(*device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate image memory!");
        }

        vkBindImageMemory(*device, image, imageMemory, 0);
    }

    void createTextureImage(VkDeviceMemory* textureImageMemory, VkImage* textureImage, VkCommandPool* commandPool, VkQueue* graphicsQueue, VkDevice* device, VkPhysicalDevice* physicalDevice) {
        int texWidth, texHeight, texChannels;
        //stbi_uc* pixels = stbi_load("textures/texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        stbi_uc* pixels = stbi_load(TEXTURE_FILE.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha); // load pixels from texture file
        VkDeviceSize imageSize = texWidth * texHeight * 4; // STBI rgb alpha uses 4 bytes per pixel, increase in case of larger datatype

        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;

        createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferMemory, device, physicalDevice);
        void* data;
        vkMapMemory(*device, stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
        vkUnmapMemory(*device, stagingBufferMemory);
        stbi_image_free(pixels);

        createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, *textureImage, *textureImageMemory, device, physicalDevice);
        // old image layout is of no interest (in this patricular case), therefore use layout undefined
        transitionImageLayout(*textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, commandPool, graphicsQueue, device);
        copyBufferToImage(stagingBuffer, *textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), commandPool, graphicsQueue, device);
        // prepare texture image for shader access to start sampling from it 
        transitionImageLayout(*textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, commandPool, graphicsQueue, device);
        
        // cleanup resources for staging buffer
        vkDestroyBuffer(*device, stagingBuffer, nullptr);
        vkFreeMemory(*device, stagingBufferMemory, nullptr);
    }

    /////////////////////////////////////////////////
    /*         Section for Drawing Objects         */
    /////////////////////////////////////////////////

    // this method is used to modify uniform buffers to e.g. apply matrix transformations to objects, views or cameras
    void updateUniformBuffer(uint32_t currentImage, std::vector<void*>* uniformBuffersMapped, VkExtent2D* swapChainExtent) {
        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        UniformBufferObject ubo{};
        // apply model matrix changes here
        // rotate the object by 90 degrees per second
        // use your creativity here to play with uniform modifications of the object
        auto rotationAngle = time * glm::radians(90.0f);
        auto rotationAxis = glm::vec3(0.0f, 0.0f, 1.0f);
        ubo.model = glm::rotate(glm::mat4(1.0f), rotationAngle, rotationAxis);

        // apply view matrix transformations here
        // loot at the object from above at 45 degree angle
        // use your creativity here to play with uniform modifications of the view
        auto eyePosition = glm::vec3(2.0f, 2.0f, 2.0f);
        auto centerPosition = glm::vec3(0.0f, 0.0f, 0.0f);
        auto upAxis = glm::vec3(0.0f, 0.0f, 1.0f);
        ubo.view = glm::lookAt(eyePosition, centerPosition, upAxis);

        // apply projection matrix changes here
        // use perspective projection with field-of-view of 45 degrees
        // use your creativity here to play with uniform modifications of the projection
        auto fieldOfView = glm::radians(90.0f);
        auto aspectRatio = swapChainExtent->width / (float)swapChainExtent->height;
        auto nearPlane = 0.1f;
        auto farPlane = 10.0f;
        ubo.proj = glm::perspective(fieldOfView, aspectRatio, nearPlane, farPlane);

        // GLM is designed for OpenGL, which has clip coordinates Y-inverted compared to Vulkan.
        ubo.proj[1][1] *= -1;

        // copy data into uniform buffer without staging buffer (increases performance as it will be called each frame)
        memcpy(uniformBuffersMapped->at(currentImage), &ubo, sizeof(ubo));
    }

    //////////////////////////////////////////////////////////
   /*  Sub-Section for Descriptor Pool/Set/Layout creation  */
   ///////////////////////////////////////////////////////////

    // Use descriptor pools to allocate descriptor sets 
    void createDescriptorPool(VkDescriptorPool* descriptorPool, VkDevice* device) {
        // create one poolsize for each descriptor, here we use one for uniform buffer and one for combined image sampler (for textures)
        std::array<VkDescriptorPoolSize, 2> poolSizes{};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);  // create descriptor set for each frame in flight with the same layout, not explicity necesary but recommended for best practice
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);  // create descriptor set for each frame in flight with the same layout, not explicity necesary but recommended for best practice

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        if (vkCreateDescriptorPool(*device, &poolInfo, nullptr, descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }
    }

    void createDescriptorSets(VkSampler* textureSampler, VkImageView* textureImageView, std::vector<VkDescriptorSet>* descriptorSets, VkDescriptorPool* descriptorPool, VkDescriptorSetLayout* descriptorSetLayout, std::vector<VkBuffer>* uniformBuffers, VkDevice* device) {
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, *descriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = *descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        allocInfo.pSetLayouts = layouts.data();

        descriptorSets->resize(MAX_FRAMES_IN_FLIGHT);
        if (vkAllocateDescriptorSets(*device, &allocInfo, descriptorSets->data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            // setup descriptor resources for uniform buffer
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = uniformBuffers->at(i);
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            // setup descriptor resources for combined image sampler
            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = *textureImageView;
            imageInfo.sampler = *textureSampler;

            std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
            // setup descriptor resources for uniform buffer
            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = descriptorSets->at(i);
            descriptorWrites[0].dstBinding = 0; // binding index for uniforms, change this if you bind them to a new location in the shader
            descriptorWrites[0].dstArrayElement = 0; // descriptors can be arrays, element specifies the first index to be updated, not used here
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1; // only one descriptor used here
            descriptorWrites[0].pBufferInfo = &bufferInfo;
            descriptorWrites[0].pImageInfo = nullptr; // Optional, used for descriptors that refer to image data
            descriptorWrites[0].pTexelBufferView = nullptr; // Optional, used for descriptors that refer to buffer views

            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = descriptorSets->at(i);
            descriptorWrites[1].dstBinding = 1; // binding index for texture, change this if you bind them to a new location in the shader
            descriptorWrites[1].dstArrayElement = 0; // descriptors can be arrays, element specifies the first index to be updated, not used here
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pImageInfo = &imageInfo;

            vkUpdateDescriptorSets(*device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }

    }

    void createDescriptorSetLayout(VkDescriptorSetLayout* descriptorSetLayout,VkDevice* device) {
        // uniform buffer binding descriptor
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.descriptorCount = 1; // use 1 for signle unifor buffer objects, increase for multiple transformations for each object.
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; // specify where descriptors are used
        uboLayoutBinding.pImmutableSamplers = nullptr; // Optional, used for image sampling

        // combined image sampler descriptor
        VkDescriptorSetLayoutBinding samplerLayoutBinding{};
        samplerLayoutBinding.binding = 1;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.pImmutableSamplers = nullptr;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT; // indicate that sampler descriptor will be used in fragment shader, use in vertex shader to e.g. deform grids for heightmaps 

        std::array<VkDescriptorSetLayoutBinding, 2> bindings = { 
            uboLayoutBinding, 
            samplerLayoutBinding };
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(*device, &layoutInfo, nullptr, descriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }
    }

    ///////////////////////////////////////
   /*   Sub-Section for Buffer creation  */
   ///////////////////////////////////////

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, VkPhysicalDevice* physicalDevice) {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(*physicalDevice, &memProperties);

        //find memory type that is suitable and check if writing data into it is possible
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");

    }
    
    // helper function, contents may be recorded into setup buffer and flused as single commandbuffer
    VkCommandBuffer beginSingleTimeCommands(VkCommandPool* commandPool, VkDevice* device) {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = *commandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(*device, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        return commandBuffer;
    }

    // helper function, contents may be recorded into setup buffer and flused as single commandbuffer
    void endSingleTimeCommands(VkCommandBuffer commandBuffer, VkCommandPool* commandPool, VkQueue* graphicsQueue, VkDevice* device) {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(*graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(*graphicsQueue);

        vkFreeCommandBuffers(*device, *commandPool, 1, &commandBuffer);
    }

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkCommandPool* commandPool, VkQueue* graphicsQueue, VkDevice* device) {

        VkCommandBuffer commandBuffer = beginSingleTimeCommands(commandPool, device);

        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0; // Optional
        copyRegion.dstOffset = 0; // Optional
        copyRegion.size = size; //  It is not possible to specify VK_WHOLE_SIZE here, unlike the vkMapMemory command.
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
        
        endSingleTimeCommands(commandBuffer, commandPool, graphicsQueue, device);

    }

    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer* buffer, VkDeviceMemory* bufferMemory, VkDevice* device, VkPhysicalDevice* physicalDevice) {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage; //  indicates for which purposes the data in the buffer is going to be used
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // buffers can also be owned by specific wqueue family or shared between multiple at the same time
        bufferInfo.flags = 0;

        if (vkCreateBuffer(*device, &bufferInfo, nullptr, buffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create vertex buffer!");
        }

        VkMemoryRequirements memRequirements;
        // size: The size of the required amount of memory in bytes, may differ from bufferInfo.size.
        // alignment: The offset in bytes where the buffer begins in the allocated region of memory, depends on bufferInfo.usage and bufferInfo.flags.
        // memoryTypeBits : Bit field of the memory types that are suitable for the buffer.
        vkGetBufferMemoryRequirements(*device, *buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties, physicalDevice);

        // Don't call vkAllicateMemory for every buffer:
        // check out https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator 
        if (vkAllocateMemory(*device, &allocInfo, nullptr, bufferMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate vertex buffer memory!");
        }

        vkBindBufferMemory(*device, *buffer, *bufferMemory, 0);
    }

    void createUniformBuffers(std::vector<void*>* uniformBuffersMapped, std::vector<VkDeviceMemory>* uniformBuffersMemory, std::vector<VkBuffer>* uniformBuffers, VkDevice* device, VkPhysicalDevice* physicalDevice) {
        VkDeviceSize bufferSize = sizeof(UniformBufferObject);
        // create uniform buffers for as many frames in flight to prevent writing into a buffer that is currently being read
        uniformBuffers->resize(MAX_FRAMES_IN_FLIGHT);
        uniformBuffersMemory->resize(MAX_FRAMES_IN_FLIGHT);
        uniformBuffersMapped->resize(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &uniformBuffers->at(i), &uniformBuffersMemory->at(i), device, physicalDevice);
            // persist mapping for the lifetime of the application to increase performance
            vkMapMemory(*device, uniformBuffersMemory->at(i), 0, bufferSize, 0, &uniformBuffersMapped->at(i));
        }
    }

    void createIndexBuffer(VkDeviceMemory* indexBufferMemory, VkBuffer* indexBuffer, VkCommandPool* commandPool, VkQueue* graphicsQueue, VkDevice* device, VkPhysicalDevice* physicalDevice) {
        VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

        //upload cpu buffer (host visible) into gpu buffer (device local) 
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;

        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferMemory, device, physicalDevice);

        // access a region of the specified memory resource defined by an offset and size, use VK_WHOLE_SIZE to map all of the memory
        void* data;
        vkMapMemory(*device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, indices.data(), (size_t)bufferSize); //copy contents of buffer into accessible field
        vkUnmapMemory(*device, stagingBufferMemory);

        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory, device, physicalDevice);

        copyBuffer(stagingBuffer, *indexBuffer, bufferSize, commandPool, graphicsQueue, device);

        vkDestroyBuffer(*device, stagingBuffer, nullptr);
        vkFreeMemory(*device, stagingBufferMemory, nullptr);
    }

    void createVertexBuffer(VkDeviceMemory* vertexBufferMemory, VkBuffer* vertexBuffer, VkCommandPool* commandPool, VkQueue* graphicsQueue, VkDevice* device, VkPhysicalDevice* physicalDevice) {
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

        //upload cpu buffer (host visible) into gpu buffer (device local) 
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;

        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferMemory, device, physicalDevice);

        // access a region of the specified memory resource defined by an offset and size, use VK_WHOLE_SIZE to map all of the memory
        void* data;
        vkMapMemory(*device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, vertices.data(), (size_t)bufferSize); //copy contents of buffer into accessible field
        vkUnmapMemory(*device, stagingBufferMemory);

        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory, device, physicalDevice);

        copyBuffer(stagingBuffer, *vertexBuffer, bufferSize, commandPool, graphicsQueue, device);

        vkDestroyBuffer(*device, stagingBuffer, nullptr);
        vkFreeMemory(*device, stagingBufferMemory, nullptr);
    }
     
    // setup layout transitions to copy buffers into images 
    // helper function, contents may be recorded into setup buffer and flused as single commandbuffer
    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, VkCommandPool* commandPool, VkQueue* graphicsQueue, VkDevice* device) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands(commandPool, device);

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout; // old layout may be layout_undefined if don't care about existing contents
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // set queue family if family ownership transfer should be applied
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        // transition: Undefined --> transfer destination: transfer writes that don't need to wait on anything
        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        // transition Transfer destination --> shader reading: shader reads should wait on transfer writes, specifically the shader reads in the fragment shader, because that's where we're going to use the texture
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else {
            throw std::invalid_argument("unsupported layout transition!");
        }

        vkCmdPipelineBarrier(
            commandBuffer,
            sourceStage /* before-barrier-stage */, 
            destinationStage /* wait-for-barrier-stage */,
            0, // 0, or VK_DEPENDENCY_BY_REGION_BIT which turns barrier into per-region condition (e.g. read from parts or a region that were already written)
            0, nullptr, // array reference of barriers of types memory barriers, buffer memory barriers or image barriers
            0, nullptr, // array reference of barriers of types memory barriers, buffer memory barriers or image barriers
            1, &barrier
        );

        endSingleTimeCommands(commandBuffer, commandPool, graphicsQueue, device);
    }

    void createSyncObjects(std::vector<VkSemaphore>* imageAvailableSemaphores, std::vector<VkSemaphore>* renderFinishedSemaphores, std::vector<VkFence>* inFlightFences, VkDevice* device) {
        imageAvailableSemaphores->resize(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores->resize(MAX_FRAMES_IN_FLIGHT);
        inFlightFences->resize(MAX_FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // set signaled to prevent infinite waiting loop on first frame

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (vkCreateSemaphore(*device, &semaphoreInfo, nullptr, &imageAvailableSemaphores->at(i)) != VK_SUCCESS ||
                vkCreateSemaphore(*device, &semaphoreInfo, nullptr, &renderFinishedSemaphores->at(i)) != VK_SUCCESS ||
                vkCreateFence(*device, &fenceInfo, nullptr, &inFlightFences->at(i)) != VK_SUCCESS) {

                throw std::runtime_error("failed to create synchronization objects for a frame!");
            }
        }
    }

    void createCommandBuffers(std::vector<VkCommandBuffer>* commandBuffers, VkCommandPool* commandPool, VkDevice* device) {
        commandBuffers->resize(MAX_FRAMES_IN_FLIGHT);

        // The level parameter specifies if the allocated command buffers are primary or secondary command buffers.
        // VK_COMMAND_BUFFER_LEVEL_PRIMARY: Can be submitted to a queue for execution, but cannot be called from other command buffers.
        // VK_COMMAND_BUFFER_LEVEL_SECONDARY : Cannot be submitted directly, but can be called from primary command buffers.
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = *commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; //use secondary to e.g. reuse common opertations from primary command buffers
        allocInfo.commandBufferCount = (uint32_t)commandBuffers->size();

        if (vkAllocateCommandBuffers(*device, &allocInfo, commandBuffers->data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }
    }

    // contains actual draw command containing info from renderpass, and buffers
    void recordCommandBuffer(uint32_t currentFrame, uint32_t imageIndex, std::vector<VkDescriptorSet>* descriptorSets, VkBuffer* indexBuffer, VkBuffer* vertexBuffer, VkCommandBuffer* commandBuffer, VkCommandPool* commandPool, VkPipeline* graphicsPipeline, VkRenderPass* renderPass, VkPipelineLayout* pipelineLayout, std::vector<VkFramebuffer>* swapchainFramebuffers, VkExtent2D* swapChainExtent, VkDevice* device) {
        // The flags parameter specifies how the command buffer is used:
        // VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT: The command buffer will be rerecorded right after executing it once.
        // VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT : This is a secondary command buffer that will be entirely within a single render pass.
        // VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT : The command buffer can be resubmitted while it is also already pending execution
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0; // Optional
        beginInfo.pInheritanceInfo = nullptr; // Optional, relevant only for secondary cmd buffers, specifies which state to inherit from the calling primary command buffers.

        if (vkBeginCommandBuffer(*commandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = *renderPass;
        renderPassInfo.framebuffer = swapchainFramebuffers->at(imageIndex);
        renderPassInfo.renderArea.offset = { 0, 0 }; // render area should match the size of the attachments for best performance.
        renderPassInfo.renderArea.extent = *swapChainExtent; // The pixels outside render area will have undefined values.
        
        // the order of clearValues should be identical to the order of the attachments.
        std::array<VkClearValue, 2> clearValues{}; // clear image to that color before writing to it. If an area is not drawn, this can also refer to "background color" 
        clearValues[0].color = CLEAR_COLOR;
        clearValues[1].depthStencil = { 1.0f, 0 }; // The range of depths in the depth buffer is 0.0 to 1.0 in Vulkan, where 1.0 lies at the far view plane and 0.0 at the near view plane.The initial value at each point in the depth buffer should be the furthest possible depth, which is 1.0.
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        // Subpass contents parameter controls how the drawing commands within the render pass will be provided.
        // VK_SUBPASS_CONTENTS_INLINE: The render pass commands will be embedded in the primary command buffer itself and no secondary command buffers will be executed.
        // VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS : The render pass commands will be executed from secondary command buffers.
        vkCmdBeginRenderPass(*commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(*commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *graphicsPipeline);

        VkBuffer vertexBuffers[] = { *vertexBuffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(*commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(*commandBuffer, *indexBuffer, 0, VK_INDEX_TYPE_UINT32);


        // define dynamic states
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(swapChainExtent->width);
        viewport.height = static_cast<float>(swapChainExtent->height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(*commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = *swapChainExtent;
        vkCmdSetScissor(*commandBuffer, 0, 1, &scissor);

        vkCmdBindDescriptorSets(*commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipelineLayout, 0, 1, &descriptorSets->at(currentFrame), 0, nullptr);

        //actual draw command
        // vertexCount
        // instanceCount : Used for instanced rendering, use 1 if you're not doing that.
        // firstVertex : Used as an offset into the vertex buffer, defines the lowest value of gl_VertexIndex.
        // firstInstance : Used as an offset for instanced rendering, defines the lowest value of gl_InstanceIndex.
        // 
       
        if (USE_INDEXED_VERTICES) {
            // reuse vertices by using their indices and place them in order specified by "indices" array
            // saves about 50% of memory for vertices
            // std::cout << "Using indexed vertices in draw command. Please make sure to specify the order of vertices." << std::endl;
            vkCmdDrawIndexed(*commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
        }
        else {
            // use non-indexed vertices
            // make sure to add the correct amount of vertices for each primitive/triangle.
            // --> three vertices for each triangle, e.g. 6 vertices for a square, etc...
            // std::cout << "Using non-indexed vertices in draw command. Please make sure to specify the amount and layout of vertices correctly when using this option." << std::endl;
            vkCmdDraw(*commandBuffer, static_cast<uint32_t>(vertices.size()), 1, 0, 0);
        }


        vkCmdEndRenderPass(*commandBuffer);

        if (vkEndCommandBuffer(*commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }
    }


    void createFramebuffers(VkImageView* depthImageView, std::vector<VkFramebuffer>* swapchainFramebuffers, VkExtent2D* swapChainExtent, std::vector<VkImageView>* swapChainImageViews, VkRenderPass* renderPass, VkDevice* device) {
        swapchainFramebuffers->resize(swapChainImageViews->size());
        for (size_t i = 0; i < swapChainImageViews->size(); i++) {//create framebuffer for each image view
            std::array<VkImageView, 2> attachments = {
                swapChainImageViews->at(i),
                *depthImageView
            };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = *renderPass;
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = swapChainExtent->width;
            framebufferInfo.height = swapChainExtent->height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(*device, &framebufferInfo, nullptr, &swapchainFramebuffers->at(i)) != VK_SUCCESS) {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
    }
};

// Creator class to initialize and setup Vulkan specific objects related to graphics pipeline
class VulkanGraphicsPipelineInitializer {
    friend class VulkanApplication;
public:

private:

    /////////////////////////////////////////////////
    /*    Section for Graphics Pipeline Objects    */
    /////////////////////////////////////////////////

    // Render pass: the attachments referenced by the pipeline stages and their usage
    void createRenderPass(VkRenderPass* renderPass, VkFormat* swapChainImageFormat, VkDevice* device, VkPhysicalDevice* physicalDevice) {
        // single color buffer attachment by one ima from swapchain
        VkAttachmentDescription colorAttachment{};
        VkAttachmentDescription depthAttachment{};

        colorAttachment.format = *swapChainImageFormat; //format should match swap chain image format
        depthAttachment.format = VulkanDrawingInitializer::findDepthFormat(physicalDevice); // The format should be the same as the depth image itself

        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT; //no multisampling for now
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

        // loading operation before rendering
        // VK_ATTACHMENT_LOAD_OP_LOAD: Preserve the existing contents of the attachment
        // VK_ATTACHMENT_LOAD_OP_CLEAR : Clear the values to a constant at the start
        // VK_ATTACHMENT_LOAD_OP_DONT_CARE : Existing contents are undefined; we don't care about them
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; //clear contents of image
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;

        // storing operation after rendering
        // VK_ATTACHMENT_STORE_OP_STORE: Rendered contents will be stored in memory and can be read later
        // VK_ATTACHMENT_STORE_OP_DONT_CARE : Contents of the framebuffer will be undefined after the rendering operation
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; //store to show on screen 
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // don't care about storing the depth data (storeOp), because it will not be used after drawing has finished


        // stencil buffer is not in use at the moment
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;

        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        // define pixel layout
        // VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: Images used as color attachment
        // VK_IMAGE_LAYOUT_PRESENT_SRC_KHR: Images to be presented in the swap chain
        // VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL : Images to be used as destination for a memory copy operation
        // VK_IMAGE_LAYOUT_UNDEFINED : Do not care about layout of image (that e.g. previous image was in)
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // specify format before render pass begins, undefined if load op is clear
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // layout transition to when renderpass finishes
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        // Render subpasses
        // subpasses reference one or more attachments
        VkAttachmentReference colorAttachmentReference{};
        colorAttachmentReference.attachment = 0; // reference to the color attachment-array index, see fragment shader layout(location = 0)
        colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; //specifies which layout we would like the attachment to have during a subpass that uses this reference

        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 1; // reference to the texture attachment-array index, see fragment shader layout(location = 1)
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        // Subpasses can these attachments attached
        // pColorAttachments
        // pInputAttachments: Attachments that are read from a shader
        // pResolveAttachments: Attachments used for multisampling color attachments
        // pDepthStencilAttachment : Attachment for depth and stencil data
        // pPreserveAttachments : Attachments that are not used by this subpass, but for which the data must be preserved
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentReference;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;


        // Steer transition of renderpass using a dependency to wait for a specific stage
        // The dstSubpass must always be higher than srcSubpass to prevent cycles in the dependency graph (unless one of the subpasses is VK_SUBPASS_EXTERNAL)
        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = 0;
        // These settings will prevent the transition from happening until it's actually necessary (and allowed): when we want to start writing colors to it.
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        // use color and depth attachment for renderpass
        std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(*device, &renderPassInfo, nullptr, renderPass) != VK_SUCCESS) {
            throw std::runtime_error("failed to create render pass!");
        }
    }
    // Fixed-function stage : all of the structures that define the fixed - function stages of the pipeline, like input assembly, rasterizer, viewport and color blending
    void setupFixedFunctionStage(std::vector<VkDynamicState>& dynamicStates,
        VkPipelineInputAssemblyStateCreateInfo& inputAssemblyInfo,
        VkPipelineDynamicStateCreateInfo& dynamicStateInfo,
        VkPipelineViewportStateCreateInfo& viewportState,
        VkPipelineRasterizationStateCreateInfo& rasterizerInfo,
        VkPipelineMultisampleStateCreateInfo& multisamplingInfo,
        VkPipelineDepthStencilStateCreateInfo& depthStencilInfo,
        VkPipelineColorBlendAttachmentState& colorBlendAttachment,
        VkPipelineColorBlendStateCreateInfo& colorBlendingInfo, VkExtent2D* swapChainExtent) {
        //////////////////////// INPUT ASSEMBLY
        //
        // specify geometry topology and primitive reuse
        // topologies:
        // VK_PRIMITIVE_TOPOLOGY_POINT_LIST: points from vertices
        // VK_PRIMITIVE_TOPOLOGY_LINE_LIST : line from every 2 vertices without reuse
        // VK_PRIMITIVE_TOPOLOGY_LINE_STRIP : the end vertex of every line is used as start vertex for the next line
        // VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST : triangle from every 3 vertices without reuse
        // VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP : the second and third vertex of every triangle are used as first two vertices of the next triangle
        inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

        //////////////////////// DYNAMIC STATES
        //
        // create dynamic state to dynamically change viewport and scissor without recreating the whole pipeline at drawing time
        dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicStateInfo.pDynamicStates = dynamicStates.data();

        ////////////////////////// VIEWPORT
        //// 
        //// specify the rendered region of the framebuffer
        //// set width to swapchain extent to fill the whole window
        //// "squash the whole image into a region"

        // dynamically load viewport and scissor
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        //// statically load viewport and scissor - needs pipeline recreation on change
        //// 
        ////viewport.x = 0.0f;
        ////viewport.y = 0.0f;
        ////viewport.width = (float)swapChainExtent->width;
        ////viewport.height = (float)swapChainExtent->height;
        ////viewport.minDepth = 0.0f;
        ////viewport.maxDepth = 1.0f;

        //////////////////////////// SCISSOR
        ////// 
        ////// cut the visible region of the framebuffer
        ////scissor.offset = { 0, 0 };
        ////scissor.extent = *swapChainExtent; //draw entire framebuffer without cutting

        ////viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        ////viewportState.viewportCount = 1;
        //////viewportState.pViewports = &viewport;
        ////viewportState.scissorCount = 1;
        //////viewportState.pScissors = &scissor;

        //////////////////////// RASTERIZER
        //
        // Rasterizer creates fragments out of vertices and also performs depth testing, face culling and scissor test.
        // Fill modes:
        // VK_POLYGON_MODE_FILL: fill the area of the polygon with fragments
        // VK_POLYGON_MODE_LINE : polygon edges are drawn as lines
        // VK_POLYGON_MODE_POINT : polygon vertices are drawn as points
        rasterizerInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizerInfo.depthClampEnable = VK_FALSE; // clamp instead of discard fragments to far or near plane if are beyond, useful for e.g. shadow maps
        rasterizerInfo.rasterizerDiscardEnable = VK_FALSE; // discard geometry passing through rasterizer, disables output to framebuffer.
        rasterizerInfo.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizerInfo.lineWidth = 1.0f; // thickness of lines in terms of number of fragments
        rasterizerInfo.cullMode = CULL_MODE; //specify cull mode such as front-, back- or front-and-back culling
        rasterizerInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; // use counter clockwise to correct reversed draw oder caused by y-flip
        rasterizerInfo.depthBiasEnable = VK_FALSE; // bias depth by adding a constant value, e.g. for shadow maps
        rasterizerInfo.depthBiasConstantFactor = 0.0f; // Optional
        rasterizerInfo.depthBiasClamp = 0.0f; // Optional
        rasterizerInfo.depthBiasSlopeFactor = 0.0f; // Optional

        //////////////////////// DEPTH AND STENCIL
        //
        depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencilInfo.depthTestEnable = VK_TRUE; // specifies if the depth of new fragments should be compared to the depth buffer to see if they should be discarded
        depthStencilInfo.depthWriteEnable = VK_TRUE; // specifies if the new depth of fragments that pass the depth test should actually be written to the depth buffer
        depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS; // specifies the comparison that is performed to keep or discard fragments. Use convention of lower depth = closer
        depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
        depthStencilInfo.minDepthBounds = 0.0f; // Optional, used for the optional depth bound test. Basically, this allows to only keep fragments that fall within the specified depth range. 
        depthStencilInfo.maxDepthBounds = 1.0f; // Optional, used for the optional depth bound test. Basically, this allows to only keep fragments that fall within the specified depth range. 
        depthStencilInfo.stencilTestEnable = VK_FALSE;
        depthStencilInfo.front = {}; // Optional
        depthStencilInfo.back = {}; // Optional

        //////////////////////// MULTISAMPLING
        //
        // One Way to perform anti-aliasing, combine fragment shader results of multiple polygons to the same pixel
        multisamplingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisamplingInfo.sampleShadingEnable = VK_FALSE;
        multisamplingInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisamplingInfo.minSampleShading = 1.0f; // Optional
        multisamplingInfo.pSampleMask = nullptr; // Optional
        multisamplingInfo.alphaToCoverageEnable = VK_FALSE; // Optional
        multisamplingInfo.alphaToOneEnable = VK_FALSE; // Optional

        //////////////////////// COLOR BLENDING
        //
        // combine fragment shader output with framebuffer color

        //contains the configuration per attached framebuffer
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

        // if using alpha blending
        //colorBlendAttachmentInfo.blendEnable = VK_TRUE;
        //colorBlendAttachmentInfo.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        //colorBlendAttachmentInfo.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        //colorBlendAttachmentInfo.colorBlendOp = VK_BLEND_OP_ADD;
        //colorBlendAttachmentInfo.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        //colorBlendAttachmentInfo.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        //colorBlendAttachmentInfo.alphaBlendOp = VK_BLEND_OP_ADD;

        //contains the global color blending settings
        colorBlendingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlendingInfo.logicOpEnable = VK_FALSE; //false applies to ALL attached framebuffers. Set to true if using e.g. alpha blending
        colorBlendingInfo.logicOp = VK_LOGIC_OP_COPY; // Optional
        colorBlendingInfo.attachmentCount = 1;
        colorBlendingInfo.pAttachments = &colorBlendAttachment;
        colorBlendingInfo.blendConstants[0] = 0.0f; // Optional
        colorBlendingInfo.blendConstants[1] = 0.0f; // Optional
        colorBlendingInfo.blendConstants[2] = 0.0f; // Optional
        colorBlendingInfo.blendConstants[3] = 0.0f; // Optional
    }

    // Shader stages : the shader modules that define the functionality of the programmable stages of the graphics pipeline
    std::vector<VkShaderModule> setupShaderStageAndReturnModules(std::array<VkVertexInputAttributeDescription, Vertex::attributeCount> attributeDescriptions, VkVertexInputBindingDescription bindingDescription, VkPipelineVertexInputStateCreateInfo& vertexInputInfo, VkPipelineShaderStageCreateInfo& fragmentShaderStageInfo, VkPipelineShaderStageCreateInfo& vertexShaderStageInfo, VkDevice* device) {
        std::string vertexShaderText = readShaderFile(VERTEX_SHADER_FILE);
        std::string fragmentShaderText = readShaderFile(FRAGMENT_SHADER_FILE);

        auto vertexShaderCode = compileShader(vertexShaderText, "vertex");
        auto fragmentShaderCode = compileShader(fragmentShaderText, "fragment");

        VkShaderModule vertexShaderModule = createShaderModule({ vertexShaderCode.cbegin(),vertexShaderCode.cend() }, *device);
        VkShaderModule fragmentShaderModule = createShaderModule({ fragmentShaderCode.cbegin(),fragmentShaderCode.cend() }, *device);

        vertexShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertexShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertexShaderStageInfo.module = vertexShaderModule;
        vertexShaderStageInfo.pName = "main"; // choose entry point function within shader
        vertexShaderStageInfo.pSpecializationInfo = nullptr; // add shader constants if used, to get optimization features by compiler

        fragmentShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragmentShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragmentShaderStageInfo.module = fragmentShaderModule;
        fragmentShaderStageInfo.pName = "main";
        vertexShaderStageInfo.pSpecializationInfo = nullptr; // add shader constants if used, to get optimization features by compiler

        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription; // Bindings: spacing between data and whether the data is per-vertex or per-instance
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data(); // Attribute descriptions: type of the attributes passed to the vertex shader, which binding to load them from and at which offset
    
        return std::vector<VkShaderModule>{vertexShaderModule, fragmentShaderModule};
    }

    // Thin wrapper for the actual SPIRV code of a shader
    VkShaderModule createShaderModule(const std::vector<uint32_t>& code, VkDevice device) {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size() * sizeof(uint32_t); // get correct codelength
        createInfo.pCode = code.data();

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shader module!");
        }
        
        return shaderModule;
    }

    // Read shader file to compile within the program itself
    static std::string readShaderFile(const std::string& filename) {
        std::ifstream file(filename);

        if (!file.is_open()) {
            throw std::runtime_error("failed to open!" + filename);
        }

        std::string shaderText((std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>());

        return shaderText;
    }
    // FIXME: remember to add information to use shaderc_combinedd.lib for debug and shaderc_combinedd.lib for release in the linker Input @properties of VS!!
    // compile shader text into spirv code format
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

        std::cout << "Compiling Shader: " << shader_type << std::endl;
        shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(source_text, shader_kind, shader_type, options);

        if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
            throw std::runtime_error(result.GetErrorMessage());
        }
        std::cout << "Shader compiled: " << shader_type << std::endl;

        return result ;
    }


    // Setup grapics pipeline stages such as shader stage, fixed function stage, pipeline layout and renderpasses
    void createGraphicsPipeline(VkPipeline* graphicsPipeline, VkRenderPass* renderPass, VkDescriptorSetLayout* descriptorSetLayout,VkPipelineLayout* pipelineLayout, VkExtent2D* swapChainExtent, VkDevice* device) {
        //////////////////////// SHADER STAGE
        VkPipelineShaderStageCreateInfo vertexShaderStageInfo{};
        VkPipelineShaderStageCreateInfo fragmentShaderStageInfo{};
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        auto bindingDescription = Vertex::getBindingDescription();
        auto attributeDescriptions = Vertex::getAttributeDescriptions();

        std::vector<VkShaderModule> shaderModules = setupShaderStageAndReturnModules(attributeDescriptions, bindingDescription, vertexInputInfo, fragmentShaderStageInfo, vertexShaderStageInfo, device);
        VkPipelineShaderStageCreateInfo shaderStages[] = { vertexShaderStageInfo, fragmentShaderStageInfo };

        //////////////////////// FIXED FUNCTION STAGE
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
        VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
        std::vector<VkDynamicState> dynamicStates;
        VkPipelineViewportStateCreateInfo viewportState{};
        VkPipelineRasterizationStateCreateInfo rasterizerInfo{};
        VkPipelineMultisampleStateCreateInfo multisamplingInfo{};
        VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        VkPipelineColorBlendStateCreateInfo colorBlendingInfo{};

        setupFixedFunctionStage(dynamicStates, inputAssemblyInfo, dynamicStateInfo, viewportState, rasterizerInfo, multisamplingInfo, depthStencilInfo, colorBlendAttachment, colorBlendingInfo, swapChainExtent);

        //////////////////////// PIPELINE LAYOUT
        // Pipeline layout : the uniform and push values referenced by the shader that can be updated at draw time

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = descriptorSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
        pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

        if (vkCreatePipelineLayout(*device, &pipelineLayoutInfo, nullptr, pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }

        //////////////////////// PIPELINE CREATION

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizerInfo;
        pipelineInfo.pMultisampleState = &multisamplingInfo;
        pipelineInfo.pDepthStencilState = &depthStencilInfo;
        pipelineInfo.pColorBlendState = &colorBlendingInfo;
        pipelineInfo.pDynamicState = &dynamicStateInfo;
        pipelineInfo.layout = *pipelineLayout;
        pipelineInfo.renderPass = *renderPass;
        pipelineInfo.subpass = 0; // index of the sub pass where this graphics pipeline will be used
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional, specify the handle of an existing pipeline with basePipelineHandle or reference another pipeline that is about to be created by index with basePipelineIndex
        pipelineInfo.basePipelineIndex = -1; // Optional, Right now there is only a single pipeline, so we'll simply specify a null handle and an invalid index. These values are only used if the VK_PIPELINE_CREATE_DERIVATIVE_BIT flag is also specified in the flags field of VkGraphicsPipelineCreateInfo.

        if (vkCreateGraphicsPipelines(*device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, graphicsPipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }

        // destroy shader modules after pipeline is created.
        for (VkShaderModule module : shaderModules) {
            vkDestroyShaderModule(*device, module, nullptr);
        }
    };
};

// Creator class to initialize and setup Vulkan specific objects related to physical and logical devices, window surfaces, swap chains and image views
class VulkanPresentationDevicesInitializer {
    friend class VulkanApplication;
public:
 
private:
    ///////////////////////////////////////
    /*      Sub-Section for Command Pool creation  */
    ///////////////////////////////////////
    void createCommandPool(VkCommandPool* commandPool, VkSurfaceKHR* surface, VkDevice* device, VkPhysicalDevice* physicalDevice) {
        QueueFamilyIndices queueFamilyIndices = findQueueFamilies(*surface, *physicalDevice);

        // VK_COMMAND_POOL_CREATE_TRANSIENT_BIT: Hint that command buffers are rerecorded with new commands very often (may change memory allocation behavior)
        // VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT: Allow command buffers to be rerecorded individually, without this flag they all have to be reset together
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // record a command buffer every frame
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

        if (vkCreateCommandPool(*device, &poolInfo, nullptr, commandPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create command pool!");
        }
    }
    //FIXME: add description for why to use short lived cmd pool
    void createShortLivedCommandPool(VkCommandPool* commandPool, VkSurfaceKHR* surface, VkDevice* device, VkPhysicalDevice* physicalDevice) {
        QueueFamilyIndices queueFamilyIndices = findQueueFamilies(*surface, *physicalDevice);

        // VK_COMMAND_POOL_CREATE_TRANSIENT_BIT: Hint that command buffers are rerecorded with new commands very often (may change memory allocation behavior)
        // VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT: Allow command buffers to be rerecorded individually, without this flag they all have to be reset together
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

        if (vkCreateCommandPool(*device, &poolInfo, nullptr, commandPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create command pool!");
        }
    }


    ///////////////////////////////////////
    /*      Section for Window Surfaces, Swap Chains and Image Views  */
    ///////////////////////////////////////


    void createImageViews(std::vector<VkImageView>* swapChainImageViews, VkFormat* swapChainImageFormat, std::vector<VkImage>* swapChainImages, VkDevice* device) {
        swapChainImageViews->resize(swapChainImages->size());
        for (size_t i = 0; i < swapChainImages->size(); i++) {
            swapChainImageViews->at(i) = createImageView(&swapChainImages->at(i), *swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, device);
        }
    }

    VkImageView createImageView(VkImage* image, VkFormat format, VkImageAspectFlags aspectFlags, VkDevice* device) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = *image; // color/render target
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; // treat images as 1D textures, 2D textures, 3D textures and cube maps.
        viewInfo.format = format; // pixel format, color space

        // Shift color channels to prefered likings e.g. map all channels to red to get monochrome textures etc.
        viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        // subresource describes what the image's purpose is and which part of the image should be accessed.
        viewInfo.subresourceRange.aspectMask = aspectFlags; // Use color or depth information
        viewInfo.subresourceRange.baseMipLevel = MIPMAP_LEVEL; // choose mipmap level for image views
        viewInfo.subresourceRange.levelCount = 1; // no multilayered images
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1; // for stereoscopic 3D applications, use multiple layers to access views for left and right eye

        VkImageView imageView;
        if (vkCreateImageView(*device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture image view!");
        }

        return imageView;
    }

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities; // min/max number of images in swap chain, min/max width and height of images
        std::vector<VkSurfaceFormatKHR> formats; // pixel format, color space
        std::vector<VkPresentModeKHR> presentationModes; // conditions for "swapping" images to the screen. e.g. FIFO, IMMEDIATE
    };

    void createSwapChain(VkExtent2D* swapChainExtent, VkFormat* swapChainImageFormat,  std::vector<VkImage>* swapChainImages, VkSwapchainKHR* swapchain, VkSurfaceKHR* surface, VkDevice* device, VkPhysicalDevice* physicalDevice, GLFWwindow* window) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(*surface, *physicalDevice);

        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentationMode = chooseSwapPresentMode(swapChainSupport.presentationModes);
        VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities, window);

        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
        // make sure to not exceed the maximum number of images while doing this, where 0 is a special value that means that there is no maximum
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
        createInfo.imageArrayLayers = 1; // specifies the amount of layers each image consists of, always 1 for 2D Applications. Increase if using 3D stereoscopic images.
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // specifies what kind of operations we'll use the images in the swap chain for.
                                                                    // VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT specifies to render directly into images
                                                                    // VK_IMAGE_USAGE_TRANSFER_DST_BIT may be used to render into separate image and perform post processing. Perform memory operation then to transport image to swap chain.
        QueueFamilyIndices indices = findQueueFamilies(*surface, *physicalDevice);
        uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentationFamily.value()};

        if (indices.graphicsFamily != indices.presentationFamily) {
            // drawing on the images in the swap chain from the graphics queue and then submitting them on the presentation queue if they are not the same
            // --> Avoid ownership management.
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT; // Images can be used across multiple queue families without explicit ownership transfers.
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; // image is owned by one queue family at a time and ownership must be explicitly transferred before using it in another queue family. --> Best performance
            createInfo.queueFamilyIndexCount = 0; // Optional
            createInfo.pQueueFamilyIndices = nullptr; // Optional
        }

        createInfo.preTransform = swapChainSupport.capabilities.currentTransform; // change this to apply transformations to each image e.g. 90 degree rotations.
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // use to blend in with other images on window - change if opacity of alpha channel is relevant.
        createInfo.presentMode = presentationMode;
        createInfo.clipped = VK_TRUE; // enable clipping if hidden pixels are not relevant or should not be read.
        createInfo.oldSwapchain = VK_NULL_HANDLE; // leave for now - resizing windows will create new swapchains based on old ones --> later topic.

        if (vkCreateSwapchainKHR(*device, &createInfo, nullptr, swapchain) != VK_SUCCESS) {
            throw std::runtime_error("failed to create swap chain!");
        }

        // potential to put this code into separate function or method and call with struct instead of such many parameters
        vkGetSwapchainImagesKHR(*device, *swapchain, &imageCount, nullptr);
        swapChainImages->resize(imageCount);
        vkGetSwapchainImagesKHR(*device, *swapchain, &imageCount, swapChainImages->data());

        *swapChainImageFormat = surfaceFormat.format;
        *swapChainExtent = extent;
    }

    // The swap extent is the resolution of the swap chain images and it's almost always exactly equal to the resolution of the window that we're drawing to in pixels (more on that in a moment).
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window) {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        }
        else {
            // Vulkan works with pixels instead of screen coordinates.
            // But the conversion for high-DPI screens such as Apple Retina Displays does not match a 1:1 conversion --> use glfwFrameBufferSize
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
            // prefer 32 Bit srgb color cormat (8 bit per channel) and srgb color space for more accurate colors
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    bool checkSwapChainSupport(VkSurfaceKHR surface, VkPhysicalDevice device) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(surface, device);
        // For simplicity it is sufficient for now to have at least one supported image format and one supported presentation mode.
            // add sophisticated swap chain selector here.
        return !swapChainSupport.formats.empty() && !swapChainSupport.presentationModes.empty();
    }

    SwapChainSupportDetails querySwapChainSupport(VkSurfaceKHR surface, VkPhysicalDevice device) {
        SwapChainSupportDetails details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
        if (formatCount != 0) {
            details.formats.resize(formatCount); // resize vector to hold all formats
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

    // Querying for swap chain support can actually be omitted, because having a presentation queue implies the presence of a swap chain extension
    bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties>availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

        // check for all device extensions can also be performed like checking for validation layers using a nested for loop.
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
        deviceFeatures.samplerAnisotropy = ENABLE_ANISOTRIPIC_FILTER;

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

        // return true if graphics family and presentation family is set.
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

        // remove unusable devices from list
        devices.erase(std::remove_if(
            devices.begin(), devices.end(),
            [surface, this](const auto &device) {
                return !isDeviceSuitable(surface, device);
            }), devices.end());

        if (devices.empty()) {
            throw std::runtime_error("failed to find a suitable GPU!");
        }

        // list all usable GPUs on console
        size_t count = 0;
        for (const auto& device : devices) {
            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(device, &deviceProperties);
            std::cout << "(" << count << ") " << deviceProperties.deviceName << std::endl;
            count++;
        }

        std::string selected;
        std::cin >> selected;

        // validate input as a number
        size_t inputIndex = -1;
        try {
            inputIndex = std::stoi(selected); // unsafe conversion if input starts with a number and continues with letters output will be the leading number.
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

        if (ENABLE_ANISOTRIPIC_FILTER && !deviceFeatures.samplerAnisotropy) {

            return false;
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

        // check for anisotropic filtering option and availablity
        if (ENABLE_ANISOTRIPIC_FILTER && !deviceFeatures.samplerAnisotropy) {

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
        // extension functions are not loaded automatically and need to be called via vkGetInstanceProcAddr
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(instance, debugMessenger, pAllocator);
        }
    }
    
    VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
        // extension functions are not loaded automatically and need to be called via vkGetInstanceProcAddr
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

    // Callback returns boolean to indicate if a Vulkan call that triggered a validation layer message should be aborted. Should only trturn true for validation Layer testing.
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
    VulkanDrawingInitializer* drawingCreator;
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
    std::vector<VkImageView> swapchainImageViews;

    VkRenderPass renderPass;
    VkDescriptorSetLayout descriptorSetLayout;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;

    std::vector<VkFramebuffer> swapchainFramebuffers;
    VkCommandPool commandPool;
    VkCommandPool shortLivedCommandPool; // for e.g. staging to vertex buffers
    std::vector<VkCommandBuffer> commandBuffers;

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;

    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    std::vector<void*> uniformBuffersMapped;

    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    uint32_t currentFrame = 0;

    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkImageView textureImageView;
    VkSampler textureSampler;

    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;


    bool framebufferResized = false;

    void initWindow() {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // dont create opengl context
        if (LOCK_WINDOW_SIZE) glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); // disable resizable windows


        window = glfwCreateWindow(WIDTH, HEIGHT, APPLICATION_NAME, nullptr, nullptr);
        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    }

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
        auto app = reinterpret_cast<VulkanApplication*>(glfwGetWindowUserPointer(window));
        app->framebufferResized = true;
    }

    void initVulkan() {
        instanceCreator->createInstance(&instance);
        instanceCreator->setupDebugMessenger(&debugMessenger, &instance);

        presentationDeviceCreator->createSurface(&surface, window, &instance);
        presentationDeviceCreator->pickPhysicalDevice(&surface, &physicalDevice, &instance);
        presentationDeviceCreator->createLogicalDevice(&surface, &presentQueue, &graphicsQueue, &device, &physicalDevice);
        presentationDeviceCreator->createSwapChain(&swapChainExtent, &swapChainImageFormat, &swapChainImages, &swapchain, &surface, &device, &physicalDevice, window);
        presentationDeviceCreator->createImageViews(&swapchainImageViews, &swapChainImageFormat, &swapChainImages, &device);

        graphicsPipelineCreator->createRenderPass(&renderPass, &swapChainImageFormat, &device, &physicalDevice);
        drawingCreator->createDescriptorSetLayout(&descriptorSetLayout, &device);
        graphicsPipelineCreator->createGraphicsPipeline(&graphicsPipeline , &renderPass, &descriptorSetLayout, &pipelineLayout, &swapChainExtent, &device);
        
        presentationDeviceCreator->createCommandPool(&commandPool, &surface, &device, &physicalDevice);
        presentationDeviceCreator->createShortLivedCommandPool(&shortLivedCommandPool, &surface, &device, &physicalDevice);
        
        drawingCreator->createDepthResources(&depthImage, &depthImageMemory, &depthImageView, &swapChainExtent, &device, &physicalDevice);

        drawingCreator->createTextureImage(&textureImageMemory, &textureImage, &commandPool, &graphicsQueue, &device, &physicalDevice);
        drawingCreator->createTextureImageView(&textureImageView, &textureImage, &device);
        drawingCreator->createTextureSampler(&textureSampler, &device, &physicalDevice);

        drawingCreator->createFramebuffers(&depthImageView, &swapchainFramebuffers, &swapChainExtent, &swapchainImageViews, &renderPass, &device);

        drawingCreator->loadModel();
        drawingCreator->createVertexBuffer(&vertexBufferMemory, &vertexBuffer, &shortLivedCommandPool, &graphicsQueue, &device, &physicalDevice);
        drawingCreator->createIndexBuffer(&indexBufferMemory, &indexBuffer, &shortLivedCommandPool, &graphicsQueue, &device, &physicalDevice);
        drawingCreator->createUniformBuffers(&uniformBuffersMapped, &uniformBuffersMemory, &uniformBuffers, &device, &physicalDevice);
        drawingCreator->createDescriptorPool(&descriptorPool, &device);
        drawingCreator->createDescriptorSets(&textureSampler, &textureImageView, &descriptorSets, &descriptorPool, &descriptorSetLayout, &uniformBuffers, &device);
        drawingCreator->createCommandBuffers(&commandBuffers, &commandPool, &device);
        drawingCreator->createSyncObjects(&imageAvailableSemaphores, &renderFinishedSemaphores, &inFlightFences, &device);
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            drawFrame();
        }

        vkDeviceWaitIdle(device);
    }

    void recreateSwapChain() {
        int width = 0, height = 0; // framebuffer size is 0 when minimizing the window
        glfwGetFramebufferSize(window, &width, &height); // handles case where the size is already correct and glfwWaitEvents would have nothing to wait on.
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(window, &width, &height);
            glfwWaitEvents(); //simply halt application when minimized
        }

        vkDeviceWaitIdle(device); // don't touch resources that may still be in use

        cleanupSwapchain();

        presentationDeviceCreator->createSwapChain(&swapChainExtent, &swapChainImageFormat, &swapChainImages, &swapchain, &surface, &device, &physicalDevice, window);
        presentationDeviceCreator->createImageViews(&swapchainImageViews, &swapChainImageFormat, &swapChainImages, &device); // Image Views are based directly on the swap chain images
        drawingCreator->createDepthResources(&depthImage, &depthImageMemory, &depthImageView, &swapChainExtent, &device, &physicalDevice);
        //drawingCreator->createTextureImageView(&textureImageView, &textureImage, &device);
        drawingCreator->createFramebuffers(&depthImageView, &swapchainFramebuffers, &swapChainExtent, &swapchainImageViews, &renderPass, &device); // framebuffers directly depend on the swap chain images

    }

    void drawFrame() {
        // wait for previous frame to finish, so that the command buffer and semaphores are available to use
        vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

        uint32_t imageIndex; // use index to pick the framebuffer
        VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
        // check if swapchain is still adequate during presentation
        //
        // VK_ERROR_OUT_OF_DATE_KHR: The swap chain has become incompatible with the surface and can no longer be used for rendering. Usually happens after a window resize.
        // VK_SUBOPTIMAL_KHR: The swap chain can still be used to successfully present to the surface, but the surface properties are no longer matched exactly.
        if (result == VK_ERROR_OUT_OF_DATE_KHR) { // proceed anyway in case of suboptimal because an image is already acquired
            recreateSwapChain();
            return;
        }
        else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        drawingCreator->updateUniformBuffer(currentFrame, &uniformBuffersMapped, &swapChainExtent);

        // reset fence to unsignaled after wating
        // only reset fence if work is submitted
        vkResetFences(device, 1, &inFlightFences[currentFrame]);

        
        vkResetCommandBuffer(commandBuffers[currentFrame], 0);
        drawingCreator->recordCommandBuffer(currentFrame, imageIndex, &descriptorSets, &indexBuffer, &vertexBuffer, &commandBuffers[currentFrame], &commandPool, &graphicsPipeline, &renderPass, &pipelineLayout, &swapchainFramebuffers, &swapChainExtent, &device);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT }; // specify stage of graphics pipeline that writes colore attachment: wait with writing colors to the image until it's available
        VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };

        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[currentFrame];
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        // submit command buffer to graphics queue
        if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit draw command buffer!");
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapchains[] = { swapchain };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapchains;
        presentInfo.pImageIndices = &imageIndex;
        presentInfo.pResults = nullptr; // Optional, allows to specify an array of VkResult values to check for every individual swap chain if presentation was successful.
    
        result = vkQueuePresentKHR(presentQueue, &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) { //  recreate the swap chain if it is suboptimal, for best possible result.
            framebufferResized = false; // handle resize manually
            recreateSwapChain();
        }
        else if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to present swap chain image!");
        }
        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT; // use modulo to  ensure that the frame index loops around after every MAX_FRAMES_IN_FLIGHT enqueued frames.
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
        cleanupDepthResources();
        cleanupFramebuffers();
        cleanupImageViews();

        vkDestroySwapchainKHR(device, swapchain, nullptr);
    }

    void cleanupImageViews() {
        for (auto imageView : swapchainImageViews) {
            vkDestroyImageView(device, imageView, nullptr);
        }
    }

    void cleanupGraphicsPipeline() {
        vkDestroyPipeline(device, graphicsPipeline, nullptr);
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        vkDestroyRenderPass(device, renderPass, nullptr);
    }

    void cleanupFramebuffers() {
        for (auto framebuffer : swapchainFramebuffers) {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }
    }

    void cleanupCommandPools() {
        vkDestroyCommandPool(device, commandPool, nullptr);
        vkDestroyCommandPool(device, shortLivedCommandPool, nullptr);
        //no commandbuffer cleanup needed, they are freed when commandpool is deleted
    }

    void cleanupSyncObjects() {
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
            vkDestroyFence(device, inFlightFences[i], nullptr);
        }
    }

    void cleanupBuffers() {
        vkDestroyBuffer(device, vertexBuffer, nullptr);
        vkDestroyBuffer(device, indexBuffer, nullptr);
    }
    void cleanupMemory() {
        vkFreeMemory(device, vertexBufferMemory, nullptr);
        vkFreeMemory(device, indexBufferMemory, nullptr);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroyBuffer(device, uniformBuffers[i], nullptr);
            vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
        }

        vkFreeMemory(device, textureImageMemory, nullptr);
    }

    void cleanupDescriptors() {
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
        //no descriptor set cleanup needed, they are freed when descriptor pool is deleted
    }

    void cleanupImages() {
        vkDestroyImage(device, textureImage, nullptr);
    }

    void cleanupTextureResources() {
        vkDestroySampler(device, textureSampler, nullptr);
        vkDestroyImageView(device, textureImageView, nullptr);
    }

    void cleanupDepthResources() {
        vkDestroyImageView(device, depthImageView, nullptr);
        vkDestroyImage(device, depthImage, nullptr);
        vkFreeMemory(device, depthImageMemory, nullptr);
    }

    void cleanup() {
        cleanupSyncObjects();
        cleanupCommandPools();
        //cleanupFramebuffers();
        cleanupDescriptors();
        cleanupBuffers();
        cleanupMemory();
        cleanupGraphicsPipeline();
        cleanupImageViews();
        cleanupTextureResources();
        cleanupSwapchain();
        cleanupImages();
        cleanupDevices();
        cleanupSurfaces();
        cleanupDebugMessengers();
        cleanupInstances();
        cleanupGlfw();
    }
};