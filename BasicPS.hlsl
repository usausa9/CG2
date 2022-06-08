cbuffer ConstBufferDataMaterial : register(b0)
{
	float4 color; // F(RGBA)
}

float4 main() : SV_TARGET
{
	return float4(1.0f,1.0f,1.0f,1.0f); // R,G,B,ƒ¿
}