/*
 * テクスチャーイメージ
 *
 * @author matsushima
 * @since 2021/07/14
 */

#include "image.hpp"
#ifdef __APPLE__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#pragma clang pop
#else
#pragma warning(push, 0)
#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#pragma warning(pop)
#endif

/**
 * テクスチャーイメージ読み込み。
 */
void read_image(const char* path, std::function<void(unsigned char* data, int width, int height)> callback) {
    cv::Mat img = cv::imread(path);
    callback(img.data, img.cols, img.rows);
}

/**
 * パレット作成。
 */
int* create_color_palette(int colors, int* color_table) {
    int* table = new int[colors];
    for (int index = 0, i = 0; i < colors; ++i) {
        if (i >= color_table[(index + 1) * 4 + 0]) {
            ++index;
        }
        int d0 = (int)i - color_table[index * 4 + 0];
        int d1 = (int)i - color_table[(index + 1) * 4 + 0];
        int d = (color_table[(index + 1) * 4 + 0] - color_table[index * 4 + 0]);
        int r = (d0 * color_table[(index + 1) * 4 + 1] - d1 * color_table[index * 4 + 1]) / d;
        int g = (d0 * color_table[(index + 1) * 4 + 2] - d1 * color_table[index * 4 + 2]) / d;
        int b = (d0 * color_table[(index + 1) * 4 + 3] - d1 * color_table[index * 4 + 3]) / d;
        table[i] = (255 << 24) + (r << 16) + (g << 8) + b;
    }
    return table;
}
