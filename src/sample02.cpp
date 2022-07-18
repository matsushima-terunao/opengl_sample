/*
 * OpenGLサンプル2 - コード整理
 * 
 * @author matsushima
 * @since 2021/06/24
 */

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
#undef inline
#include <iostream>
#include <vector>
#include <assert.h>

// シーン情報

/** 頂点情報 */
struct Vertex {
    float x, y;
    float r, g, b;
};

/** モデル情報 */
struct Model {
    float x, y, z;
    float a, b, c;
    float va, vb, vc;

    float* vertsf = nullptr;
    float* vertsf_center = nullptr;
    size_t verts_count, verts_stride;
    int* polys = nullptr;
    size_t polys_count;
    unsigned int vertex_array, vertex_buffer, element_buffer = 0;
};

/** モデル位置・角度。 */
static Model triangle_model = {
    0.0f, 0.0f, 0.0,  0.0f, 0.0f, 0.0f,  0.0f, 0.0f, 0.01f,
};
/** カメラ情報。 */
static vec3 camera_eye = { 0.0f, 0.0f, 1.0f }, camera_center = { 0.f, 0.f, 0.f }, camera_up = { 0.f, 1.0f, 0.f };

/** 正投影情報。 */
struct ortho { float left, right, bottom, top, near, far; };
/** 正投影情報。 */
static struct ortho ortho = { .0f, .0f, .0f, .0f, 0.1f, 100.0f };

// シェーダー情報

/**
 * 頂点情報 uniform 構造体定義。
 * @see vertex_shader_src: layout(std140) uniform vertex_uniform
 * @see create_uniform_buffer
 */
struct vertex_uniform {
    mat4x4 modelview_projection_matrix;
};
/** 頂点情報 uniform 現在データ。 */
static struct vertex_uniform vertex_uniform = {
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

/*
 * シェーダー uniform の　binding point。
 * @see create_uniform_buffer: glUniformBlockBinding(program, index, binding);
 * @see create_uniform_buffer: glBindBufferBase(GL_UNIFORM_BUFFER, binding, uniform_buffer);
 */
constexpr GLuint vertex_uniform_binding = 0;

/** バーテックスシェーダーのソースプログラム。 */
static const std::string vertex_shader_src = R"(
#version 410 core
/**
 * 頂点情報 uniform 構造体定義、現在データ。
 * @see struct vertex_uniform
 */
layout(std140) uniform vertex_uniform {
    mat4 modelview_projection_matrix;
};
layout (location = 0) in vec3 position; // x, y, z: 頂点座標
layout (location = 1) in vec3 color; // r, g, b: 頂点カラー
out vec4 vertex_color; // 頂点カラー

void main() {
    gl_Position = modelview_projection_matrix * vec4(position, 1.0); // 頂点座標
    vertex_color = vec4(color, 1.0);
}
)";

/** フラグメントシェーダーのソースプログラム。 */
static const std::string fragment_shader_src = R"(
#version 410 core
in vec4 vertex_color; // 頂点カラー
out vec4 fragment_color; // 出力ピクセルカラー

