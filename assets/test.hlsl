cbuffer Object : register(b0) {
    matrix _world;
    float3 _lightDirection;
};
cbuffer Camera : register(b1) {
    matrix _view;
    matrix _projection;
};
struct VSOutput {
    float4 position : SV_POSITION;
    float4 normal : NORMAL;
    float2 uv : TEXCOORD;
};
VSOutput VS(float3 vertex : POSITION, float3 normal : NORMAL, float2 uv : TEXCOORD) {
    VSOutput output = (VSOutput)0;
    output.position = mul(_world, float4(vertex, 1.0));
    output.position = mul(_view, output.position);
    output.position = mul(_projection, output.position);
    output.normal = normalize(mul(_world, float4(normal, 1)));
    output.uv = uv;
    return output;
}
float4 PS(VSOutput pixel) : SV_TARGET {
    float diffuse = dot(-_lightDirection, pixel.normal.xyz);
    return max(0, float4(float3(1, 1, 1) * diffuse, 1));
}
