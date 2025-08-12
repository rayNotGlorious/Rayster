void main(
	in float3 color : Color,
	out float4 o_color : SV_TARGET
) {
    o_color = float4(color, 1.0f);
}