#if 0
//
//  sample_all.cpp
//  opengl_sample
//
//  Created by matsushima on 2021/07/21.
//  Copyright © 2021 matsushima. All rights reserved.
//

#include "fractal_texture.hpp"
#include "image.hpp"
#include "model.hpp"
#include "render_text.hpp"
#include "sphare_mesh.hpp"
#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#ifdef __APPLE__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
#include <GLFW/glfw3.h>
#pragma clang pop
#else
#include <GLFW/glfw3.h>
#endif
#include "linmath.h"
#include <functional>
#include <iomanip>
#include <iostream>
#include <unordered_map>
#include <sstream>
#include <vector>

// シーン情報

#define MODE_COLOR_MASK (MODE_COLOR_NONE | MODE_COLOR_VERTEX | MODE_COLOR_TEXTURE)
#define MODE_SHADING_MASK (MODE_SHADING_FLAT | MODE_SHADING_GOUROUD | MODE_SHADING_PHONG)
#define MODE_PROJECTION_MASK (MODE_PROJECTION_ORTHO | MODE_PROJECTION_PERSPECTIVE)
#define MODE_COLOR_NONE 1
#define MODE_COLOR_VERTEX 2
#define MODE_COLOR_TEXTURE 4
#define MODE_SHADING_FLAT 8
#define MODE_SHADING_GOUROUD 16
#define MODE_SHADING_PHONG 32
#define MODE_LIGHTING 64
#define MODE_PROJECTION_ORTHO 128
#define MODE_PROJECTION_PERSPECTIVE 256

/** モデル数。 */
constexpr int model_cnt = 5;

/** モデル位置・角度。 */
static Model models[model_cnt] = {
    { 0.0f, 0.0f, 0.0f,  0.5f, 0.0f, 0.0f,  0.0f, 0.0f, 0.003f },
    { 0.0f, 0.0f, -3.0f,  0.0f, 0.0f, 0.0f,  0.03f, 0.02f, 0.01f, },
    { 0.0f, 0.0f, -3.0f,  0.0f, 0.0f, 0.0f,  0.03f, 0.02f, 0.01f, },
    { 0.0f, 0.0f, -2500.0f,  0.5f, 0.0f, 0.0f,  0.0f, 0.003f, 0.0f },
    { 0.0f, 0.0f, -2500.0f,  0.5f, 0.0f, 0.0f,  0.0f, 0.003f, 0.0f },
};

/** 表示モデル。 */
static int model_idx = 2;

/** 表示モード。 */
static int render_mode = MODE_COLOR_TEXTURE | MODE_SHADING_PHONG | MODE_LIGHTING | MODE_PROJECTION_PERSPECTIVE;

/** バッファー切り替え間隔。 */
static int interval = 1;

/** カメラ情報。 */
static vec3 camera_eye = { 0.0f, 0.0f, 1.0f }, camera_center = { 0.f, 0.f, 0.f }, camera_up = { 0.f, 1.0f, 0.f };

/** 正投影情報。 */
struct ortho { float left, right, bottom, top, near, far; };
/** 正投影情報。 */
static struct ortho ortho = { .0f, .0f, .0f, .0f, 0.1f, 10000.0f };
/** 透視投影情報。 */
struct perspective { float y_fov, aspect, near, far; };
/** 透視投影情報。 */
static struct perspective perspective = { 1.5f, .0f, 0.1f, 10000.0f };

// シェーダー情報

/**
 * 頂点情報 uniform 構造体定義。
 * @see vertex_shader_src: layout(std140) uniform vertex_uniform
 * @see create_uniform_buffer
 */
struct vertex_uniform {
    int mode, pad11, pad12, pad13; // rendering mode
    mat4x4 modelview_matrix;
    mat4x4 modelview_normal_matrix; // transpose(inverse(modelview_matrix))
    mat4x4 modelview_projection_matrix;
};
/**
 * ライティング情報 uniform 構造体定義。
 * @see lighting_shader_src: layout(std140) uniform light_uniform
 * @see create_uniform_buffer
 */
struct light_uniform {
    float Ka; // ambient reflection coefficient
    float Kd; // diffuse reflection coefficient
    float Ks; // specular reflection coefficient
    float shininess;
    vec3 ambient_color; float pad21;
    vec3 diffuse_color; float pad22;
    vec3 specular_color; float pad23;
    vec3 light_position;
};
/** 頂点情報 uniform。 */
static struct vertex_uniform vertex_uniform = {
};
/** ライティング情報 uniform。 */
static struct light_uniform light_uniform = {
    .Ka = 1.0f, .Kd = 1.0f, .Ks = 1.0f, .shininess = 80.0f,
    //.ambient_color = { 0.2f, 0.2f, 0.2f }, .diffuse_color = { 0.6f, 0.6f, 0.6f }, .specular_color = { 1.0f, 1.0f, 1.0f },
    .ambient_color = { 0.6f, 0.6f, 0.6f }, .diffuse_color = { 0.8f, 0.8f, 0.8f }, .specular_color = { 1.0f, 1.0f, 1.0f },
    .light_position = { 3000.0f, 3000.0f, 0.0f },
};

/*
 * 頂点情報の location。
 * @see struct Vertex
 * @see vertex_shader_src: layout (location = _location) in ...;
 * @see create_vertex: glEnableVertexAttribArray(_location);
 * @see create_vertex: glVertexAttribPointer(_location, ...);
 */
