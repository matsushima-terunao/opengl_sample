/*
 * モデル
 *
 * @author matsushima
 * @since 2021/07/20
 */

#include "model.hpp"
#include "linmath.h"
#include <memory.h>
#include <iostream>

static float randf(float min, float max) {
    return rand() * (max - min) / RAND_MAX + min;
}

/**
 * 破片データ作成。
 */
void create_fragment(Model& model) {
    constexpr int cnt_max11 = 100, cnt_max12 = 150, cnt_max21 = 50, cnt_max22 = 100;
    constexpr float vangle = 0.03f, vdistance = 10.0f, vangle2 = 0.03f, vdistance2 = 10.0f;
    size_t vertex_count = model.polys ? model.polys_count : model.verts_count;
    model.fragments1 = new Model::Fragment[vertex_count / 3];
    model.fragments2 = new Model::Fragment[vertex_count];
    memset(model.fragments1, 0, vertex_count / 3 * sizeof(Model::Fragment));
    memset(model.fragments2, 0, vertex_count * sizeof(Model::Fragment));
    model.fragments_count = vertex_count * 2;
    std::cout << "model.verts_count=" << model.verts_count << std::endl;
    std::cout << "> create_fragment(): vertex_count = " << vertex_count << ", model.fragments_count = " << model.fragments_count << std::endl;
    float* new_vertsf = new float[model.fragments_count * model.verts_stride / sizeof(float)];
    memcpy(new_vertsf, model.vertsf, model.verts_count * model.verts_stride);
    // 中心座標用に頂点座標を加算
    for (int i = 0; i < vertex_count; ++i) {
        size_t vi = (model.polys ? model.polys[i] : i) * model.verts_stride / sizeof(float);
        model.fragments1[i / 3].x += model.vertsf[vi + 0] / 3;
        model.fragments1[i / 3].y += model.vertsf[vi + 1] / 3;
        model.fragments1[i / 3].z += model.vertsf[vi + 2] / 3;
        int i2 = i - i % 3 + (i + 2) % 3; // 0-1 1-2 2-0
        model.fragments2[i].x += model.vertsf[vi + 0] / 2;
        model.fragments2[i].y += model.vertsf[vi + 1] / 2;
        model.fragments2[i].z += model.vertsf[vi + 2] / 2;
        model.fragments2[i2].x += model.vertsf[vi + 0] / 2;
        model.fragments2[i2].y += model.vertsf[vi + 1] / 2;
        model.fragments2[i2].z += model.vertsf[vi + 2] / 2;
    }
    // 移動・回転量
    for (int i = 0; i < vertex_count / 3; ++i) {
        vec3_norm(&model.fragments1[i].vx, &model.fragments1[i].x);
        model.fragments1[i].vx += randf(-1.0f, 1.0f) * vdistance;
        model.fragments1[i].vy += randf(-1.0f, 1.0f) * vdistance;
        model.fragments1[i].vz += randf(-1.0f, 1.0f) * vdistance;
        model.fragments1[i].va = randf(-1.0f, 1.0f) * vangle;
        model.fragments1[i].vb = randf(-1.0f, 1.0f) * vangle;
        model.fragments1[i].vc = randf(-1.0f, 1.0f) * vangle;
        model.fragments1[i].cnt = 0;
        model.fragments1[i].cnt_max = (int)randf(cnt_max11, cnt_max12);
    }
    for (int i = 0; i < vertex_count; ++i) {
        vec3_sub(&model.fragments2[i].vx, &model.fragments2[i].x, &model.fragments1[i / 3].x);
        vec3_norm(&model.fragments2[i].vx, &model.fragments2[i].x);
        model.fragments2[i].vx += randf(-1.0f, 1.0f) * vdistance2;
        model.fragments2[i].vy += randf(-1.0f, 1.0f) * vdistance2;
        model.fragments2[i].vz += randf(-1.0f, 1.0f) * vdistance2;
        model.fragments2[i].va = randf(-1.0f, 1.0f) * vangle2;
        model.fragments2[i].vb = randf(-1.0f, 1.0f) * vangle2;
        model.fragments2[i].vc = randf(-1.0f, 1.0f) * vangle2;
        model.fragments2[i].cnt = 0;
        model.fragments2[i].cnt_max = (int)randf(cnt_max21, cnt_max22);
    }
    model.vertsf = new_vertsf;
}

/**
 * 破片データ初期化。
 */
void init_fragment(Model& model, float* buf) {
    constexpr int cnt_max11 = 100, cnt_max12 = 150, cnt_max21 = 50, cnt_max22 = 100;
    size_t vertex_count = model.polys ? model.polys_count : model.verts_count;
    // 移動・回転量
    for (int i = 0; i < vertex_count / 3; ++i) {
        model.fragments1[i].cnt = 0;
        model.fragments1[i].cnt_max = (int)randf(cnt_max11, cnt_max12);
    }
    for (int i = 0; i < vertex_count; ++i) {
        model.fragments2[i].cnt = 0;
        model.fragments2[i].cnt_max = (int)randf(cnt_max21, cnt_max22);
    }
    // 初期データ
    model.fragment1_count = model.fragment2_count = 0;
    memcpy(buf, model.vertsf, model.verts_count * model.verts_stride);
}

