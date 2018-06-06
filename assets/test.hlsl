cbuffer Camera : register(b0)
{
	matrix view;
	matrix projection;
};
cbuffer Object : register(b1)
{
	matrix world;
};
cbuffer Animation : register(b2)
{
	matrix bones[200];
};
SamplerState samp : register(s0);
Texture2D diffuseTexture : register(t0);
Texture2D glossinessTexture : register(t1);
Texture2D normalTexture : register(t2);
struct Vertex
{
	float4 position : POSITION;
	float3 normal : NORMAL;
	float2 uv : TEXCOORD;
    float3 tangent : TANGENT;
	uint4 blendIndices0 : BLENDINDICES0;
	uint4 blendIndices1 : BLENDINDICES1;
	float4 blendWeights0 : BLENDWEIGHT0;
	float4 blendWeights1 : BLENDWEIGHT1;
};
struct Pixel
{
	float4 position : SV_POSITION;
	float3 normal : NORMAL;
	float2 uv : TEXCOORD;
    float3 tspace0 : TANGENT0;
    float3 tspace1 : TANGENT1;
    float3 tspace2 : TANGENT2;
};
Pixel VS(Vertex vertex)
{
	Pixel output;

	if (vertex.blendIndices0.x < 999)
	{
		matrix temp = 0;
		float weights[8];
		weights[0] = vertex.blendWeights0.x;
		weights[1] = vertex.blendWeights0.y;
		weights[2] = vertex.blendWeights0.z;
		weights[3] = vertex.blendWeights0.w;
		weights[4] = vertex.blendWeights1.x;
		weights[5] = vertex.blendWeights1.y;
		weights[6] = vertex.blendWeights1.z;
		weights[7] = vertex.blendWeights1.w;
		uint indices[8];
		indices[0] = vertex.blendIndices0.x;
		indices[1] = vertex.blendIndices0.y;
		indices[2] = vertex.blendIndices0.z;
		indices[3] = vertex.blendIndices0.w;
		indices[4] = vertex.blendIndices1.x;
		indices[5] = vertex.blendIndices1.y;
		indices[6] = vertex.blendIndices1.z;
		indices[7] = vertex.blendIndices1.w;

		for (int i = 0; i < 8; i++)
		{
			if (weights[i] <= 0.0)
				continue;

			temp += bones[indices[i]] * weights[i];
		}

		vertex.position = mul(vertex.position, temp);
		vertex.position.w = 1.0;
		vertex.normal = mul(vertex.normal, (float3x3)temp);
	}

	output.position = mul(vertex.position, world);
	output.position = mul(output.position, view);
	output.position = mul(output.position, projection);
	output.normal = mul(vertex.normal, (float3x3)world);
	output.uv = vertex.uv;

    float3 bitangent = cross(output.normal, vertex.tangent);
    output.tspace0 = float3(vertex.tangent.x, bitangent.x, output.normal.x);
    output.tspace1 = float3(vertex.tangent.y, bitangent.y, output.normal.y);
    output.tspace2 = float3(vertex.tangent.z, bitangent.z, output.normal.z);

    return output;
}
float4 PS(Pixel pixel) : SV_TARGET
{
	float3 normal;
    float3 normalMap = normalTexture.Sample(samp, pixel.uv).xyz;
    normal.x = dot(pixel.tspace0, normalMap);
    normal.y = dot(pixel.tspace1, normalMap);
    normal.z = dot(pixel.tspace2, normalMap);
    normal = normalize(normal);

	float3 lightDirection = normalize(float3(0.25, -1.0, 0.5));
	float3 lightColor = float3(1.0, 1.0, 1.0);
    float4 diffuseColor = diffuseTexture.Sample(samp, pixel.uv);

	float3 viewDirection = normalize(float3(0.0, 3.0, -5.0));
	float3 reflection = reflect(lightDirection, normal);
    float glossiness = glossinessTexture.Sample(samp, pixel.uv).r;

	float3 diffuseIntensity = dot(-lightDirection, normal) * lightColor;
	float3 ambientIntensity = lightColor * 0.2;
	float3 specularIntensity = pow(max(dot(viewDirection, reflection), 0.0), glossiness * 100.0) * 10.0 * lightColor;

	return diffuseColor * float4(diffuseIntensity + ambientIntensity + specularIntensity, 1);
}
