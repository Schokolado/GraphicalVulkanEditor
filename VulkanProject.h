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

// Creator class to initialize and setup Vulkan specific objects related to drawing, such as framebuffers and command buffers/pools
class VulkanDrawingInitializer {
    friend class VulkanApplication;
public:

private:

    /////////////////////////////////////////////////
    /*         Section for Drawing Objects         */
    /////////////////////////////////////////////////

    void createCommandBuffer(VkCommandBuffer* commandBuffer, VkCommandPool* commandPool, VkDevice* device) {
        // The level parameter specifies if the allocated command buffers are primary or secondary command buffers.
        // VK_COMMAND_BUFFER_LEVEL_PRIMARY: Can be submitted to a queue for execution, but cannot be called from other command buffers.
        // VK_COMMAND_BUFFER_LEVEL_SECONDARY : Cannot be submitted directly, but can be called from primary command buffers.
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = *commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; //use secondary to e.g. reuse common opertations from primary command buffers
        allocInfo.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(*device, &allocInfo, commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }


    }

    void recordCommandBuffer(uint32_t imageIndex, VkCommandBuffer* commandBuffer, VkCommandPool* commandPool, VkPipeline* graphicsPipeline, VkRenderPass* renderPass, std::vector<VkFramebuffer>* swapChainFramebuffers, VkExtent2D* swapChainExtent, VkDevice* device) {
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
        renderPassInfo.framebuffer = swapChainFramebuffers->at(imageIndex);
        renderPassInfo.renderArea.offset = { 0, 0 }; // render area should match the size of the attachments for best performance.
        renderPassInfo.renderArea.extent = *swapChainExtent; // The pixels outside render area will have undefined values.
        VkClearValue clearColor = CLEAR_COLOR;
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        // Subpass contents parameter controls how the drawing commands within the render pass will be provided.
        // VK_SUBPASS_CONTENTS_INLINE: The render pass commands will be embedded in the primary command buffer itself and no secondary command buffers will be executed.
        // VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS : The render pass commands will be executed from secondary command buffers.
        vkCmdBeginRenderPass(*commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(*commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *graphicsPipeline);

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

        //actual draw command
        // vertexCount
        // instanceCount : Used for instanced rendering, use 1 if you're not doing that.
        // firstVertex : Used as an offset into the vertex buffer, defines the lowest value of gl_VertexIndex.
        // firstInstance : Used as an offset for instanced rendering, defines the lowest value of gl_InstanceIndex.
        vkCmdDraw(*commandBuffer, 3, 1, 0, 0);

        vkCmdEndRenderPass(*commandBuffer);

        if (vkEndCommandBuffer(*commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }
    }


    void createFramebuffers(std::vector<VkFramebuffer>* swapChainFramebuffers, VkExtent2D* swapChainExtent, std::vector<VkImageView>* swapChainImageViews, VkRenderPass* renderPass, VkDevice* device) {
        swapChainFramebuffers->resize(swapChainImageViews->size());
        for (size_t i = 0; i < swapChainImageViews->size(); i++) {//create framebuffer for each image view
            VkImageView attachments[] = {
                swapChainImageViews->at(i)
            };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = *renderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = swapChainExtent->width;
            framebufferInfo.height = swapChainExtent->height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(*device, &framebufferInfo, nullptr, &swapChainFramebuffers->at(i)) != VK_SUCCESS) {
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
    void createRenderPass(VkRenderPass* renderPass, VkFormat* swapChainImageFormat, VkDevice* device) {
        // single color buffer attachment by one ima from swapchain
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = *swapChainImageFormat; //format should match swap chain image format
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT; //no multisampling for now

        // loading operation before rendering
        // VK_ATTACHMENT_LOAD_OP_LOAD: Preserve the existing contents of the attachment
        // VK_ATTACHMENT_LOAD_OP_CLEAR : Clear the values to a constant at the start
        // VK_ATTACHMENT_LOAD_OP_DONT_CARE : Existing contents are undefined; we don't care about them
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; //clear contents of image

        // storing operation after rendering
        // VK_ATTACHMENT_STORE_OP_STORE: Rendered contents will be stored in memory and can be read later
        // VK_ATTACHMENT_STORE_OP_DONT_CARE : Contents of the framebuffer will be undefined after the rendering operation
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; //store to show on screen 

        // stencil buffer is not in use at the moment
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        // define pixel layout
        // VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: Images used as color attachment
        // VK_IMAGE_LAYOUT_PRESENT_SRC_KHR: Images to be presented in the swap chain
        // VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL : Images to be used as destination for a memory copy operation
        // VK_IMAGE_LAYOUT_UNDEFINED : Do not care about layout of image (that e.g. previous image was in)
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // specify format before render pass begins, undefined if load op is clear
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // layout transition to when renderpass finishes

        // Render subpasses
        // subpasses reference one or more attachments
        VkAttachmentReference colorAttachmentReference{};
        colorAttachmentReference.attachment = 0; // reference to the color attachment-array index, see fragment shader layout(location = 0)
        colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; //specifies which layout we would like the attachment to have during a subpass that uses this reference

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

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;

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
        rasterizerInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizerInfo.depthBiasEnable = VK_FALSE; // bias depth by adding a constant value, e.g. for shadow maps
        rasterizerInfo.depthBiasConstantFactor = 0.0f; // Optional
        rasterizerInfo.depthBiasClamp = 0.0f; // Optional
        rasterizerInfo.depthBiasSlopeFactor = 0.0f; // Optional


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
    std::vector<VkShaderModule> setupShaderStageAndReturnModules(VkPipelineVertexInputStateCreateInfo& vertexInputInfo, VkPipelineShaderStageCreateInfo& fragmentShaderStageInfo, VkPipelineShaderStageCreateInfo& vertexShaderStageInfo, VkDevice* device) {
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
        vertexInputInfo.vertexBindingDescriptionCount = 0;
        vertexInputInfo.pVertexBindingDescriptions = nullptr; // Bindings: spacing between data and whether the data is per-vertex or per-instance
        vertexInputInfo.vertexAttributeDescriptionCount = 0;
        vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Attribute descriptions: type of the attributes passed to the vertex shader, which binding to load them from and at which offset
    
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
    void createGraphicsPipeline(VkPipeline* graphicsPipeline, VkRenderPass* renderPass, VkPipelineLayout* pipelineLayout, VkExtent2D* swapChainExtent, VkDevice* device) {
        //////////////////////// SHADER STAGE
        VkPipelineShaderStageCreateInfo vertexShaderStageInfo{};
        VkPipelineShaderStageCreateInfo fragmentShaderStageInfo{};
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};

        std::vector<VkShaderModule> shaderModules = setupShaderStageAndReturnModules(vertexInputInfo, fragmentShaderStageInfo, vertexShaderStageInfo, device);
        VkPipelineShaderStageCreateInfo shaderStages[] = { vertexShaderStageInfo, fragmentShaderStageInfo };

        //////////////////////// FIXED FUNCTION STAGE
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
        VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
        std::vector<VkDynamicState> dynamicStates;
        VkPipelineViewportStateCreateInfo viewportState{};
        VkPipelineRasterizationStateCreateInfo rasterizerInfo{};
        VkPipelineMultisampleStateCreateInfo multisamplingInfo{};
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        VkPipelineColorBlendStateCreateInfo colorBlendingInfo{};

        setupFixedFunctionStage(dynamicStates, inputAssemblyInfo, dynamicStateInfo, viewportState, rasterizerInfo, multisamplingInfo, colorBlendAttachment, colorBlendingInfo, swapChainExtent);

        //////////////////////// PIPELINE LAYOUT
        // Pipeline layout : the uniform and push values referenced by the shader that can be updated at draw time

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0; // Optional
        pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
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
        pipelineInfo.pDepthStencilState = nullptr; // Optional
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


    ///////////////////////////////////////
    /*      Section for Window Surfaces, Swap Chains and Image Views  */
    ///////////////////////////////////////


    void createImageViews(std::vector<VkImageView>* swapChainImageViews, VkFormat* swapChainImageFormat, std::vector<VkImage>* swapChainImages, VkDevice* device) {
        swapChainImageViews->resize(swapChainImages->size());
        for (size_t i = 0; i < swapChainImages->size(); i++) {
            VkImageViewCreateInfo createInfo{};
           
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = swapChainImages->at(i); // color/render target
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; // treat images as 1D textures, 2D textures, 3D textures and cube maps.
            createInfo.format = *swapChainImageFormat; // pixel format, color space
           
            // Shift color channels to prefered likings e.g. map all channels to red to get monochrome textures etc.
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            
            // subresource describes what the image's purpose is and which part of the image should be accessed.
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // Use color information
            createInfo.subresourceRange.baseMipLevel = MIPMAP_LEVEL; // choose mipmap level for image views
            createInfo.subresourceRange.levelCount = 1; // no multilayered images
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1; // for stereoscopic 3D applications, use multiple layers to access views for left and right eye

            if (vkCreateImageView(*device, &createInfo, nullptr, &swapChainImageViews->at(i)) != VK_SUCCESS) {
                throw std::runtime_error("failed to create image views!");
            }
        }
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
    std::vector<VkImageView> swapChainImageViews;

    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;

    std::vector<VkFramebuffer> swapChainFramebuffers;
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;




    void initWindow() {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // dont create opengl context
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); // disable resizable windows

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

        graphicsPipelineCreator->createRenderPass(&renderPass, &swapChainImageFormat, &device);
        graphicsPipelineCreator->createGraphicsPipeline(&graphicsPipeline , &renderPass, &pipelineLayout, &swapChainExtent, &device);
        
        drawingCreator->createFramebuffers(&swapChainFramebuffers, &swapChainExtent, &swapChainImageViews, &renderPass, &device);
        presentationDeviceCreator->createCommandPool(&commandPool, &surface, &device, &physicalDevice);
        drawingCreator->createCommandBuffer(&commandBuffer, &commandPool, &device);
        drawingCreator->recordCommandBuffer(1, &commandBuffer, &commandPool, &graphicsPipeline, &renderPass, &swapChainFramebuffers, &swapChainExtent, &device);
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

    void cleanupGraphicsPipeline() {
        vkDestroyPipeline(device, graphicsPipeline, nullptr);
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        vkDestroyRenderPass(device, renderPass, nullptr);
    }

    void cleanupFramebuffers() {
        for (auto framebuffer : swapChainFramebuffers) {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }
    }

    void cleanupCommandPools() {
        vkDestroyCommandPool(device, commandPool, nullptr);
    }

    void cleanup() {
        cleanupCommandPools();
        cleanupFramebuffers();
        cleanupGraphicsPipeline();
        cleanupImageViews();
        cleanupSwapchain();
        cleanupDevices();
        cleanupSurfaces();
        cleanupDebugMessengers();
        cleanupInstances();
        cleanupGlfw();
    }



};


