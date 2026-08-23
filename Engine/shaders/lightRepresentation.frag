#version 450

layout(std140, binding = 0) uniform Camera
{
    mat4 view;
    mat4 projection;
    vec4 position;
} camera;

layout(location = 0) in vec3 worldPos;

layout(location = 0) out vec4 FragColor;

float SphereSDF(vec3 p, float r)
{
	return length(p) - r;
}

float RayMarchSphere(vec3 ro, vec3 rd)
{
	float t = 0.0;
	float r = 0.3;

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
	vec3 dir = vec3(0.0) - camera.position.xyz;

	float t = RayMarchSphere(camera.position.xyz, dir);

	if (t < 0.0)
	{
		discard;
	}

	FragColor = vec4(1.0);
}