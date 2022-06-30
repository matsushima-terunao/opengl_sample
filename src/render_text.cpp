//2021/4/27
//https://gitlab.com/wikibooks-opengl/modern-tutorials/-/tree/master/text01_intro
//https://learnopengl.com/In-Practice/Text-Rendering
//https://en.wikibooks.org/wiki/OpenGL_Programming/Modern_OpenGL_Tutorial_Text_Rendering_02

#include "render_text.hpp"
#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
#include <GLFW/glfw3.h>
#pragma clang pop
#include "linmath.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <iostream>
#include <map>
#include <assert.h>
/*
#include "misc.hpp"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#pragma warning(push, 0)
#include "glfw/deps/linmath.h"
#undef inline
#pragma warning(pop)
#include <ft2build.h>
#include FT_FREETYPE_H
#include <map>
#include <string>
#include <vector>
*/

static const char* text_vertex_shader = R"(
#version 330 core
layout (location = 0) in vec4 vertex;
out vec2 texture_uv;
uniform mat4 projection;
void main() {
	gl_Position = projection * vec4(vertex.xy, 1.0, 1.0);
	texture_uv = vertex.zw;
}
)";

static const char* text_fragment_shader = R"(
#version 330 core
in vec2 texture_uv;
out vec4 fragment_color;
uniform sampler2D fragment_texture;
uniform vec3 texture_color;
void main() {
	fragment_color = vec4(texture_color, 1.0) * vec4(1.0, 1.0, 1.0, texture(fragment_texture, texture_uv).r);
}
)";

static const unsigned int SCR_WIDTH = 800;
static const unsigned int SCR_HEIGHT = 600;

struct character_rectangle_info {
	int x, y, w, h;
	int advance_x;
};

static FT_Library ft;
static FT_Face face;
static std::map<GLchar, character_rectangle_info> character_textures;
static GLuint program;
static unsigned int texture;
static unsigned int vertex_array, vertex_object;
constexpr int ch_width = 48, ch_count = 128, str_length = 256;

/**
 * シェーダー作成。
 */
static GLuint create_shader(const char* name, const GLuint program, GLenum shaderType, const GLchar** string) {
    // シェーダ―作成
    const GLuint shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, string, NULL);
    glCompileShader(shader);
    // コンパイル結果
    GLint compile_status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compile_status);
    if (GL_FALSE == compile_status) {
        std::cerr << "create_shader(): !glCompileShader(): " << name << std::endl;
    }
    GLsizei maxLength, length;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);
    if (maxLength > 1) {
        std::vector<GLchar> infoLog(maxLength);
        glGetShaderInfoLog(shader, maxLength, &length, infoLog.data());
        std::cerr << "create_shader(): glGetShaderInfoLog(): " << name << std::endl;
        std::cerr << infoLog.data() << std::endl;
    }
    assert(GL_FALSE != compile_status && "create_shader(): !glCompileShader()");
    if (GL_FALSE == compile_status) {
        exit(1);
    }
    // プログラムオブジェクトにアタッチ
    glAttachShader(program, shader);
    glDeleteShader(shader);
    return shader;
}

/**
 * テキスト描画初期化。
 */
