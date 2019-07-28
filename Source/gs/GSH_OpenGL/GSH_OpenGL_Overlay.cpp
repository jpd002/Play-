#include "GSH_OpenGL.h"
#include <assert.h>
#include <sstream>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_STROKER_H

#ifdef GLES_COMPATIBILITY
#define GLSL_VERSION "#version 300 es"
#else
#define GLSL_VERSION "#version 150"
#endif

void CGSH_OpenGL::InitOverlay()
{
	FT_Face face;
	FT_Library library;
	if(FT_Init_FreeType(&library) == 0)
	{
		// int res = FT_New_Face(library, "C:\\Users\\Muna\\Desktop\\Play-Build\\Anonymous.ttf", 0, &face);
		int res = FT_New_Face(library, "/Users/alawi/Desktop/Play-Build/Play/build/UbuntuMono-B.ttf", 0, &face);
		assert(res == 0);
	}

	m_characterTextureMap.clear();
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	FT_Set_Pixel_Sizes(face, 0, 184);
	FT_GlyphSlot slot = face->glyph;

	for (int c = 0; c < 128; c++)
	{
		FT_Load_Char(face, c, FT_LOAD_RENDER);

		GLuint texture;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);

		//Note the bitmap only holds alpha value, so we'll store it in in red bits
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, slot->bitmap.width, slot->bitmap.rows, 0, GL_RED, GL_UNSIGNED_BYTE, slot->bitmap.buffer);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		CharTex character =
		{
			texture,
			{slot->bitmap.width / 2048.0f, slot->bitmap.rows / 2048.0f},
			slot->bitmap_left / 2048.0f,
			slot->advance.x / (2048.0f * 64.0f)
		};
		m_characterTextureMap.insert(std::pair<char, CharTex>(static_cast<char>(c), character));
	}

	FT_Done_Face(face);
	FT_Done_FreeType(library);

}

void CGSH_OpenGL::PrintOverlayText(std::string text)
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glUseProgram(*m_overlayProgram);

	glActiveTexture(GL_TEXTURE3);
	glUniform1i(m_overlayTextureUniform, 3);
	assert(m_overlayTextureUniform != -1);

	glUniform3f(m_overlayTextColorUniform, 0.0, 0.25, 0.75);
	assert(m_overlayTextColorUniform != -1);

	glBindVertexArray(m_overlayVertexArray);
	glBindBuffer(GL_ARRAY_BUFFER, m_overlayVertexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_overlayVertexElementsBuffer);

	CHECKGLERROR();

	float y = m_characterTextureMap['y'].advancePosition * 2.0f, x = 0;
	for (auto c = text.begin(); c != text.end(); ++c)
	{
		CharTex ch = m_characterTextureMap[*c];
		if(*c == '\n')
		{
			y += ch.advancePosition * 2.0f;
			x = 0;
			continue;
		}

		GLfloat positionX = -1.0f +  x + ch.offsetX;
		GLfloat positionY = 1.0f - y;

		GLfloat width = ch.size.width;
		GLfloat height = ch.size.height;

		GLfloat vertices[4][4] =
		{
			{ positionX + width, positionY,          1.0, 1.0 }, // Right-Top
			{ positionX,         positionY,          0.0, 1.0 }, // Left-Top
			{ positionX,         positionY + height, 0.0, 0.0 }, // Left-Bottom
			{ positionX + width, positionY + height, 1.0, 0.0 }, // Right-Bottom
		};

		glBindTexture(GL_TEXTURE_2D, ch.texture);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); 
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		x += ch.advancePosition;
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);

}

Framework::OpenGl::CBuffer CGSH_OpenGL::GenerateOverlayVertexElementsBuffer()
{
	auto buffer = Framework::OpenGl::CBuffer::Create();

	GLuint elements[] =
	{
		0, 1, 2,
		2, 0, 3
	};

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	return buffer;

}

Framework::OpenGl::CBuffer CGSH_OpenGL::GenerateOverlayVertexBuffer()
{
	auto buffer = Framework::OpenGl::CBuffer::Create();

	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 4 * 4, nullptr, GL_DYNAMIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	return buffer;
}

