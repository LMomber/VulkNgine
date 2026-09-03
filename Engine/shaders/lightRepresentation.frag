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
    vec2 resolution;
    uint lightIndex;
    uint padding;
} pushConstants;

layout(set = 1, binding = 0) uniform sampler2D DepthTexture;

layout(location = 0) out vec4 FragColor;

float SphereSDF(vec3 p, float r)
{
	vec3 worldPos = lightBuffer.lights[pushConstants.lightIndex].position.xyz;
	return length(p - worldPos) - r;
}

float RayMarchSphere(vec3 ro, vec3 rd)
{
	float t = 0.0;
	float r = 0.1;

	for (int i = 0; i < 100; i++)
	{
		vec3 p = ro + rd * t;

		float d = SphereSDF(p, r);
		
		if (d < 0.001)
		{
			return t;
		}

		t += d;

		if (t > 100.0)
		{
			break;
		}
	}

	return -1.0;
}

void main()
{	
	const vec2 uv = gl_FragCoord.xy / pushConstants.resolution;

	const vec2 ndc = uv * 2.0 - 1.0;

	vec4 nearPoint = inverse(camera.projection * camera.view) * vec4(ndc, 0.0, 1.0);
	vec4 farPoint = inverse(camera.projection * camera.view) * vec4(ndc, 1.0, 1.0);

	nearPoint /= nearPoint.w;
	farPoint  /= farPoint.w;

	const vec3 rayOrigin = camera.position.xyz;
	const vec3 rayDirection = normalize(farPoint.xyz - nearPoint.xyz);

	const float t = RayMarchSphere(rayOrigin, rayDirection);

	if (t < 0.0)
	{ 
		discard;
	}

	const vec3 hitPos = rayOrigin + rayDirection * t;
	const vec4 clipPos = camera.projection * camera.view * vec4(hitPos, 1.0);
	const float hitDepth = clipPos.z / clipPos.w;

	const float depth = texture(DepthTexture, uv).r;

	if (hitDepth < depth)
	{
		FragColor = vec4(10.0, 10.0, 10.0, 1.0);
		return;
	}

	discard;
}