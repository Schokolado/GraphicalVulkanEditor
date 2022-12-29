// This header includes all changeable bust constant variables for the VulkanProject Header.
// Any changes should be made inside this header-file such that the original implementation can be kept untouched.


#pragma once

#include <stdexcept>

enum GPUType { integrated, dedicated, not_specified };

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
const char* APPLICATION_NAME = "Vulkano";
const GPUType GPU_TYPE = dedicated;
