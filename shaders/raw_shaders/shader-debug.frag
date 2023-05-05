#version 450

layout(binding = 1) uniform sampler2D texSampler; //access images through sampler unioform aka combined image sampler

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() { 
    outColor = vec4(fragTexCoord, 0.0, 1.0); //print texture coords for debugging
}