void main() {
    // 頂点カラー
    fragment_color = vertex_color;
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
glfw_error_callback(): 65543: WGL: Driver does not support OpenGL version 5.3
Assertion failed: 0 && "glfw_error_callback()", file C:\USR\src\blog\opengl_sample\vs\opengl_sample\opengl_sample\sample02.cpp, line 159
 */
static void glfw_error_callback(int error, const char* description) {
    std::cerr << "glfw_error_callback(): " << error << ": " << description << std::endl;
    assert(0 && "glfw_error_callback()");
}

/**
 * OpenGL エラー・デバッグメッセージのコールバック。
gl_debug_message_callback(): source = 33352 type = 824c(GL_DEBUG_TYPE_ERROR) id = 0 severity = 37190 length = 146 userParam = 0000000000000000
message = SHADER_ID_COMPILE error has been generated. GLSL compile failed for shader 2, "": ERROR: 0:3: 'corexxx' : unknown profile in #version directive
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
 * メッシュ作成。
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
    glVertexAttribPointer(position_location, 2, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(color_location);
    glVertexAttribPointer(color_location, 3, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(GLfloat)));

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
    glBufferData(GL_UNIFORM_BUFFER, size, NULL, GL_DYNAMIC_DRAW);
    GLuint index = glGetUniformBlockIndex(program, uniformBlockName);
    glUniformBlockBinding(program, index, binding);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    return uniform_buffer;
}

// モデル作成

/**
 * triangle
 */
static void create_triangle_model(Model& model) {
    // メッシュ作成。
    static float vertsf[] = {
         -0.6f, -0.4f, 1.f, 0.f, 0.f ,
          0.6f, -0.4f, 0.f, 1.f, 0.f ,
           0.f,  0.6f, 0.f, 0.f, 1.f
    };
    model.vertsf = vertsf;
    model.verts_count = sizeof(vertsf) / sizeof(Vertex);
    model.verts_stride = sizeof(Vertex);
    model.polys = nullptr;
    model.polys_count = 0;
    create_vertex_buffer(model.vertex_array, model.vertex_buffer, model.element_buffer,
        sizeof(vertsf), vertsf, sizeof(Vertex), 0, nullptr);
}

// 入力

/**
 * キー入力のコールバック。
 */
static void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (GLFW_KEY_ESCAPE == key && GLFW_PRESS == action) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

/**
 * パラメーター計算。
 */
static void calc_params(float time, float ratio) {
    // パラメーター変更
    // ビューパラメーター
    ortho.left = -ratio;
    ortho.right = ratio;
    ortho.bottom = -1.0;
    ortho.top = 1.0;
    // モデルパラメーター
    triangle_model.a += triangle_model.va;
    triangle_model.b += triangle_model.vb;
    triangle_model.c += triangle_model.vc;

    // 変換
    mat4x4 model_matrix, view_matrix, projection_matrix;
    // モデル変換
    mat4x4_identity(model_matrix);
    mat4x4_translate(model_matrix, triangle_model.x, triangle_model.y, triangle_model.z);
    mat4x4_rotate_X(model_matrix, model_matrix, triangle_model.a);
    mat4x4_rotate_Y(model_matrix, model_matrix, triangle_model.b);
    mat4x4_rotate_Z(model_matrix, model_matrix, triangle_model.c);
    // ビュー変換
    mat4x4_look_at(view_matrix, camera_eye, camera_center, camera_up);
    // プロジェクション変換
    mat4x4_ortho(projection_matrix, ortho.left, ortho.right, ortho.bottom, ortho.top, ortho.near, ortho.far);
    // MVP
    mat4x4_mul(vertex_uniform.modelview_projection_matrix, projection_matrix, view_matrix);
    mat4x4_mul(vertex_uniform.modelview_projection_matrix, vertex_uniform.modelview_projection_matrix, model_matrix);
}

// メイン

int main02(void) {
    std::cout << "start sample02" << std::endl;
    atexit(atexit_function);

    // GLFW 初期化
    glfwSetErrorCallback(glfw_error_callback); // エラー発生時のコールバック指定
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
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // MacOS で必須
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // Core Profile
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE); // デバッグモード
    GLFWwindow* const window = glfwCreateWindow(1280, 720, "sample", NULL, NULL); // ウィンドウ作成
    if (nullptr == window) {
        std::cerr << "!glfwCreateWindow()" << std::endl;
        glfwTerminate();
        exit(1);
    }
    glfwSetKeyCallback(window, glfw_key_callback); // キーコールバック指定
    glfwMakeContextCurrent(window); // 描画対象
    glfwSwapInterval(1); // バッファー切り替え間隔

    // OpenGL 初期化
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "!gladLoadGLLoader()" << std::endl;
        glfwTerminate();
        exit(1);
    }
    // デバッグ出力有効
    if (NULL != glDebugMessageCallback) {
        glEnable(GL_DEBUG_OUTPUT);
        glDebugMessageCallback(gl_debug_message_callback, 0);
    }
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // カラーバッファーをクリアする色
    glEnable(GL_DEPTH_TEST); // デプステストを有効にする
    glDepthFunc(GL_LESS); // 前のものよりもカメラに近ければ、フラグメントを受け入れる

    // モデル作成。
    create_triangle_model(triangle_model);

    // シェーダー作成。
    const GLuint program = glCreateProgram();
    const GLchar* shader_src[] = { vertex_shader_src.c_str(), fragment_shader_src.c_str() };
    create_shader("vertex shader", program, GL_VERTEX_SHADER, shader_src);
    create_shader("fragment shader", program, GL_FRAGMENT_SHADER, shader_src + 1);
    glLinkProgram(program);
    glUseProgram(program);
    // UBO 作成。
    GLuint uniform_buffer = create_uniform_buffer(sizeof(vertex_uniform), program, "vertex_uniform", vertex_uniform_binding);

    // エラー判定
    GLenum error = GL_GET_ERRORS();
    assert(GL_NO_ERROR == error && "main: glGetError();");
    if (GL_NO_ERROR != error) {
        exit(1);
    }

    // メインループ
    while (GL_FALSE == glfwWindowShouldClose(window)) {
        double time = glfwGetTime();
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // パラメーター計算。
        float ratio = (float)width / height;
        calc_params((float)time, ratio);
        // シェーダー設定
        glUseProgram(program);
        glBindBufferBase(GL_UNIFORM_BUFFER, vertex_uniform_binding, uniform_buffer);
        GLvoid* buf = glMapBufferRange(GL_UNIFORM_BUFFER, 0, sizeof(vertex_uniform), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
        memcpy(buf, &vertex_uniform, sizeof(vertex_uniform));
        glUnmapBuffer(GL_UNIFORM_BUFFER);

        // モデル描画
        // 描画
        glBindVertexArray(triangle_model.vertex_array);
        if (0 == triangle_model.element_buffer) {
            glDrawArrays(GL_TRIANGLES, 0, (GLsizei)triangle_model.verts_count);
        }
        else {
            glDrawElements(GL_TRIANGLES, (GLsizei)triangle_model.polys_count, GL_UNSIGNED_INT, NULL);
        }
        // 描画反映
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    return 0;
}
