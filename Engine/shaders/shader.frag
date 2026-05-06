#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler2D albedo;
layout(set = 1, binding = 1) uniform sampler2D normal;
layout(set = 1, binding = 2) uniform sampler2D metallicRoughness;
layout(set = 1, binding = 3) uniform sampler2D AO;
layout(set = 1, binding = 4) uniform sampler2D emissive;

void main() 
{
    outColor = texture(albedo, fragTexCoord);
}