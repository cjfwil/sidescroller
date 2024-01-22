float4 vs_main(float2 pos : POSITION) : SV_POSITION
{
    return float4(pos, 0.0f, 1.0f);
}

float4 ps_main(float4 pos : SV_POSITION) : SV_TARGET
{
    return float4(1.0f, 1.0f, 1.0f, 1.0f);
}