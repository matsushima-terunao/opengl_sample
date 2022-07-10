/*
 * テクスチャーイメージ
 *
 * @author matsushima
 * @since 2021/07/14
 */

#ifndef image_hpp
#define image_hpp

#include <functional>

/**
 * テクスチャーイメージ読み込み。
 */
void read_image(const char* path, std::function<void (unsigned char* data, int width, int height)> callback);

/**
 * パレット作成。
 */
int* create_color_palette(int colors, int* color_table);

#endif /* image_hpp */
