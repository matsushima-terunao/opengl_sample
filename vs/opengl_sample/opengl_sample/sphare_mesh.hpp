//
//  sphare_mesh.hpp
//  opengl_sample
//
//  Created by matsushima on 2021/07/20.
//  Copyright © 2021 matsushima. All rights reserved.
//

#ifndef sphare_mesh_hpp
#define sphare_mesh_hpp

/**
 * 球体メッシュ作成。
 */
void create_sphere_mesh(
    int width, int height, int vert_height, int texture_width_bits, int surfaces, int* texture, const int* palette,
    int div_a0, int div_da, int div_b,
    float*& verts, size_t& verts_count, size_t& verts_stride, int*& polys, size_t& polys_count
);

#endif /* sphare_mesh_hpp */