int init_render_text() {
	glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	// シェーダー作成。
	program = glCreateProgram();
	create_shader("text vertex shader", program, GL_VERTEX_SHADER, &text_vertex_shader);
	create_shader("text fragment shader", program, GL_FRAGMENT_SHADER, &text_fragment_shader);
	glLinkProgram(program);
	glUseProgram(program);
	// 透視設定
	mat4x4 projection;
	mat4x4_ortho(projection, 0.0f, static_cast<float>(SCR_WIDTH), 0.0f, static_cast<float>(SCR_HEIGHT), 1.f, -100.f);
	glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_FALSE, (const GLfloat*)projection);
	// FreeType
	FT_Error ft_error;
	ft_error = FT_Init_FreeType(&ft);
	assert(!ft_error && "init_resources: FT_Init_FreeType();");
	std::string font_name = "assets/font/FreeSans.ttf";
	ft_error = FT_New_Face(ft, font_name.c_str(), 0, &face);
	assert(!ft_error && "init_resources: FT_New_Face();");
	FT_Set_Pixel_Sizes(face, 0, ch_width);
	// テクスチャー作成
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // disable byte-alignment restriction
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, ch_width * ch_count, ch_width, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
	// 文字ごとのイメージ書き込み
	for (unsigned char c = 0; c < ch_count; ++c) {
		ft_error = FT_Load_Char(face, c, FT_LOAD_RENDER);
		if (ft_error) {
			assert(!ft_error && "init_resources: FT_LOAD_RENDER();");
			continue;
		}
		if (face->glyph->bitmap.width > 0) {
			glTexSubImage2D(GL_TEXTURE_2D, 0, c * ch_width, 0, face->glyph->bitmap.width, face->glyph->bitmap.rows,
				GL_RED, GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer);
		}
		character_textures[c] = {
			face->glyph->bitmap_left, face->glyph->bitmap_top,
			(int)face->glyph->bitmap.width, (int)face->glyph->bitmap.rows,
			(int)face->glyph->advance.x,
		};
	}
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);
	FT_Done_Face(face);
	FT_Done_FreeType(ft);
	// VAO/VBO 作成
	glGenVertexArrays(1, &vertex_array);
	glGenBuffers(1, &vertex_object);
	glBindVertexArray(vertex_array);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_object);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4 * str_length, NULL, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	return 0;
}

/**
 * テキスト描画。
 */
float render_text(const char* text, float x, float y, float scale, float color[3], bool proportional) {
	glUseProgram(program);
	glUniform3f(glGetUniformLocation(program, "texture_color"), color[0], color[1], color[2]);
	glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(vertex_array);
	char* vertices_buf = new char[strlen(text) * 4 * 24];
	int i;
	for (i = 0; i < str_length && '\0' != text[i]; ++i) {
		// 文字ごとにポリゴン追加
		character_rectangle_info& ch = character_textures[text[i]];
		float xpos = x + ch.x * scale;
		float ypos = y - (ch.h - ch.y) * scale;
		float w = ch.w * scale;
		float h = ch.h * scale * 1.5f;
		float u = 1.0f / ch_count * text[i];
		float uw = (float)ch.w / ch_width / ch_count;
		float uh = (float)ch.h / ch_width;
		if (!proportional) {
			xpos += (ch_width * scale - w) / 2;
		}
		float vertices[6][4] = {
			{ xpos,     ypos + h, u,      0.0f }, // 0
			{ xpos,     ypos,     u,      uh },
			{ xpos + w, ypos,     u + uw, uh },
			{ xpos,     ypos + h, u,      0.0f }, // 1
			{ xpos + w, ypos,     u + uw, uh },
			{ xpos + w, ypos + h, u + uw, 0.0f },
		};
		memcpy(vertices_buf + i * sizeof(vertices), vertices, sizeof(vertices));
		x += proportional ? (ch.advance_x >> 6) * scale : ch_width * scale / 1.5f;
	}
	// ポリゴン描画
	glBindTexture(GL_TEXTURE_2D, texture);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_object);
	glBufferSubData(GL_ARRAY_BUFFER, 0, i * 6ull * 4 * sizeof(float), vertices_buf); // be sure to use glBufferSubData and not glBufferData
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDrawArrays(GL_TRIANGLES, 0, 6 * i);
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	delete[] vertices_buf;
	return x;
}

/**
 * テキスト一覧描画。
 */
float render_text_list(std::vector<std::pair<std::string, float*>>& render_text_list, float x, float y, float scale, bool proportional) {
	for (size_t i = 0, c = render_text_list.size(); i < c; ++i) {
		if (nullptr == render_text_list[i].second) {
			x = 0;
			y -= ch_width * scale + 5.0f;
		}
		else {
			x = render_text(render_text_list[i].first.c_str(), x, y, scale, render_text_list[i].second, proportional);
		}
	}
	return x;
}

/**
 * テキスト追加。
 */
void append_render_text(std::vector<std::pair<std::string, float*>>& render_text_list, const char* text, float* color) {
	if (render_text_list.empty() || nullptr == color || nullptr == render_text_list.back().second
		|| !std::equal(color, color + 3, render_text_list.back().second)) {
		render_text_list.emplace_back(text, color);
	}
	else {
		render_text_list.back().first += text;
	}
}
