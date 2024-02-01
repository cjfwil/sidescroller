cbuffer view_info : register(b0)
{
    matrix view;
    float4 colour;
    float2 offset;
    float2 scale;
    float rot;    
}

float4 vs_main(float2 pos: POSITION) : SV_POSITION
{
    pos *= scale;

    float2x2 rotMatrix = float2x2(cos(rot), -sin(rot),
                                  sin(rot), cos(rot));
    pos = mul(pos, rotMatrix);
    pos += offset;
    return mul(float4(pos, 0.0f, 1.0f), view);
}

float4 ps_main(float4 pos: SV_POSITION) : SV_TARGET
{
    return colour;
}
