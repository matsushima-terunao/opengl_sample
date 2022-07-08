/*
 * メッシュ
 *
 * @author matsushima
 * @since 2021/07/13
 */

#ifndef mesh_hpp
#define mesh_hpp

#include <vector>

/** 頂点情報 */
struct Vertex {
    float x, y, z;
    float r, g, b;
    float u, v;
    float nx, ny, nz;
};

/**
 * メッシュ読み込み。
 */
void read_mesh(const char* path, std::vector<Vertex>& vertex_list);

#endif /* mesh_hpp */
