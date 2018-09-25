#include <D3Dcompiler.h>
#include "GSH_Direct3D9.h"
#include "nuanceur/generators/HlslShaderGenerator.h"

CGSH_Direct3D9::VertexShaderPtr CGSH_Direct3D9::CreateVertexShader(SHADERCAPS caps)
{
	HRESULT result = S_OK;
	auto shaderCode = GenerateVertexShader(caps);

	auto shaderSource = Nuanceur::CHlslShaderGenerator::Generate("main", shaderCode);

	UINT compileFlags = 0;
#ifdef _DEBUG
	compileFlags |= D3DCOMPILE_DEBUG;
#endif

	Framework::Win32::CComPtr<ID3DBlob> shaderBinary;
	Framework::Win32::CComPtr<ID3DBlob> compileErrors;
	result = D3DCompile(shaderSource.c_str(), shaderSource.length() + 1, "vs", nullptr, nullptr, "main",
	                    "vs_3_0", compileFlags, 0, &shaderBinary, &compileErrors);
	assert(SUCCEEDED(result));

	VertexShaderPtr shader;
	result = m_device->CreateVertexShader(reinterpret_cast<DWORD*>(shaderBinary->GetBufferPointer()), &shader);
	assert(SUCCEEDED(result));

	return shader;
}

CGSH_Direct3D9::PixelShaderPtr CGSH_Direct3D9::CreatePixelShader(SHADERCAPS caps)
{
	HRESULT result = S_OK;
	auto shaderCode = GeneratePixelShader(caps);

	auto shaderSource = Nuanceur::CHlslShaderGenerator::Generate("main", shaderCode,
	                                                             Nuanceur::CHlslShaderGenerator::FLAG_COMBINED_SAMPLER_TEXTURE);

	UINT compileFlags = 0;
#ifdef _DEBUG
	compileFlags |= D3DCOMPILE_DEBUG;
#endif

	Framework::Win32::CComPtr<ID3DBlob> shaderBinary;
	Framework::Win32::CComPtr<ID3DBlob> compileErrors;
	result = D3DCompile(shaderSource.c_str(), shaderSource.length() + 1, "ps", nullptr, nullptr, "main",
	                    "ps_3_0", compileFlags, 0, &shaderBinary, &compileErrors);
	assert(SUCCEEDED(result));

	PixelShaderPtr shader;
	result = m_device->CreatePixelShader(reinterpret_cast<DWORD*>(shaderBinary->GetBufferPointer()), &shader);
	assert(SUCCEEDED(result));

	return shader;
}

Nuanceur::CShaderBuilder CGSH_Direct3D9::GenerateVertexShader(SHADERCAPS caps)
{
	using namespace Nuanceur;

	auto b = CShaderBuilder();

	{
		//Inputs
		auto inputPosition = CFloat4Lvalue(b.CreateInput(SEMANTIC_POSITION));
		auto inputTexCoord = CFloat4Lvalue(b.CreateInput(SEMANTIC_TEXCOORD, 0));
		auto inputColor = CFloat4Lvalue(b.CreateInput(SEMANTIC_TEXCOORD, 1));

		//Outputs
		auto outputPosition = CFloat4Lvalue(b.CreateOutput(SEMANTIC_SYSTEM_POSITION));
		auto outputTexCoord = CFloat4Lvalue(b.CreateOutput(SEMANTIC_TEXCOORD, 0));
		auto outputColor = CFloat4Lvalue(b.CreateOutput(SEMANTIC_TEXCOORD, 1));

		//Constants
		auto projMatrix = CMatrix44Value(b.CreateUniformMatrix("g_projMatrix"));
		auto texMatrix = CMatrix44Value(b.CreateUniformMatrix("g_texMatrix"));

		outputPosition = projMatrix * NewFloat4(inputPosition->xyz(), NewFloat(b, 1.0f));
		outputTexCoord = texMatrix * NewFloat4(inputTexCoord->xyz(), NewFloat(b, 1.0f));
		outputColor = inputColor->xyzw();
	}

	return b;
}

Nuanceur::CShaderBuilder CGSH_Direct3D9::GeneratePixelShader(SHADERCAPS caps)
{
	using namespace Nuanceur;

	auto b = CShaderBuilder();

	{
		//Inputs
		auto inputTexCoord = CFloat4Lvalue(b.CreateInput(SEMANTIC_TEXCOORD, 0));
		auto inputColor = CFloat4Lvalue(b.CreateInput(SEMANTIC_TEXCOORD, 1));

		//Outputs
		auto outputColor = CFloat4Lvalue(b.CreateOutput(SEMANTIC_SYSTEM_COLOR));

		//Textures
		auto baseTexture = CTexture2DValue(b.CreateTexture2D(0));
		auto clutTexture = CTexture2DValue(b.CreateTexture2D(1));

		//Temporaries
		auto texCoord = CFloat2Lvalue(b.CreateTemporary());
		auto textureColor = CFloat4Lvalue(b.CreateTemporary());
		auto finalAlpha = CFloatLvalue(b.CreateTemporary());

		texCoord = inputTexCoord->xy() / inputTexCoord->zz();
		textureColor = NewFloat4(b, 1, 1, 1, 1);

		if(caps.texSourceMode == TEXTURE_SOURCE_MODE_STD)
		{
			textureColor = Sample(baseTexture, texCoord);
		}
		else if(caps.isIndexedTextureSource())
		{
			auto colorIndex = CFloatLvalue(b.CreateTemporary());
			colorIndex = Sample(baseTexture, texCoord)->x() * NewFloat(b, 255.f);
			if(caps.texSourceMode == TEXTURE_SOURCE_MODE_IDX4)
			{
				colorIndex = colorIndex / NewFloat(b, 15.f);
			}
			else if(caps.texSourceMode == TEXTURE_SOURCE_MODE_IDX8)
			{
				colorIndex = colorIndex / NewFloat(b, 255.f);
			}
			else
			{
				assert(false);
			}
			textureColor = Sample(clutTexture, NewFloat2(colorIndex, NewFloat(b, 0)));
		}

		if(caps.texSourceMode != TEXTURE_SOURCE_MODE_NONE)
		{
			switch(caps.texFunction)
			{
			case TEX0_FUNCTION_MODULATE:
				textureColor = Clamp(textureColor * inputColor * NewFloat4(b, 2, 2, 2, 2),
				                     NewFloat4(b, 0, 0, 0, 0), NewFloat4(b, 1, 1, 1, 1));
				break;
			case TEX0_FUNCTION_DECAL:
				break;
			default:
				assert(0);
				break;
			}
		}
		else
		{
			textureColor = inputColor->xyzw();
		}

		//For proper alpha blending, alpha has to be multiplied by 2 (0x80 -> 1.0)
		//This has the side effect of not writing a proper value in the framebuffer (should write alpha "as is")
		finalAlpha = Saturate(textureColor->w() * NewFloat(b, 2));
		outputColor = NewFloat4(textureColor->xyz(), finalAlpha);
	}

	return b;
}
