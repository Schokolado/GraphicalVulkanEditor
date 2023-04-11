// This header includes all changeable bust constant variables for the VulkanProject Header.
// Any changes should be made inside this header-file such that the original implementation can be kept untouched.


#pragma once

const uint32_t WIDTH = 1200;
const uint32_t HEIGHT = 1000;
const char* APPLICATION_NAME = "Vulkan Application";
const bool RUN_ON_MACOS = false;
const bool CHOOSE_GPU_ON_STARTUP = false;
const bool SHOW_VALIDATION_LAYER_DEBUG_INFO = false;
const bool SAVE_ENERGY_FOR_MOBILE = false;
const bool MIPMAP_LEVEL = 0;
const std::string VERTEX_SHADER_FILE = "shaders/raw_shaders/shader.vert";
const std::string FRAGMENT_SHADER_FILE = "shaders/raw_shaders/shader.frag";
const bool REDUCE_SPIRV_CODE_SIZE = false;
const VkCullModeFlagBits CULL_MODE = VK_CULL_MODE_BACK_BIT;
VkClearColorValue CLEAR_COLOR = { {0.0f, 0.0f, 0.0f, 1.0f} };
const int MAX_FRAMES_IN_FLIGHT = 5;
const bool LOCK_WINDOW_SIZE = false;
const bool USE_INDEXED_VERTICES = true;
const VkBool32 ENABLE_ANISOTRIPIC_FILTER = VK_TRUE;