constexpr GLuint position_location = 0; // x, y, z: 頂点座標
constexpr GLuint color_location = 1; // r, g, b: 頂点カラー
constexpr GLuint texture_uv_location = 2; // u, v: テクスチャーのUVマッピング座標
constexpr GLuint normal_location = 3; // nx, ny, nz: 頂点の法線ベクトル

/*
 * シェーダー uniform の　binding point。
 * @see create_uniform_buffer: glUniformBlockBinding(program, index, binding);
 * @see create_uniform_buffer: glBindBufferBase(GL_UNIFORM_BUFFER, binding, uniform_buffer); // or glBindBufferRange()
 */
constexpr GLuint vertex_uniform_binding = 0;
constexpr GLuint light_uniform_binding = 1;

/** バーテックス・フラグメントシェーダーのライティングソースプログラム。 */
static const std::string lighting_shader_src = R"(
#version 410 core

#define MODE_COLOR_NONE 1
#define MODE_COLOR_VERTEX 2
#define MODE_COLOR_TEXTURE 4
#define MODE_SHADING_FLAT 8
#define MODE_SHADING_GOUROUD 16
#define MODE_SHADING_PHONG 32
#define MODE_LIGHTING 64
#define MODE_PROJECTION_ORTHO 128
#define MODE_PROJECTION_PERSPECTIVE 256

/**
 * ライティング情報 uniform 構造体定義、現在データ。
 * @see struct light_uniform
 */
layout(std140) uniform light_uniform {
float Ka; // ambient reflection coefficient
float Kd; // diffuse reflection coefficient
float Ks; // specular reflection coefficient
float shininess;
    vec3 ambient_color; float pad21;
    vec3 diffuse_color; float pad22;
    vec3 specular_color; float pad23;
    vec3 light_position;
};

/**
 * ライティング。
 */
vec4 lighting(vec3 vertex_normal, vec3 vertex_position) {
    // Ip = ka*ia + Σlights(kd(L*N)id + ks(R*V)^a*is)
    // ambient
    vec3 ambient = Ka * ambient_color;
    // diffse
    vec3 N = normalize(vertex_normal); // point normal
    vec3 L = normalize(light_position - vertex_position); // point to light direction
    float lambertian = max(dot(N, L), 0.0);
    vec3 diffuse = clamp(Kd * lambertian * diffuse_color, 0.0, 1.0);
    // specular
    vec3 R = reflect(-L, N); // point to light direction
    vec3 V = normalize(-vertex_position); // point to view direction
    vec3 specular = clamp(Ks * pow(max(dot(R, V), 0.0), shininess) * specular_color, 0.0, 1.0);
    return vec4(ambient + diffuse + specular, 1.0);
}
)";

/** バーテックスシェーダーのソースプログラム。 */
static const std::string vertex_shader_src = lighting_shader_src + R"(
/**
 * 頂点情報 uniform 構造体定義、現在データ。
 * @see struct vertex_uniform
 */
layout(std140) uniform vertex_uniform {
    int mode, pad11, pad12, pad13; // rendering mode
    mat4 modelview_matrix;
    mat4 modelview_normal_matrix; // transpose(inverse(modelview_matrix))
    mat4 modelview_projection_matrix;
};
layout (location = 0) in vec3 position; // x, y, z: 頂点座標
layout (location = 1) in vec3 color; // r, g, b: 頂点カラー
layout (location = 2) in vec2 texture_uv; // u, v: テクスチャーのUVマッピング座標
layout (location = 3) in vec3 normal; // nx, ny, nz: 頂点の法線ベクトル
flat out int vertex_mode; // rendering mode
out vec3 vertex_position; // 頂点座標
out vec3 vertex_normal; // 頂点の法線ベクトル
out vec4 vertex_color; // 頂点カラー
flat out vec4 vertex_color_flat; // フラットシェーディングの頂点カラー
out vec2 vertex_texture_uv; // テクスチャーのUVマッピング座標

void main() {
    gl_Position = modelview_projection_matrix * vec4(position, 1.0); // 頂点座標
    vertex_mode = mode; // rendering mode
    vertex_position = vec3(modelview_matrix * vec4(position, 1.0)); // 頂点座標
    vertex_normal = vec3(modelview_normal_matrix * vec4(normal, 0.0)); // 頂点の法線ベクトル
    vertex_color = vec4(1.0, 1.0, 1.0, 1.0); // 頂点カラー
    // 頂点カラーを使用
    if ((mode & MODE_COLOR_VERTEX) == MODE_COLOR_VERTEX) {
        vertex_color = vec4(color, 1.0);
    }
    // フラットシェーディング・グーローシェーディングはライティングをバーテックスシェーダーで行う
    if ((mode & MODE_LIGHTING) == MODE_LIGHTING
        && (mode & MODE_SHADING_PHONG) != MODE_SHADING_PHONG) {
        vertex_color = lighting(vertex_normal, vertex_position) * vertex_color;
    }
    // フラットシェーディングの頂点カラー
    vertex_color_flat = vertex_color;
    // テクスチャーのUVマッピング座標
    vertex_texture_uv = texture_uv;
}
)";

/** フラグメントシェーダーのソースプログラム。 */
static const std::string fragment_shader_src = lighting_shader_src + R"(
uniform sampler2D fragment_texture; // テクスチャー
flat in int vertex_mode; // rendering mode
in vec3 vertex_position; // 頂点座標
in vec3 vertex_normal; // 頂点の法線ベクトル
in vec4 vertex_color; // 頂点カラー
flat in vec4 vertex_color_flat; // フラットシェーディングの頂点カラー
in vec2 vertex_texture_uv; // テクスチャーのUVマッピング座標
out vec4 fragment_color; // 出力ピクセルカラー

