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

VertexOutput main(VertexInput input)
{
	VertexOutput output;
	output.position = input.position;
	output.texCoord = ((input.position.xy + 1.0f) / 2.0f) * g_screenSize / 10.f;
	return output;
}

float2 g_screenSize;
layout (location = 0) in vec4 a_position;
out vec2 a_texCoord;

void main()
{
	gl_Position =  a_position;
	a_texCoord = ((a_position.xy + 1.0f) / 2.0f) * g_screenSize / 10.f;
}
