/*
 * メッシュ
 *
 * @author matsushima
 * @since 2021/07/13
 */
/*
start
<read_mesh:assets/bunny.obj
v=104451,vt=0,vn=104451,f=69630
>read_mesh:time1=0.031,time2=1.04,time3=0.215
<create_fractal_image:width_bits=9(512)
elem_max=1.66635e-05,elem_min=-1.88058e-05
elem_max=2.66137e-05,elem_min=-2.06867e-05
>create_fractal_image:time1=2.131
<create_sphere_model:unit=9,width=2,height=2
verts=3894,p=3894,div_a=36
polys=1896,p=1896,div_a=36
>create_sphere_model:time1=2.549
ALL -27.44fps -36.44ms CPU 51fps 19.58ms TOTAL 0.84s 43f
*/

#include "mesh.hpp"
#include <iostream>
#include <list>
#include <unordered_map>
#include <assert.h>

/**
 * ファイル読み込み。
 */
static bool read_file(const char* path, std::vector<char>& buf) {
#ifdef _WIN64
    FILE* fp = NULL;
    fopen_s(&fp, path, "rb");
#else
    FILE* fp = fopen(path, "rb");
#endif
    if (!fp) {
        assert(fp && "fopen(path, \"rb\");");
        std::cerr << "fopen(path, \"rb\"); path=" << path << std::endl;
        return false;
    }
    fseek(fp, 0, SEEK_END);
    //size_t size = ftell(fp);
    fpos_t size;
    fgetpos(fp, &size);
    fseek(fp, 0, SEEK_SET);
    buf.resize(size + 1);
    fread(buf.data(), 1, size, fp);
    fclose(fp);
    buf[size] = 0;
    return true;
}

/**
 * カラム取得。
 */
static bool get_col(const char*& s1, const char*& s2) {
#define DELIM_COL " \t"
#define DELIM_LINE "\r\n"
    s1 = s2 + strspn(s2, DELIM_COL);
    s2 = s1 + strspn(s1, DELIM_LINE);
    if (s1 < s2) {
        return false;
    }
    s2 = s1 + strcspn(s1, DELIM_COL DELIM_LINE);
    return s1 < s2;
}

/**
 * カラム取得2。
 */
static bool get_col2(const char*& s1, const char*& s2, char delim) {
    s1 = (delim == *s2) ? s2 + 1 : s2;
    s2 = strchr(s1, delim);
    if (!s2) {
        s2 = s1 + strlen(s1);
    }
    return s1 < s2;
}

/**
 * メッシュ読み込み。
 */
void read_mesh(const char* path, std::vector<Vertex>& vertex_list) {
    std::cout << "<read_mesh:" << path << std::endl;
    clock_t time1 = clock();

    // ファイル読み込み
    std::vector<char> buf;
    if (!read_file(path, buf)) {
        return;
    }
    time1 = clock() - time1;
    clock_t time2 = clock();

    // ファイル解析
    //v 1.000000 -1.000000 -1.000000
    //vt 0.748573 0.750412
    //vn 0.000000 0.000000 -1.000000
    //usemtl Material_ray.png
    //s off
    //f 5/1/1 1/2/1 4/3/1
    std::unordered_map<std::string, std::list<std::vector<const char*>>> entity;
    std::unordered_map<std::string, std::vector<float>> entityf;
    std::vector<const char*> cols;
    for (const char* s1, *s2 = buf.data(); get_col(s1, s2); ) {
        std::string key(s1, s2);
        if ("v" == key || "vt" == key || "vn" == key) {
            auto& entityf_vx = entityf[key];
            while (get_col(s1, s2)) {
                entityf_vx.emplace_back((float)atof(s1));
            }
        }
        else {
            cols.reserve(4);
            while (get_col(s1, s2)) {
                cols.emplace_back(s1);
            }
            if (!cols.empty()) {
                entity[key].emplace_back(std::move(cols));
            }
        }
    }
    time2 = clock() - time2;
    std::cout << "v=" << entityf["v"].size() << ",vt=" << entityf["vt"].size() << ",vn=" << entityf["vn"].size() << ",f=" << entity["f"].size() << std::endl;
    clock_t time3 = clock();

    // ポリゴン追加
    //f v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3
    //f 5/1/1 1/2/1 4/3/1
    static const float dummy[] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, };
    vertex_list.reserve(entity["f"].size() * 3);
    int c = 0;
    auto& entityf_v = entityf["v"];
    auto& entityf_vt = entityf["vt"];
    auto& entityf_vn = entityf["vn"];
    for (auto& f : entity["f"]) {
        ++c;
        for (size_t i = 0; i < 3; ++i) {
            const char* s1, * s2 = f[i];
            get_col2(s1, s2, '/');
            auto* v = &entityf_v[3 * (atoll(s1) - 1)];
            auto* vt = !get_col2(s1, s2, '/') || entityf_vt.empty() ? dummy : &entityf_vt[2 * (atoll(s1) - 1)];
            auto* vn = entityf_vn.empty() ? v : &entityf_vn[3 * (atoll(s2 + 1) - 1)];
            float a = vn[2] / sqrt(vn[0] * vn[0] + vn[1] * vn[1] + vn[2] * vn[2]); // dummy:(0,0,1)との角度
            vertex_list.emplace_back(Vertex{
                .x = v[0], .y = v[1], .z = v[2],
                //.r = (c & 1) ? 1.0f : 0.f, .g = (c & 2) ? 1.0f : 0.f, .b = (c & 4) ? 1.0f : 0.f,
                .r = a, .g = a, .b = a,
                .u = vt[0], .v = -vt[1],
                .nx = vn[0], .ny = vn[1], .nz = vn[2],
                });
        }
    }
    time3 = clock() - time3;
    std::cout << ">read_mesh:time1=" << ((double)time1 / CLOCKS_PER_SEC) << ",time2=" << ((double)time2 / CLOCKS_PER_SEC) << ",time3=" << ((double)time3 / CLOCKS_PER_SEC) << std::endl;
    //>read_mesh:time1=0.015,time2=0.968,time3=0.175
    //>read_mesh:time1=0.007,time2=0.948,time3=0.2
}
