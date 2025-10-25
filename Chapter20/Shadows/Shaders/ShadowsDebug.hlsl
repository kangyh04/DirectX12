#include "Common.hlsl"

struct VertexIn
{
    float3 PosL : POSITION;
    float2 TexC : TEXCOORD;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float2 TexC : TEXCOORD0;
};

VertexOut VS(VertexIn vin, uint instanceID : SV_InstanceID)
{
    VertexOut vout = (VertexOut) 0.0f;

    vout.PosH = float4(vin.PosL, 1.0f);
    
    vout.TexC = vin.TexC;

    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    return float4(gShadowMap.Sample(gsamLinearWrap, pin.TexC).rrr, 1.0f);

}