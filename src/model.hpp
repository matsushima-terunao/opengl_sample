/*
 * モデル
 *
 * @author matsushima
 * @since 2021/07/20
 */

#ifndef model_hpp
#define model_hpp

#include "mesh.hpp"
#include <stddef.h>
#include <assert.h>

/** モデル情報 */
struct Model {
    float x, y, z;
    float a, b, c;
    float va, vb, vc;
    int cnt;
    bool fragmented = false;

    struct Fragment {
        float x, y, z;
        float a, b, c;
        float vx, vy, vz;
        float va, vb, vc;
        int cnt, cnt_max;
    };

    /** 頂点バッファー */
    float* vertsf = nullptr;
    float* vertsf_center = nullptr;
    size_t verts_count, verts_stride;
    int* polys = nullptr;
    size_t polys_count;
    struct Fragment* fragments1 = nullptr;
    struct Fragment* fragments2 = nullptr;
    size_t fragments_count;
    size_t fragment1_count, fragment2_count;
    std::vector<Vertex> vertex_list;
    unsigned int vertex_array, vertex_buffer, element_buffer = 0;

    /** テクスチャー */
    unsigned int texture;
    /** ピクセルデータ */
    int* pixels = nullptr;
    /** ピクセルのカラーパレット */
    int* palette = nullptr;
    /** ピクセル幅・奥行き(= 1 << texture_width_bits) */
    int width, height;
};

/**
 * 破片データ作成。
 */
void create_fragment(Model& model);

/*
 * 破片データ初期化。
 */
void init_fragment(Model& model, float* buf);

/**
 * 破片データ更新。
 */
void translate_vertex(Model& model, float* buf);

#endif /* model_hpp */
