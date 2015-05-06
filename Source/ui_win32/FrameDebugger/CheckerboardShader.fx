float2 g_screenSize;

struct VertexInput
{
	float4	position		: POSITION;
};

struct VertexOutput
{
	float4 position			: POSITION;
	float2 texCoord			: TEXCOORD;
};

VertexOutput vertexShader(VertexInput input)
{
	VertexOutput output;
	output.position = input.position;
	output.texCoord = ((input.position.xy + 1.0f) / 2.0f) * g_screenSize / 10.f;
	return output;
}

float4 pixelShader(VertexOutput input) : COLOR
{
	if(fmod(floor(input.texCoord.x) + floor(input.texCoord.y), 2) < 1)
	{
		return float4(1, 1, 1, 1);
	}
	else
	{
		return float4(0.75f, 0.75f, 0.75f, 1);
	}
}

technique technique0 
{
	pass p0 
	{
		VertexShader = compile vs_3_0 vertexShader();
		PixelShader = compile ps_3_0 pixelShader();
	}
}