void main() {
    // 頂点カラー
    fragment_color = vertex_color;
    // フラットシェーディングの頂点カラー
    if ((vertex_mode & MODE_SHADING_FLAT) == MODE_SHADING_FLAT) {
        fragment_color = vertex_color_flat;
    }
    // テクスチャーのカラー
    if ((vertex_mode & MODE_COLOR_TEXTURE) == MODE_COLOR_TEXTURE) {
        fragment_color = texture(fragment_texture, vertex_texture_uv) * fragment_color;
    }
    // フォンシェーディングはライティングをフラグメントシェーダーで行う
    if ((vertex_mode & MODE_LIGHTING) == MODE_LIGHTING
        && (vertex_mode & MODE_SHADING_PHONG) == MODE_SHADING_PHONG) {
        fragment_color = lighting(vertex_normal, vertex_position) * fragment_color;
    }
}
)";

// エラー処理

/**
 * GLエラー出力。
 */
static GLenum gl_get_errors(const char* file, int line, const char* msg = "") {
    if (NULL == glGetError) {
        std::cerr << file << "(" << line << "): " << "gl_get_errors(): NULL == glGetError" << msg << std::endl;
        return 1;
    }
    GLenum error, first_error = GL_NO_ERROR;
    while (GL_NO_ERROR != (error = glGetError())) {
        if (GL_NO_ERROR == first_error) {
            first_error = error;
        }
        std::cerr << file << "(" << line << "): " << "gl_get_errors(): glGetError() = " << msg << error << std::endl;
    }
    return first_error;
}

#define GL_GET_ERRORS(...) gl_get_errors(__FILE__, __LINE__ __VA_ARGS__)

/**
 * 終了ハンドラ。
 */
static void atexit_function() {
    GLenum error = GL_GET_ERRORS();
    assert(GL_NO_ERROR == error && "atexit_function(): glGetError();");
    glfwTerminate();
}

/**
 * GLFW エラーのコールバック。
glfw_error_callback: 65543: WGL: Driver does not support OpenGL version 5.3
Assertion failed: 0 && "glfw_error_callback", file C:\Users\matsu\source\repos\opengl\opengl\game_sample4.cpp, line 296
 */
static void glfw_error_callback(int error, const char* description) {
    std::cerr << "glfw_error_callback(): " << error << ": " << description << std::endl;
    assert(0 && "glfw_error_callback()");
}

/**
 * OpenGL エラー・デバッグメッセージのコールバック。
gl_debug_message_callback: source = 33352 type = 824c(GL_DEBUG_TYPE_ERROR) id = 0 severity = 9146 length = 90 userParam = 0000000000000000
message = SHADER_ID_COMPILE error has been generated. GLSL compile failed for shader 2, "": ERROR: 0:3: 'coreaaa' : unknown profile in #version directive

create_shader(): !glCompileShader(): vertex shader
create_shader(): glGetShaderInfoLog(): vertex shader
ERROR: 0:3: 'coreaaa' : unknown profile in #version directive

Assertion failed: GL_FALSE != compile_status && "create_shader(): !glCompileShader()", file C:\Users\matsu\source\repos\opengl\opengl\game_sample4.cpp, line 438
 */
static void GLAPIENTRY gl_debug_message_callback(
    GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
    std::cerr << "gl_debug_message_callback(): source = " << source
        << " type = " << std::hex << type << std::dec << (GL_DEBUG_TYPE_ERROR == type ? "(GL_DEBUG_TYPE_ERROR)" : "")
        << " id = " << id << " severity = " << severity << " length = " << length << " userParam = " << userParam
        << "\nmessage = " << message << std::endl;
}

// glラッパー

/**
 * 頂点バッファー作成。
 */
static void create_vertex_buffer(
    GLuint& array_buffer, GLuint& vertex_buffer, GLuint& element_buffer,
    GLsizeiptr size, const void* data, GLsizei stride, GLsizeiptr element_size, const void* element_data
) {
    std::cout << "< create_vertex_buffer(): size = " << size << ", element_size = " << element_size << std::endl;
    // VAO(vertex array object) 作成
    glGenVertexArrays(1, &array_buffer);
    assert(0 != array_buffer && "create_vertex_buffer(): glGenVertexArrays(1, &array_buffer);");
    glBindVertexArray(array_buffer);
    // VBO(vertex buffer object) 作成
    glGenBuffers(1, &vertex_buffer);
    assert(0 != vertex_buffer && "create_vertex_buffer(): glGenBuffers(1, &vertex_buffer);");
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
    glEnableVertexAttribArray(position_location);
    glVertexAttribPointer(position_location, 3, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(color_location);
    glVertexAttribPointer(color_location, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(texture_uv_location);
    glVertexAttribPointer(texture_uv_location, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(GLfloat)));
    glEnableVertexAttribArray(normal_location);
    glVertexAttribPointer(normal_location, 3, GL_FLOAT, GL_FALSE, stride, (void*)(8 * sizeof(GLfloat)));
    // EBO(element array buffer object) 作成
    if (nullptr != element_data) {
        glGenBuffers(1, &element_buffer);
        assert(0 != element_buffer && "create_vertex_buffer(): glGenBuffers(1, &element_buffer);");
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_buffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, element_size, element_data, GL_STATIC_DRAW);
    }
}

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
 * UBO(uniform buffer object) 作成。
 */