Framework::OpenGl::CVertexArray CGSH_OpenGL::GenerateOverlayVertexArray()
{
	auto vertexArray = Framework::OpenGl::CVertexArray::Create();

	glBindVertexArray(vertexArray);

	glBindBuffer(GL_ARRAY_BUFFER, m_overlayVertexBuffer);

	glEnableVertexAttribArray(static_cast<GLuint>(PRIM_VERTEX_ATTRIB::POSITION));
	glVertexAttribPointer(static_cast<GLuint>(PRIM_VERTEX_ATTRIB::POSITION), 2, GL_FLOAT,
	                      GL_FALSE, sizeof(float) * 4, reinterpret_cast<const GLvoid*>(0));

	glEnableVertexAttribArray(static_cast<GLuint>(PRIM_VERTEX_ATTRIB::TEXCOORD));
	glVertexAttribPointer(static_cast<GLuint>(PRIM_VERTEX_ATTRIB::TEXCOORD), 2, GL_FLOAT,
	                      GL_FALSE, sizeof(float) * 4, reinterpret_cast<const GLvoid*>(8));

	glBindVertexArray(0);

	return vertexArray;
}

void CGSH_OpenGL::SetupOverlay()
{
	InitOverlay();
	m_overlayProgram = GenerateOverlayProgram();

	m_overlayVertexBuffer = GenerateOverlayVertexBuffer();
	m_overlayVertexElementsBuffer = GenerateOverlayVertexElementsBuffer();
	m_overlayVertexArray = GenerateOverlayVertexArray();

	m_overlayTextureUniform = glGetUniformLocation(*m_overlayProgram, "g_texture");
	m_overlayTextColorUniform = glGetUniformLocation(*m_overlayProgram, "g_textcolor");
}

Framework::OpenGl::ProgramPtr CGSH_OpenGL::GenerateOverlayProgram()
{
	Framework::OpenGl::CShader vertexShader(GL_VERTEX_SHADER);
	Framework::OpenGl::CShader pixelShader(GL_FRAGMENT_SHADER);

	{
		std::stringstream shaderBuilder;
		shaderBuilder << GLSL_VERSION << std::endl;
		shaderBuilder << "in vec2 a_position;" << std::endl;
		shaderBuilder << "in vec2 a_texCoord;" << std::endl;
		shaderBuilder << "out vec2 v_texCoord;" << std::endl;
		shaderBuilder << "void main()" << std::endl;
		shaderBuilder << "{" << std::endl;
		shaderBuilder << "	v_texCoord = a_texCoord;" << std::endl;
		shaderBuilder << "	gl_Position = vec4(a_position, 0, 1);" << std::endl;
		shaderBuilder << "}" << std::endl;

		vertexShader.SetSource(shaderBuilder.str().c_str());
		FRAMEWORK_MAYBE_UNUSED bool result = vertexShader.Compile();
		assert(result);
	}

	{
		std::stringstream shaderBuilder;
		shaderBuilder << GLSL_VERSION << std::endl;
		shaderBuilder << "precision mediump float;" << std::endl;
		shaderBuilder << "in vec2 v_texCoord;" << std::endl;
		shaderBuilder << "out vec4 fragColor;" << std::endl;
		shaderBuilder << "uniform sampler2D g_texture;" << std::endl;
		shaderBuilder << "uniform vec3 g_textcolor;" << std::endl;
		shaderBuilder << "void main()" << std::endl;
		shaderBuilder << "{" << std::endl;
		shaderBuilder << "	float alpha = texture(g_texture, v_texCoord).r;" << std::endl;
		shaderBuilder << "	fragColor = vec4(g_textcolor, alpha);" << std::endl;
		shaderBuilder << "}" << std::endl;

		pixelShader.SetSource(shaderBuilder.str().c_str());
		FRAMEWORK_MAYBE_UNUSED bool result = pixelShader.Compile();
		assert(result);

	}

	auto program = std::make_shared<Framework::OpenGl::CProgram>();

	{
		program->AttachShader(vertexShader);
		program->AttachShader(pixelShader);

		glBindAttribLocation(*program, static_cast<GLuint>(PRIM_VERTEX_ATTRIB::POSITION), "a_position");
		glBindAttribLocation(*program, static_cast<GLuint>(PRIM_VERTEX_ATTRIB::TEXCOORD), "a_texCoord");

		FRAMEWORK_MAYBE_UNUSED bool result = program->Link();
		assert(result);
		CHECKGLERROR();

	}

	return program;
}
