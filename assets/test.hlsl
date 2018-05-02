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
	matrix bone[100];
};
//Texture2D texture0 : register(t0);
//SamplerState sampler0 : register(s0);
struct Vertex
{
    float4 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
	uint4 blendIndices : BLENDINDICES;
	float4 blendWeights : BLENDWEIGHT;
};
struct Pixel
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};
Pixel VS(Vertex vertex)
{
    Pixel output;
	float4 position = vertex.position;
	if(vertex.blendIndices.x < 99999)
	{
		matrix m = 0;
		float weights[4];
		weights[0] = vertex.blendWeights.x;
		weights[1] = vertex.blendWeights.y;
		weights[2] = vertex.blendWeights.z;
		weights[3] = vertex.blendWeights.w;
		uint indices[4];
		indices[0] = vertex.blendIndices.x;
		indices[1] = vertex.blendIndices.y;
		indices[2] = vertex.blendIndices.z;
		indices[3] = vertex.blendIndices.w;

		for (int i = 0; i < 4; i++)
		{
			if (weights[i] > 0.0f)
			{
				m += bone[indices[i]] * weights[i];
			}
		}
		position = mul(vertex.position, m);
		position.w = 1.0;
	}
	output.position = mul(position, world);
    output.position = mul(output.position, view);
    output.position = mul(output.position, projection);
    output.normal = mul(vertex.normal, (float3x3)world);
    output.uv = vertex.uv;
    return output;
}
float4 PS(Pixel pixel) : SV_TARGET
{
    float3 normal = normalize(pixel.normal);
    float3 lightDirection = normalize(float3(0.25, -1.0, 0.5));
    float3 lightColor = float3(1.0, 1.0, 1.0);
    //float4 diffuseColor = texture0.Sample(sampler0, pixel.uv);

	float3 viewDirection = normalize(float3(0.0, 3.0, -5.0));
	float3 reflection = reflect(lightDirection, normal);

    float3 diffuseIntensity = dot(-lightDirection, normal) * lightColor;
    float3 ambientIntensity = lightColor * 0.2;
	float3 specularIntensity = pow(max(dot(viewDirection, reflection), 0.0), 50.0) * 10.0 * lightColor;

	return float4(diffuseIntensity + ambientIntensity, 1);
	//return float4(diffuseIntensity + ambientIntensity + specularIntensity, 1);
	//return diffuseColor * float4(diffuseIntensity + ambientIntensity + specularIntensity, 1);
}
