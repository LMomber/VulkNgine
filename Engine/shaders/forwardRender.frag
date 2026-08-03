#version 450

const float PI = 3.14159265358979323846;
const float oneOverPi = 1.0 / PI;

layout(location = 0) in vec3 worldPos;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in mat3 TBN;

layout(location = 0) out vec4 FragColor;

layout(set = 1, binding = 0) uniform sampler2D Albedo;
layout(set = 1, binding = 1) uniform sampler2D Normal;
layout(set = 1, binding = 2) uniform sampler2D MetallicRoughness;
layout(set = 1, binding = 3) uniform sampler2D AO;
layout(set = 1, binding = 4) uniform sampler2D Emissive;

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

vec3 UnpackNormal(vec3 packedNormal)
{
	vec3 unpacked = packedNormal * 2.0 - 1.0;
	return normalize(vec3(TBN * unpacked));
}

vec3 Fresnel(const vec3 F0, const vec3 h, const vec3 v)
{
	const float cosTheta = max(dot(h, v), 0.0);
	const float x = clamp(1.0 - cosTheta, 0.0, 1.0);
	const float x5 = x * x * x * x * x;
	return F0 + (1.0 - F0) * x5;
}

float NormalDistributionFunction(const float r, const vec3 n, const vec3 h)
{
	const float a = r * r;
	const float a2 = a*a;
	const float cosTheta = max(dot(n, h), 0.0);
	const float cosTheta2 = cosTheta * cosTheta;
	const float intm = cosTheta2 * (a2 - 1.0) + 1.0;
	const float denominator = PI * (intm * intm);

	return a2 / denominator;
}

float SubGeometryFunction(const float r, const vec3 n, const vec3 v)
{
	const float intm = 1.0 + r;
	const float k = (intm * intm) / 8.0; // Only for direct lighting!!!

	const float cosTheta = max(dot(n, v), 0.0);
	const float denominator = cosTheta * (1.0 - k) + k;

	return cosTheta / denominator;
}

float GeometryFunction(const float r, const vec3 n, const vec3 v, const vec3 l)
{
	// Geometry obstruction
	const float obstruction = SubGeometryFunction(r, n, l);

	// Geometry shadowing
	const float shadowing = SubGeometryFunction(r, n, v);

	return  obstruction * shadowing;
}

void main()
{
	const vec3 albedo = texture(Albedo, fragTexCoord).rgb;
	const vec3 normal = UnpackNormal(texture(Normal, fragTexCoord).rgb);
	const vec3 metallicRoughness = texture(MetallicRoughness, fragTexCoord).rgb;
	const float metallic = metallicRoughness.b;
	const float roughness = metallicRoughness.g;
	const float ao = texture(AO, fragTexCoord).r;
	const vec3 emissive = texture(Emissive, fragTexCoord).rgb;

	vec3 F0 = vec3(0.04);
	F0 = mix(F0, vec3(albedo), metallic);

	const vec3 n = normalize(normal);
	const vec3 v = normalize(camera.position.xyz - worldPos); // If it doesn't work, invert OutDir
	const vec3 p = worldPos;
	const float r = max(roughness, 0.04);

	const vec3 diffuse = albedo * oneOverPi /*+ (vec3(0.03) * albedo)*/;

	vec3 Lo = vec3(0.0);

	for (int i = 0; i < lightBuffer.lightCount; i++)
	{
		const vec3 lightPos = lightBuffer.lights[i].position.xyz;
		const vec3 flux = lightBuffer.lights[i].intensity.xyz * lightBuffer.lights[i].intensity.w;

		const vec3 L = normalize(lightPos - p);
		const vec3 h = normalize(v + L);

		const float distance = length(lightPos - p);
		const float attenuation = 1.0 / (distance * distance);
		const vec3 radiance = flux * attenuation;

		const float D = NormalDistributionFunction(r, n, h);
		const vec3 F = Fresnel(F0, h, v);
		const float G = GeometryFunction(r, n, v, L);
		const vec3 nom = D * F * G;
		const float denom = 4 * max(dot(n, v), 0.0) * max(dot(n, L), 0.0) + 0.0001;

		const vec3 reflectance = nom / denom;
		vec3 kD = vec3(1.0) - F;
		kD *= 1 - metallic;
		const vec3 refractance = kD * diffuse;

		const float NdotL = max(dot(n, L), 0.0);
		//Lo = vec3(D);
		Lo += (refractance + reflectance) * radiance * NdotL;
	}

	//Lo += vec3(0.03) * albedo * ao;

//	// Tone mapping 
//	Lo = Lo / (Lo + vec3(1.0));
//	// Gamma correction
//	Lo = pow(Lo, vec3(1.0/2.2));
//
	Lo += emissive;


	FragColor = vec4(Lo, 1.0);
}