static GLuint create_uniform_buffer(GLsizeiptr size, GLuint program, const GLchar* uniformBlockName, GLuint binding) {
    GLuint uniform_buffer;
    glGenBuffers(1, &uniform_buffer);
    glBindBuffer(GL_UNIFORM_BUFFER, uniform_buffer);
    glBufferData(GL_UNIFORM_BUFFER, size, NULL, GL_STREAM_DRAW);
    GLuint index = glGetUniformBlockIndex(program, uniformBlockName);
    glUniformBlockBinding(program, index, binding);
    glBindBufferBase(GL_UNIFORM_BUFFER, binding, uniform_buffer); // or glBindBufferRange()
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    return uniform_buffer;
}

/**
 * テクスチャー作成。
 */
static GLuint create_texture(unsigned char* data, int width, int height, int cvt_color = 0, const int* palette = nullptr) {
    // パレット -> RGB
    if (palette) {
        int* pixels = (int*)data;
        for (int i = 0; i < width * height; ++i) {
            pixels[i] = palette[pixels[i]];
        }
    }
    // R/B 入れ替え
    if (cvt_color) {
        for (int i = 0; i < width * height; ++i) {
            std::swap(data[i * cvt_color], data[i * cvt_color + 2]);
        }
    }
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    GLenum format = (4 == cvt_color) ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    return texture;
}

// モデル

/**
 * triangle model
 */
static void create_triangle_model(Model& model) {
    static float vertsf[] = {
         -0.6f, -0.4f, 0.f,  1.f, 0.f, 0.f,  0.f, 0.f,   0.f, 0.f, -1.f,
          0.6f, -0.4f, 0.f,  0.f, 1.f, 0.f,  0.f, 0.f,   0.f, 0.f, -1.f,
           0.f,  0.6f, 0.f,  0.f, 0.f, 1.f,  0.f, 0.f,   0.f, 0.f, -1.f,
    };
    model.vertsf = vertsf;
    model.verts_count = sizeof(vertsf) / sizeof(Vertex);
    model.verts_stride = sizeof(Vertex);
    model.polys = nullptr;
    model.polys_count = 0;
    create_vertex_buffer(model.vertex_array, model.vertex_buffer, model.element_buffer,
        sizeof(vertsf), vertsf, sizeof(Vertex), 0, nullptr);
}

/**
 * asset model
 */
static void create_asset_model(Model& model, const char* mesh_path, const char* image_path = nullptr) {
    read_mesh(mesh_path, model.vertex_list);
    model.vertsf = (float*)model.vertex_list.data();
    model.verts_count = model.vertex_list.size();
    model.verts_stride = sizeof(Vertex);
    create_vertex_buffer(model.vertex_array, model.vertex_buffer, model.element_buffer,
        model.verts_count * model.verts_stride, model.vertsf, (GLsizei)model.verts_stride,
        0, nullptr);
    if (image_path) {
        model.texture = read_image(image_path);
    }
}

/**
 * earth sphere model
 */
static void create_earth_model(Model& model) {
    // テクスチャー作成
    int bytes_width_bits = 9;
    int colors = 256;
    model.width = model.height = 1 << bytes_width_bits;
    // フラクタルのビットマップ作成。
    //srand((unsigned int)time(NULL));
    float* frac1 = create_fractal(bytes_width_bits, 0.8); // 地表
    float* frac2 = create_fractal(bytes_width_bits, 0.6); // 雲
    // パレット作成。
    int color_table[] = {
        0, 0, 0, 0,
        145, 40, 40, 240,
        160, 160, 100, 80,
        180, 20, 120, 0,
        235, 100, 30, 10,
        245, 128, 128, 128,
        256, 255, 255, 254,
    };
    model.palette = create_color_palette(colors, color_table);
    // [0.0f..1.0f] -> [0..colors -1]、テクスチャの合成
    model.pixels = new int[(size_t)model.width * model.height];
    if (nullptr != frac2) {
        for (int i = 0; i < model.width * model.height; ++i) {
            int c = model.palette[(int)(frac1[i] * (colors - 1))];
            // frac2(0.5 .. 1.0] -> frac1[0.0 .. 1.0-c2] + frac2[0.0 .. 1.0]
            float border = 0.5f;
            if (frac2[i] > border) {
                float c2 = (frac2[i] - border) / (1.0f - border);
                float r = (((c >> 16) & 255) * (1.0f - c2)) + (colors - 1) * c2;
                float g = (((c >> 8) & 255) * (1.0f - c2)) + (colors - 1) * c2;
                float b = (((c >> 0) & 255) * (1.0f - c2)) + (colors - 1) * c2;
                c = (255 << 24) | ((int)r << 16) | ((int)g << 8) | (int)b;
            }
            model.pixels[i] = c;
        }
    }
    delete[] frac1;
    delete[] frac2;

    // 球体メッシュ作成。
    int width = 2000;
    create_sphere_mesh(
        width, width, 0, bytes_width_bits, 2, model.pixels, nullptr,
        8, 4, 8,//16, 4, 8 * 8);//16, 4*4, 8*4);
        model.vertsf, model.verts_count, model.verts_stride, model.polys, model.polys_count);
    // 頂点バッファー作成。
    create_vertex_buffer(model.vertex_array, model.vertex_buffer, model.element_buffer,
        model.verts_count * model.verts_stride, model.vertsf, (GLsizei)model.verts_stride,
        model.polys_count * sizeof(model.polys[0]), model.polys);

    // テクスチャー作成。
    model.texture = create_texture((unsigned char*)model.pixels, model.width, model.height, 4);
}

/**
 * rock sphere model
 */
