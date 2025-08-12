float4x4 translation : register(c0);

void main(
    in float3 pos : Position, 
    in float3 color : Color,
    out float3 o_color : Color,
    out float4 o_pos : SV_POSITION
) {
    o_color = color;
    o_pos = mul(float4(pos, 1.0f), translation);
}