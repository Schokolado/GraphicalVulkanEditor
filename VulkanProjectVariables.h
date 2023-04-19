// This header includes all changeable but constant variables for the VulkanProject Header.
// Any changes should be made inside this header-file such that the original implementation can be kept untouched.


#pragma once
// Instance
const char* APPLICATION_NAME = "Vulkan Application";
const bool SHOW_VALIDATION_LAYER_DEBUG_INFO = false;
const bool RUN_ON_MACOS = false;

// Physical Device
const bool CHOOSE_GPU_ON_STARTUP = false;
const bool SAVE_ENERGY_FOR_MOBILE = false;

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
	// Rasterizer
	const VkCullModeFlagBits CULL_MODE = VK_CULL_MODE_NONE; //VK_CULL_MODE_BACK_BIT;
	const VkPolygonMode POLYGON_MODE = VK_POLYGON_MODE_FILL;
// 
const bool MIPMAP_LEVEL = 0;
const VkBool32 ENABLE_ANISOTRIPIC_FILTER = VK_TRUE;