static void create_rock_model(Model& model) {
    // テクスチャー作成
    int bytes_width_bits = 9;
    int colors = 256;
    model.width = model.height = 1 << bytes_width_bits;
    // フラクタルのビットマップ作成。
    float* frac1 = create_fractal(bytes_width_bits, 0.8);
    // パレット作成。
    int color_table[] = {
        0,  64,  64,  64, // w
        128,  32,  32,  32, // w
        256, 128, 128, 128, // s
    };
    model.palette = create_color_palette(colors, color_table);
    // [0.0f..1.0f] -> [0..colors -1]
    model.pixels = new int[(size_t)model.width * model.height];
    for (int i = 0; i < model.width * model.height; ++i) {
        model.pixels[i] = (int)(255 * frac1[i]);
    }
    delete[] frac1;

    // 球体メッシュ作成。
    int width = 1000;
    create_sphere_mesh(
        width, width, 1, bytes_width_bits, 2, model.pixels, model.palette,
        8, 4, 8,//16, 4, 8 * 8);//16, 4*4, 8*4);
        model.vertsf, model.verts_count, model.verts_stride, model.polys, model.polys_count);
    // 破片データ作成。
    create_fragment(model);
    // 頂点バッファー作成。
    create_vertex_buffer(model.vertex_array, model.vertex_buffer, model.element_buffer,
        std::max(model.verts_count, model.fragments_count) * model.verts_stride, model.vertsf, (GLsizei)model.verts_stride,
        model.polys_count * sizeof(model.polys[0]), model.polys);

    // テクスチャー作成。
    model.texture = create_texture((unsigned char*)model.pixels, model.width, model.height, 4, model.palette);
}

/**
 * モデル変更。
 */
static void translate_model(Model& model) {
    vertex_uniform.mode = render_mode;
    if (!model.texture && (vertex_uniform.mode & MODE_COLOR_TEXTURE)) {
        vertex_uniform.mode = (vertex_uniform.mode & ~MODE_COLOR_TEXTURE) | MODE_COLOR_VERTEX;
    }
    // モデルパラメーター
    ++model.cnt;
    model.a += model.va;
    model.b += model.vb;
    model.c += model.vc;
    // 破片データ更新。
    if (nullptr != model.fragments1) {
        if (model.cnt % 200 >= 100) {
            glBindBuffer(GL_ARRAY_BUFFER, model.vertex_buffer);
            //glBufferSubData(GL_ARRAY_BUFFER, 0, size, data);
            float* buf = (float*)glMapBufferRange(GL_ARRAY_BUFFER, 0, model.verts_count * model.verts_stride, GL_MAP_WRITE_BIT);
            translate_vertex(model, buf);
            glUnmapBuffer(GL_ARRAY_BUFFER);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        } else if (model.cnt % 200 == 0) {
            glBindBuffer(GL_ARRAY_BUFFER, model.vertex_buffer);
            //glBufferSubData(GL_ARRAY_BUFFER, 0, size, data);
            float* buf = (float*)glMapBufferRange(GL_ARRAY_BUFFER, 0, model.verts_count * model.verts_stride, GL_MAP_WRITE_BIT);
            init_fragment(model, buf);
            glUnmapBuffer(GL_ARRAY_BUFFER);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
    }
}

/**
 * モデル変換。
 */
static void transform_model(Model& model, float time, float ratio) {
    // 変換
    mat4x4 model_matrix, view_matrix, projection_matrix, modelview_invert_matrix;
    // モデル変換
    mat4x4_identity(model_matrix);
    mat4x4_translate(model_matrix, model.x, model.y, model.z);
    mat4x4_rotate_X(model_matrix, model_matrix, model.a);
    mat4x4_rotate_Y(model_matrix, model_matrix, model.b);
    mat4x4_rotate_Z(model_matrix, model_matrix, model.c);
    // ビュー変換
    mat4x4_look_at(view_matrix, camera_eye, camera_center, camera_up);
    // プロジェクション変換
    ortho.left = -ratio;
    ortho.right = ratio;
    ortho.bottom = -1.0;
    ortho.top = 1.0;
    perspective.aspect = ratio;
    if (MODE_PROJECTION_ORTHO & vertex_uniform.mode) {
        mat4x4_ortho(projection_matrix, ortho.left, ortho.right, ortho.bottom, ortho.top, ortho.near, ortho.far);
    }
    else if (MODE_PROJECTION_PERSPECTIVE & vertex_uniform.mode) {
        mat4x4_perspective(projection_matrix, perspective.y_fov, perspective.aspect, perspective.near, perspective.far);
    }
    // MVP
    mat4x4_mul(vertex_uniform.modelview_matrix, view_matrix, model_matrix);
    mat4x4_mul(vertex_uniform.modelview_projection_matrix, projection_matrix, vertex_uniform.modelview_matrix);
    // mat4 modelview_normal_matrix; // transpose(inverse(modelview_matrix))
    mat4x4_invert(modelview_invert_matrix, vertex_uniform.modelview_matrix);
    mat4x4_transpose(vertex_uniform.modelview_normal_matrix, modelview_invert_matrix);
}

// 入力

static std::unordered_map<int, bool> key_map; // キー押下状態
static std::vector<std::pair<std::string, float*>> render_text_listv; // パラメーター表示用

/**
 * キー押下判定。
 */
static bool is_key(int key, bool is_once) {
    bool result = ((key_map.find(key) != key_map.end()) && key_map[key]);
    key_map[key] = (!is_once && result);
    return result;
}

/**
 * キー入力のコールバック。
 */
static void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (GLFW_KEY_ESCAPE == key && GLFW_PRESS == action) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
    key_map[key] = (GLFW_PRESS == action || GLFW_REPEAT == action);
}

