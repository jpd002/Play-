float2	g_screenSize;
float2	g_bufferSize;
float2	g_panOffset;
float	g_zoomFactor;

struct VertexInput
{
	float4	position		: POSITION;
	float2	texCoord		: TEXCOORD;
};

struct VertexOutput
{
	float4 position			: POSITION;
	float2 texCoord			: TEXCOORD;
};

VertexOutput main(VertexInput input)
{
	VertexOutput output;
	output.position = ((input.position * float4(g_bufferSize / g_screenSize, 0, 1)) + float4(g_panOffset, 0, 0)) * float4(g_zoomFactor, g_zoomFactor, 0, 1);
	output.texCoord = input.texCoord;
	return output;
}


vec2	g_screenSize;
vec2	g_bufferSize;
vec2	g_panOffset;
float	g_zoomFactor;
uniform vec2 g_texCoord;
out vec4 a_position;
out vec2 a_texCoord;

void main()
{
	gl_Position = ((a_position * vec4(g_bufferSize / g_screenSize, 0, 1)) + vec4(g_panOffset, 0, 0)) * vec4(g_zoomFactor, g_zoomFactor, 0, 1);
	a_texCoord = g_texCoord;
}
