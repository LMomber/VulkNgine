#version 450

layout(std140, binding = 0) uniform Camera
{
    mat4 view;
    mat4 projection;
    vec4 position;
} camera;

struct Object
{
    mat4 model;
};

layout(std430, binding = 1) readonly buffer ObjectBuffer
{
	Object objects[];
} objectBuffer;

layout(push_constant) uniform Constants
{
    uint objectIndex;
} pushConstants;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec3 worldPos;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out mat3 TBN;

void main()
{
    //const mat4 model = mat4(0.0);
    const mat4 model = objectBuffer.objects[pushConstants.objectIndex].model;
	gl_Position = camera.projection * camera.view * model * vec4(inPosition, 1.0);
    worldPos = vec3(model * vec4(inPosition, 1.0));
    fragTexCoord = inTexCoord;

    vec3 tangent = normalize(model * vec4(inTangent, 0.0)).xyz;
    vec3 normal = normalize(model * vec4(inNormal, 0.0)).xyz;
    vec3 bitangent = normalize(cross(normal, tangent));
    TBN = mat3(tangent, bitangent, normal);
}