/**
 * パラメーター変更UI。
 */
static void change_params(Model& model) {
    // パラメーター変更項目
    struct param_info {
        const char* name = "  "; // +改行 *変更不可 テキスト
        int* flagsip = nullptr; // フラグ参照先 flag, mask
        int* rangeip = nullptr; // 範囲参照先 min, max, step
        float* rangefp = nullptr; // 範囲参照先 minf, maxf, stepf
        int flag, mask, min, max, step;
        float minf, maxf, stepf;
        std::function<void()> post_func = [] {};
        int x, y = -1;
        param_info* ref; // 参照元の param_info
    };
    static param_info param_info[] = {
        {.name = "+ model="     , .rangeip = &model_idx, .min = 0, .max = model_cnt - 1, .step = 1},
        {.name = "  interval="  , .rangeip = &interval, .min = 0, .max = 10, .step = 1},
        {.name = "+*color: "    , .flagsip = &render_mode, .mask = MODE_COLOR_MASK},
        {.name = "  none "      , .flag = MODE_COLOR_NONE}, {.name = "  vertex ", .flag = MODE_COLOR_VERTEX}, {.name = "  texture", .flag = MODE_COLOR_TEXTURE},
        {.name = " *   shading: "   , .flagsip = &render_mode, .mask = MODE_SHADING_MASK},
        {.name = "  flat "          , .flag = MODE_SHADING_FLAT}, {.name = "  gouroud ", .flag = MODE_SHADING_GOUROUD}, {.name = "  phong", .flag = MODE_SHADING_PHONG},
        {.name = "+ model: position="   , .rangefp = &model.x, .minf = -INFINITY, .maxf = INFINITY, .stepf = 0.1f}, {}, {},
        {.name = "  angle="             , .rangefp = &model.a, .minf = -INFINITY, .maxf = INFINITY, .stepf = 0.1f}, {}, {},
        {.name = "  dangle="            , .rangefp = &model.va, .minf = -1.f, .maxf = 1.0f, .stepf = 0.01f}, {}, {},
        {.name = "+ camera: eye="       , .rangefp = camera_eye, .minf = -10.f, .maxf = 10.0f, .stepf = 0.1f}, {}, {},
        {.name = "  center="            , .rangefp = camera_center, .minf = -10.f, .maxf = 10.0f, .stepf = 0.1f}, {}, {},
        {.name = "  up="                , .rangefp = camera_up, .minf = -10.f, .maxf = 10.0f, .stepf = 0.1f}, {}, {},
        {.name = "+*projection: "   , .flagsip = &render_mode, .mask = MODE_PROJECTION_MASK},
        {.name = "  ortho "         , .flag = MODE_PROJECTION_ORTHO}, {.name = "  perspective", .flag = MODE_PROJECTION_PERSPECTIVE},
        {.name = "+*ortho: left="   , .rangefp = &ortho.left, .minf = 0.f, .maxf = 0.f, .stepf = 0.1f}, {.name = " *right="}, {.name = " *bottom="}, {.name = " *top="},
        {.name = "  near="          , .rangefp = &ortho.near, .minf = 0.f, .maxf = 100.f, .stepf = 0.1f},
        {.name = "  far="           , .rangefp = &ortho.far , .minf = 0.f, .maxf = 100.f, .stepf = 1.0f},
        {.name = "+ perspective: y_fov=", .rangefp = &perspective.y_fov, .minf = 0.f, .maxf = 100.f, .stepf = 0.1f}, {.name = " *aspect="},
        {.name = "  near="              , .rangefp = &perspective.near , .minf = 0.f, .maxf = 100.f, .stepf = 0.1f},
        {.name = "  far="               , .rangefp = &perspective.far  , .minf = 0.f, .maxf = 100.f, .stepf = 1.0f},
        {.name = "+ lighting"       , .flagsip = &render_mode, .flag = MODE_LIGHTING, .mask = 0},
        {.name = "+ material: Ka="  , .rangefp = &light_uniform.Ka, .minf = 0.f, .maxf = 1.0f, .stepf = 0.1f}, {.name = "  Kd="}, {.name = "  Ks="},
        {.name = "  shininess="     , .rangefp = &light_uniform.shininess, .minf = 0.f, .maxf = 100.f, .stepf = 1.0f},
        {.name = "+ light: ambient=", .rangefp = light_uniform.ambient_color, .minf = 0.f, .maxf = 1.0f, .stepf = 0.1f}, {}, {},
        {.name = "  diffuse="       , .rangefp = light_uniform.diffuse_color, .minf = 0.f, .maxf = 1.0f, .stepf = 0.1f}, {}, {},
        {.name = "  specular="      , .rangefp = light_uniform.specular_color, .minf = 0.f, .maxf = 1.0f, .stepf = 0.1f}, {}, {},
        {.name = "  position="      , .rangefp = light_uniform.light_position, .minf = -10.f, .maxf = 10.f, .stepf = 0.1f}, {}, {},
    };
    static int cx = 0, cy = 0; // カーソル位置
    static std::vector<int> cx_max; // xカーソル最大値
    static bool c_enter = false; // パラメーター変更中か

    // 初回初期化
    if (-1 == param_info[0].y) {
        int x = -1, y = -1;
        for (auto& param : param_info) {
            param.ref = &param; // 参照元の param_info
            if (nullptr == param.flagsip && nullptr == param.rangeip && nullptr == param.rangefp) {
                param.ref = (&param - 1)->ref;
                param.flagsip = param.ref->flagsip;
                param.rangeip = nullptr == param.ref->rangeip ? nullptr : param.ref->rangeip + (&param - param.ref);
                param.rangefp = nullptr == param.ref->rangefp ? nullptr : param.ref->rangefp + (&param - param.ref);
            }
            ++x;
            if ('+' == param.name[0]) { // 改行
                x = 0;
                ++y;
                cx_max.push_back(0);
            }
            param.x = x;
            param.y = y;
            if ('*' == param.name[1]) { // 変更不可
                --x;
                param.x = -1;
            }
            cx_max[y] = x;
        }
    }

    // パラメーター変更
    int dx = is_key(GLFW_KEY_RIGHT, true) ? 1 : is_key(GLFW_KEY_LEFT, true) ? -1 : 0;
    int dy = is_key(GLFW_KEY_DOWN, true) ? 1 : is_key(GLFW_KEY_UP, true) ? -1 : 0;
    c_enter = is_key(GLFW_KEY_ENTER, true) ? !c_enter : c_enter;
    if (!c_enter) {
        // カーソル移動
        cy = std::min(std::max(cy + dy, 0), (int)cx_max.size() - 1);
        cx = std::min(std::max(cx + dx, 0), cx_max[cy]);
    }
    else {
        // パラメーター変更
        auto& param = *std::find_if(std::begin(param_info), std::end(param_info), [&](auto& param) { return param.x == cx && param.y == cy; });
        if (nullptr != param.flagsip) {
            *param.flagsip = param.ref->mask ? (*param.flagsip & ~param.ref->mask) | param.flag : *param.flagsip ^ param.flag;
            c_enter = false;
        }
        else if (nullptr != param.rangeip) {
            *param.rangeip = -1 == dy ? param.ref->min : 1 == dy ? param.ref->max
                : std::min(std::max(*param.rangeip + param.ref->step * dx, param.ref->min), param.ref->max);
        }
        else if (nullptr != param.rangefp) {
            if ((-1 == dy && -INFINITY == param.ref->minf) || (1 == dy && INFINITY == param.ref->maxf)) {
                param.rangefp[1] += param.ref->stepf * dy;
            }
            else {
                *param.rangefp = -1 == dy ? param.ref->minf : 1 == dy ? param.ref->maxf
                    : std::min(std::max(*param.rangefp + param.ref->stepf * dx, param.ref->minf), param.ref->maxf);
            }
        }
        param.post_func();
    }

    // パラメーター表示内容
    render_text_listv.clear();
    static float cursor[] = { 0.f, 1.0f, 0.f }; // green
    static float pressed[] = { 1.0f, 0.f, 0.f }; // red
    static float flagset[] = { 1.0f, 1.0f, 0.f }; // yellow
    static float normal[] = { 0.7f, 0.7f, 0.7f }; // gray
    for (auto& param : param_info) {
        if ('+' == param.name[0]) {
            append_render_text(render_text_listv, "\n", nullptr);
        }
        float* color = param.y == cy && param.x == cx ? c_enter ? pressed : cursor : normal;
        if (nullptr != param.flagsip) {
            append_render_text(render_text_listv, param.name + 2, *param.flagsip & param.flag ? flagset : color);
        }
        else if (nullptr != param.rangeip) {
            append_render_text(render_text_listv, param.name + 2, normal);
            append_render_text(render_text_listv, std::to_string(*param.rangeip).append(" ").c_str(), color);
        }
        else if (nullptr != param.rangefp) {
            append_render_text(render_text_listv, param.name + 2, normal);
            char buf[128];
            std::snprintf(buf, sizeof(buf), "%.*f", (int)-log10(param.ref->stepf), *param.rangefp);
            append_render_text(render_text_listv, std::string(buf).append(" ").c_str(), color);
        }
    }
}

