#version 450

layout(std140, binding = 0) uniform Camera
{
    mat4 view;
    mat4 projection;
    vec4 position;
} camera;

struct Light
{
	vec4 position;
	vec4 intensity; // rgb = color, w = intensity
};

layout(std430, binding = 2) readonly buffer LightBuffer
{
	uint lightCount;
	uint padding[3];
	Light lights[];
} lightBuffer;

layout(push_constant) uniform Constants
{
    uint lightIndex;
} pushConstants;

layout(location = 0) out vec3 worldPos;

void main()
{
    vec4 translation = lightBuffer.lights[pushConstants.lightIndex].position;
    worldPos = vec3(translation.xyz);

    mat4 transform = mat4(1);
    transform[3] = translation;
	gl_Position = camera.projection * camera.view * transform * vec4(1.0);
}