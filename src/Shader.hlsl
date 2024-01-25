cbuffer view_info : register(b0)
{
    matrix view;
    float2 offset;
    float2 scale;
}

float4 vs_main(float2 pos : POSITION) : SV_POSITION
{
    pos *= scale;
    pos += offset;    
    return mul(float4(pos, 0.0f, 1.0f), view);
}

float4 ps_main(float4 pos : SV_POSITION) : SV_TARGET
{
    return float4(1.0f, 1.0f, 1.0f, 1.0f);
}