// メイン

int sample_all(void) {
    std::cout << "start sample05" << std::endl;
    atexit(atexit_function);

    // GLFW 初期化
    glfwSetErrorCallback(glfw_error_callback);
    if (GL_FALSE == glfwInit()) {
        std::cerr << "!glfwInit()" << std::endl;
        return 1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
#ifdef __APPLE__
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
#else
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
#endif
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
    GLFWwindow* const window = glfwCreateWindow(1280, 720, "sample", NULL, NULL);
    if (nullptr == window) {
        std::cerr << "!glfwCreateWindow()" << std::endl;
        glfwTerminate();
        return 1;
    }
    glfwSetKeyCallback(window, glfw_key_callback);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(interval); // バッファー切り替え間隔

    // OpenGL 初期化
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "!gladLoadGLLoader()" << std::endl;
        glfwTerminate();
        return 1;
    }
    if (NULL != glDebugMessageCallback) {
        glEnable(GL_DEBUG_OUTPUT);
        glDebugMessageCallback(gl_debug_message_callback, 0);
    }
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glEnable(GL_DEPTH_TEST); // デプステストを有効にする
    glDepthFunc(GL_LESS); // 前のものよりもカメラに近ければ、フラグメントを受け入れる
    glProvokingVertex(GL_FIRST_VERTEX_CONVENTION);

    // モデル作成。
    create_triangle_model(models[0]);
    create_asset_model(models[1], "assets/cube.obj", "assets/uvmap.png");
    create_asset_model(models[2], "assets/bunny.obj");
    create_earth_model(models[3]);
    create_rock_model(models[4]);

    // シェーダー作成。
    const GLuint program = glCreateProgram();
    const GLchar* shader_src[] = { vertex_shader_src.c_str(), fragment_shader_src.c_str() };
    create_shader("vertex shader", program, GL_VERTEX_SHADER, shader_src);
    create_shader("fragment shader", program, GL_FRAGMENT_SHADER, shader_src + 1);
    glLinkProgram(program);
    glUseProgram(program);
    // UBO 作成。
    GLuint vertex_uniform_buffer = create_uniform_buffer(sizeof(vertex_uniform), program, "vertex_uniform", vertex_uniform_binding);
    GLuint light_uniform_buffer = create_uniform_buffer(sizeof(vertex_uniform), program, "light_uniform", light_uniform_binding);
    // テキスト描画初期化。
    init_render_text();
    // エラー判定
    GLenum error = GL_GET_ERRORS();
    assert(GL_NO_ERROR == error && "main: glGetError();");
    if (GL_NO_ERROR != error) {
        exit(1);
    }

    // メインループ
    int frame_count = 0;
    double start_time = glfwGetTime(), prev_time = start_time, time = 0.0;
    clock_t start_time2 = clock(), prev_time2 = start_time2, time2 = 0;
    while (GL_FALSE == glfwWindowShouldClose(window)) {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // パラメーター変更。
        Model& model = models[model_idx];
        int interval_prev = interval;
        change_params(model);
        // モデル変更。
        translate_model(model);
        // モデル変換。
        float ratio = (float)width / height;
        transform_model(model, (float)time, ratio);
        // シェーダー設定
        glUseProgram(program);
        glBindBuffer(GL_UNIFORM_BUFFER, vertex_uniform_buffer);
        GLvoid* buf = glMapBufferRange(GL_UNIFORM_BUFFER, 0, sizeof(vertex_uniform), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
        memcpy(buf, &vertex_uniform, sizeof(vertex_uniform));
        glUnmapBuffer(GL_UNIFORM_BUFFER);
        glBindBuffer(GL_UNIFORM_BUFFER, light_uniform_buffer);
        buf = glMapBufferRange(GL_UNIFORM_BUFFER, 0, sizeof(light_uniform), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
        memcpy(buf, &light_uniform, sizeof(light_uniform));
        glUnmapBuffer(GL_UNIFORM_BUFFER);
        // テクスチャー設定
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, model.texture);

        // モデル描画
        glBindVertexArray(model.vertex_array);
        if (model.fragment1_count >= 1) {
            glDrawArrays(GL_TRIANGLES, 0, (GLsizei)model.fragment1_count);
        }
        if (model.fragment2_count >= 1) {
            glDrawArrays(GL_LINES, (GLint)(model.fragments_count - model.fragment2_count), (GLsizei)model.fragment2_count);
        }
        if (!(model.fragment1_count >= 1 || model.fragment2_count >= 1)) {
            if (0 == model.element_buffer) {
                glDrawArrays(GL_TRIANGLES, 0, (GLsizei)model.verts_count);
            }
            else {
                glDrawElements(GL_TRIANGLES, (GLsizei)model.polys_count, GL_UNSIGNED_INT, NULL);
            }
        }

        // パラメーター表示
        float y = 580.0f;
        if (frame_count >= 1) {
            std::stringstream st1, st2;
            st1 << "ALL  " << std::fixed << std::setprecision(2) << (frame_count / (time - start_time)) << "fps " << (1000.0 * (time - start_time) / frame_count) << "ms "
                << "CPU " << (frame_count * CLOCKS_PER_SEC / (time2 - start_time2)) << "fps " << (1000.0 * ((double)time2 - start_time2) / CLOCKS_PER_SEC / frame_count) << "ms "
                << "TOTAL " << time << "s " << frame_count << "f";
            render_text(st1.str().c_str(), 0.0f, y, 0.2f, vec3{ 1.0f, 1.0f, 1.0f }, true);
            y -= 15.0f;
            st2 << "CUR " << std::fixed << std::setprecision(2) << (1 / (time - prev_time)) << "fps " << (1000.0 * (time - prev_time) / 1) << "ms "
                << "CPU " << (1 * CLOCKS_PER_SEC / (time2 - prev_time2)) << "fps " << (1000.0 * ((double)time2 - prev_time2) / CLOCKS_PER_SEC / 1) << "ms "
                ;
            render_text(st2.str().c_str(), 0.0f, y, 0.2f, vec3{ 1.0f, 1.0f, 1.0f }, true);
            y -= 5.0f;
        }
        render_text_list(render_text_listv, 0.0f, y, 0.18f, true);
        
        // 描画反映
        glfwSwapBuffers(window);
        glfwPollEvents();
        // 計測用
        ++frame_count;
        prev_time = time;
        prev_time2 = time2;
        time = glfwGetTime();
        time2 = clock();
        if (interval != interval_prev) {
            glfwSwapInterval(interval); // バッファー切り替え間隔
            frame_count = 0;
            start_time = time;
            start_time2 = time2;
        }
    }
    time = glfwGetTime() - start_time;
    std::cout << "ALL " << std::fixed << std::setprecision(2) << (frame_count / (time - start_time)) << "fps " << (1000.0 * (time - start_time) / frame_count) << "ms "
        << "CPU " << (frame_count * CLOCKS_PER_SEC / (time2 - start_time2)) << "fps " << (1000.0 * (time2 - start_time2) / CLOCKS_PER_SEC / frame_count) << "ms "
        << "TOTAL " << time << "s " << frame_count << "f";
    std::cout << std::endl;
    return 0;
}
#endif
