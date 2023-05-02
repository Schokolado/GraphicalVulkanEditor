// This header includes all changeable but constant variables for the VulkanProject Header.
// Any changes should be made inside this header-file such that the original implementation can be kept untouched.
// DO NOT TOUCH THIS FILE. Any changes will be overridden on next save of Vulkan Setup GUI.


#pragma once
// Instance
const char* APPLICATION_NAME = "Vulkan Application";
const bool SHOW_VALIDATION_LAYER_DEBUG_INFO = true;
const bool RUN_ON_MACOS = false;

// Physical Device
const bool CHOOSE_GPU_ON_STARTUP = false;

// Device

const std::vector<const char*> DEVICE_EXTENSIONS {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

// Swapchain
const uint32_t WIDTH = 1200;
const uint32_t HEIGHT = 1000;
VkClearColorValue CLEAR_COLOR = { {0.0f, 0.0f, 0.0f, 1.0f} };
const int MAX_FRAMES_IN_FLIGHT = 5;
const bool LOCK_WINDOW_SIZE = false;
const VkImageUsageFlagBits IMAGE_USAGE = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
const VkPresentModeKHR PRESENTATION_MODE = VK_PRESENT_MODE_MAILBOX_KHR;
const bool SAVE_ENERGY_FOR_MOBILE = false;
const VkFormat IMAGE_FORMAT = VK_FORMAT_B8G8R8A8_SRGB;
const VkColorSpaceKHR IMAGE_COLOR_SPACE = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
// Descriptor

// Shader
const std::string VERTEX_SHADER_FILE = "shaders/raw_shaders/shader.vert";
const std::string FRAGMENT_SHADER_FILE = "shaders/raw_shaders/shader.frag";
const bool USE_INDEXED_VERTICES = true;
const bool REDUCE_SPIRV_CODE_SIZE = false;
const char* SHADER_ENTRY_FUNCTION_NAME = "main";
const VkPrimitiveTopology VERTEX_TOPOLOGY = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

// Model
const std::string MODEL_FILE = "models/viking_room.obj";
const std::string TEXTURE_FILE = "textures/viking_room.png";

// Graphics Pipeline
	// Pipeline
	const int PIPELINE_COUNT = 2;
	// Rasterizer
	const VkCullModeFlagBits CULL_MODE = VK_CULL_MODE_NONE; //VK_CULL_MODE_BACK_BIT;
	const VkPolygonMode POLYGON_MODE = VK_POLYGON_MODE_FILL;

// 
const bool MIPMAP_LEVEL = 0;
const VkBool32 ENABLE_ANISOTRIPIC_FILTER = VK_TRUE;


struct FixedFunctionStageParameters {

    //////////////////////// INPUT ASSEMBLY
	const VkPrimitiveTopology inputAssemblyInfo_topology;
	const VkBool32 inputAssemblyInfo_primitiveRestartEnable;

    //////////////////////// RASTERIZER
	const VkBool32 rasterizerInfo_depthClampEnable; // clamp instead of discard fragments to far or near plane if are beyond, useful for e.g. shadow maps
	const VkBool32 rasterizerInfo_rasterizerDiscardEnable; // discard geometry passing through rasterizer, disables output to framebuffer.
	const VkPolygonMode rasterizerInfo_polygonMode;
	const float rasterizerInfo_lineWidth; // thickness of lines in terms of number of fragments
	const VkCullModeFlagBits rasterizerInfo_cullMode; //specify cull mode such as front-, back- or front-and-back culling
	const VkFrontFace rasterizerInfo_frontFace; // use counter clockwise to correct reversed draw oder caused by y-flip
	const VkBool32 rasterizerInfo_depthBiasEnable; // bias depth by adding a constant value, e.g. for shadow maps
	const float rasterizerInfo_depthBiasConstantFactor; // Optional
	const float rasterizerInfo_depthBiasClamp; // Optional
	const float rasterizerInfo_depthBiasSlopeFactor; // Optional

    //////////////////////// DEPTH AND STENCIL
	const VkBool32 depthStencilInfo_depthTestEnable; // specifies if the depth of new fragments should be compared to the depth buffer to see if they should be discarded
	const VkBool32 depthStencilInfo_depthWriteEnable; // specifies if the new depth of fragments that pass the depth test should actually be written to the depth buffer
	const VkCompareOp depthStencilInfo_depthCompareOp; // specifies the comparison that is performed to keep or discard fragments. Use convention of lower depth = closer
	const VkBool32 depthStencilInfo_depthBoundsTestEnable;
	const float depthStencilInfo_minDepthBounds; // Optional, used for the optional depth bound test. Basically, this allows to only keep fragments that fall within the specified depth range. 
	const float depthStencilInfo_maxDepthBounds; // Optional, used for the optional depth bound test. Basically, this allows to only keep fragments that fall within the specified depth range. 
	const VkBool32 depthStencilInfo_stencilTestEnable;

    //////////////////////// MULTISAMPLING
    const VkBool32 multisamplingInfo_sampleShadingEnable;
    const VkSampleCountFlagBits multisamplingInfo_rasterizationSamples;
    const float multisamplingInfo_minSampleShading; // Optional
    const VkBool32 multisamplingInfo_alphaToCoverageEnable; // Optional
    const VkBool32 multisamplingInfo_alphaToOneEnable; // Optional

    //////////////////////// COLOR BLENDING
    const VkColorComponentFlags colorBlendAttachment_colorWriteMask;
    const VkBool32 colorBlendAttachment_blendEnable;
    const VkBlendFactor colorBlendAttachment_srcColorBlendFactor; // Optional
    const VkBlendFactor colorBlendAttachment_dstColorBlendFactor; // Optional
    const VkBlendOp colorBlendAttachment_colorBlendOp; // Optional
    const VkBlendFactor colorBlendAttachment_srcAlphaBlendFactor; // Optional
    const VkBlendFactor colorBlendAttachment_dstAlphaBlendFactor; // Optional
    const VkBlendOp colorBlendAttachment_alphaBlendOp; // Optional

    const VkBool32 colorBlendingInfo_logicOpEnable; //false applies to ALL attached framebuffers. Set to true if using e.g. alpha blending
    const VkLogicOp colorBlendingInfo_logicOp; // Optional
    const uint32_t colorBlendingInfo_attachmentCount;
    const float colorBlendingInfo_blendConstants_0; // Optional
    const float colorBlendingInfo_blendConstants_1; // Optional
    const float colorBlendingInfo_blendConstants_2; // Optional
    const float colorBlendingInfo_blendConstants_3; // Optional
};
FixedFunctionStageParameters pipelineParameters_1{
	//////////////////////// INPUT ASSEMBLY
	VERTEX_TOPOLOGY, // inputAssemblyInfo_topology
	VK_FALSE, // inputAssemblyInfo_primitiveRestartEnable

	//////////////////////// RASTERIZER
	VK_FALSE, // rasterizerInfo_depthClampEnable
	VK_FALSE, // rasterizerInfo_rasterizerDiscardEnable
	POLYGON_MODE, // rasterizerInfo_polygonMode 
	1.0f, // rasterizerInfo_lineWidth
	CULL_MODE, //CULL_MODE, // rasterizerInfo_cullMode
	VK_FRONT_FACE_COUNTER_CLOCKWISE, // rasterizerInfo_frontFace
	VK_FALSE, // rasterizerInfodepthBiasEnable
	0.0f, // rasterizerInfo_depthBiasConstantFactor
	0.0f, // rasterizerInfo_depthBiasClamp
	0.0f, // rasterizerInfo_depthBiasSlopeFactor

	//////////////////////// DEPTH AND STENCIL
	VK_TRUE, // depthStencilInfo_depthTestEnable
	VK_TRUE, // depthStencilInfo_depthWriteEnable
	VK_COMPARE_OP_LESS, // depthStencilInfo_depthCompareOp
	VK_FALSE, // depthStencilInfo_depthBoundsTestEnable
	0.0f, // depthStencilInfo_minDepthBounds
	1.0f, // depthStencilInfo_maxDepthBounds
	VK_FALSE, // depthStencilInfo_stencilTestEnable

	//////////////////////// MULTISAMPLING
	VK_FALSE, // multisamplingInfo_sampleShadingEnable
	VK_SAMPLE_COUNT_1_BIT, // multisamplingInfo_rasterizationSamples
	1.0f, // multisamplingInfo_minSampleShading
	VK_FALSE, // multisamplingInfo_alphaToCoverageEnable
	VK_FALSE, // multisamplingInfo_alphaToOneEnable

	//////////////////////// COLOR BLENDING
	VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, // colorBlendAttachment_colorWriteMask
	VK_FALSE, // colorBlendAttachment_blendEnable
	VK_BLEND_FACTOR_ONE, // colorBlendAttachment_srcColorBlendFactor
	VK_BLEND_FACTOR_ZERO, // colorBlendAttachment_dstColorBlendFactor
	VK_BLEND_OP_ADD, // colorBlendAttachment_colorBlendOp
	VK_BLEND_FACTOR_ONE, // colorBlendAttachment_srcAlphaBlendFactor
	VK_BLEND_FACTOR_ZERO, // colorBlendAttachment_dstAlphaBlendFactor
	VK_BLEND_OP_ADD, // colorBlendAttachment_alphaBlendOp

	VK_FALSE, // colorBlendingInfo_logicOpEnable
	VK_LOGIC_OP_COPY, // colorBlendingInfo_logicOp
	1, // colorBlendingInfo_attachmentCount
	0.0f, // colorBlendingInfo_blendConstants_0
	0.0f, // colorBlendingInfo_blendConstants_1
	0.0f, // colorBlendingInfo_blendConstants_2
	0.0f // colorBlendingInfo_blendConstants_3
};

FixedFunctionStageParameters pipelineParameters_2{
	//////////////////////// INPUT ASSEMBLY
	VERTEX_TOPOLOGY, // inputAssemblyInfo_topology
	VK_FALSE, // inputAssemblyInfo_primitiveRestartEnable

	//////////////////////// RASTERIZER
	VK_FALSE, // rasterizerInfo_depthClampEnable
	VK_FALSE, // rasterizerInfo_rasterizerDiscardEnable
	POLYGON_MODE, // rasterizerInfo_polygonMode 
	1.0f, // rasterizerInfo_lineWidth
	CULL_MODE, //CULL_MODE, // rasterizerInfo_cullMode
	VK_FRONT_FACE_COUNTER_CLOCKWISE, // rasterizerInfo_frontFace
	VK_FALSE, // rasterizerInfodepthBiasEnable
	0.0f, // rasterizerInfo_depthBiasConstantFactor
	0.0f, // rasterizerInfo_depthBiasClamp
	0.0f, // rasterizerInfo_depthBiasSlopeFactor

	//////////////////////// DEPTH AND STENCIL
	VK_TRUE, // depthStencilInfo_depthTestEnable
	VK_TRUE, // depthStencilInfo_depthWriteEnable
	VK_COMPARE_OP_LESS, // depthStencilInfo_depthCompareOp
	VK_FALSE, // depthStencilInfo_depthBoundsTestEnable
	0.0f, // depthStencilInfo_minDepthBounds
	1.0f, // depthStencilInfo_maxDepthBounds
	VK_FALSE, // depthStencilInfo_stencilTestEnable

	//////////////////////// MULTISAMPLING
	VK_FALSE, // multisamplingInfo_sampleShadingEnable
	VK_SAMPLE_COUNT_1_BIT, // multisamplingInfo_rasterizationSamples
	1.0f, // multisamplingInfo_minSampleShading
	VK_FALSE, // multisamplingInfo_alphaToCoverageEnable
	VK_FALSE, // multisamplingInfo_alphaToOneEnable

	//////////////////////// COLOR BLENDING
	VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, // colorBlendAttachment_colorWriteMask
	VK_FALSE, // colorBlendAttachment_blendEnable
	VK_BLEND_FACTOR_ONE, // colorBlendAttachment_srcColorBlendFactor
	VK_BLEND_FACTOR_ZERO, // colorBlendAttachment_dstColorBlendFactor
	VK_BLEND_OP_ADD, // colorBlendAttachment_colorBlendOp
	VK_BLEND_FACTOR_ONE, // colorBlendAttachment_srcAlphaBlendFactor
	VK_BLEND_FACTOR_ZERO, // colorBlendAttachment_dstAlphaBlendFactor
	VK_BLEND_OP_ADD, // colorBlendAttachment_alphaBlendOp

	VK_FALSE, // colorBlendingInfo_logicOpEnable
	VK_LOGIC_OP_COPY, // colorBlendingInfo_logicOp
	1, // colorBlendingInfo_attachmentCount
	0.0f, // colorBlendingInfo_blendConstants_0
	0.0f, // colorBlendingInfo_blendConstants_1
	0.0f, // colorBlendingInfo_blendConstants_2
	0.0f // colorBlendingInfo_blendConstants_3
};