/**
 * 頂点変更。
 */
static void translate_vertex(Model::Fragment* fragment, float* dst_vertex, float* src_vertex, float* angle, float* sin, float* cos) {
    float x = src_vertex[0];
    float y = src_vertex[1];
    float z = src_vertex[2];
    if (fragment) {
        // オフセット
        x -= fragment->x;
        y -= fragment->y;
        z -= fragment->z;
        // sin/cos 更新
        for (int i = 0; i < 3; ++i) {
            float a = fragment->cnt * (&fragment->va)[i];
            if (angle[i] != a) {
                angle[i] = a;
                sin[i] = sinf(a);
                cos[i] = cosf(a);
            }
        }
    }
    // x軸回転
    float x1 = x;
    float y1 = y * cos[0] - z * sin[0];
    float z1 = y * sin[0] + z * cos[0];
    // y軸回転
    float x2 = x1 * cos[1] + z1 * sin[1];
    float y2 = y1;
    float z2 = -x1 * sin[1] + z1 * cos[1];
    // z軸回転
    dst_vertex[0] = x2 * cos[2] - y2 * sin[2];
    dst_vertex[1] = x2 * sin[2] + y2 * cos[2];
    dst_vertex[2] = z2;
    if (fragment) {
        // オフセット + 移動
        dst_vertex[0] += fragment->x + fragment->vx * fragment->cnt;
        dst_vertex[1] += fragment->y + fragment->vy * fragment->cnt;
        dst_vertex[2] += fragment->z + fragment->vz * fragment->cnt;
    }
}

/**
 * 破片データ頂点変更。
 */
static void translate_fragment_vertex(Model::Fragment* fragment1, Model::Fragment* fragment2,
    Vertex& dst_vertex, Vertex& src_vertex, float* angle, float* sin, float* cos, float* angle2, float* sin2, float* cos2) {
    translate_vertex(fragment1, &dst_vertex.x, &src_vertex.x, angle, sin, cos);
    if (fragment2) {
        translate_vertex(fragment2, &dst_vertex.x, &dst_vertex.x, angle2, sin2, cos2);
    }
    dst_vertex.r = src_vertex.r;
    dst_vertex.g = src_vertex.g;
    dst_vertex.b = src_vertex.b;
    dst_vertex.u = src_vertex.u;
    dst_vertex.v = src_vertex.v;
    translate_vertex(nullptr, &dst_vertex.nx, &src_vertex.nx, angle, sin, cos);
    if (fragment2) {
        translate_vertex(nullptr, &dst_vertex.nx, &dst_vertex.nx, angle2, sin2, cos2);
    }
}

/**
 * 破片データ変更。
 */
void translate_vertex(Model& model, float* buf) {
    size_t stride_length = model.verts_stride / sizeof(float);
    size_t vertex_count = model.polys ? model.polys_count : model.verts_count;
    model.fragment1_count = model.fragment2_count = 0;
    float* buf1 = buf;
    float* buf2 = buf + model.fragments_count * stride_length;
    float angle1[3] = { 0.0f, 0.0f, 0.0f }, sin1[3] = { 0.0f, 0.0f, 0.0f }, cos1[3] = { 1.0f, 1.0f, 1.0f };
    float angle2[3] = { 0.0f, 0.0f, 0.0f }, sin2[3] = { 0.0f, 0.0f, 0.0f }, cos2[3] = { 1.0f, 1.0f, 1.0f };
    for (int i = 0; i < vertex_count; ++i) {
        size_t vi = (model.polys ? model.polys[i] : i) * stride_length;
        if (model.fragments1[i / 3].cnt < model.fragments1[i / 3].cnt_max) {
            // 破片1
            translate_fragment_vertex(&model.fragments1[i / 3], nullptr,
                *(Vertex*)buf1, *(Vertex*)&model.vertsf[vi], angle1, sin1, cos1, nullptr, nullptr, nullptr);
            model.fragments1[i / 3].cnt += (2 == i % 3);
            buf1 += stride_length;
            ++model.fragment1_count;
        } else if (model.fragments2[i].cnt < model.fragments2[i].cnt_max) {
            // 破片2
            for (int j = 0; j < 2; ++j) {
                buf2 -= stride_length;
                ++model.fragment2_count;
                size_t i2 = (0 == j) ? i : i - i % 3 + (i + 2) % 3; // 0-1 1-2 2-0
                size_t vi2 = (model.polys ? model.polys[i2] : i2) * stride_length;
                translate_fragment_vertex(&model.fragments2[i], &model.fragments1[i / 3],
                    *(Vertex*)buf2, *(Vertex*)&model.vertsf[vi2], angle2, sin2, cos2, angle1, sin1, cos1);
            }
            model.fragments1[i / 3].cnt += (2 == i % 3);
            ++model.fragments2[i].cnt;
        }
    }
}
