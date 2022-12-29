#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <cstdlib>

#include "VulkanProjectVariables.h"


class HelloTriangleApplication {
public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    GLFWwindow* window;


    void initWindow() {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); //dont create opengl context
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); //disable resizable windows

        window = glfwCreateWindow(WIDTH, HEIGHT, APPLICATION_NAME, nullptr, nullptr);
    }

    void initVulkan() {

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

    void cleanup() {
        cleanupGlfw();
    }
};


