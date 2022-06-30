//
//  image.hpp
//  opengl_sample
//
//  Created by matsushima on 2021/07/14.
//  Copyright © 2021 matsushima. All rights reserved.
//

#ifndef image_hpp
#define image_hpp

/**
 * テクスチャーイメージ読み込み。
 */
unsigned int read_image(const char* path, int cvt_color = 0);

/**
 * パレット作成。
 */
int* create_color_palette(int colors, int* color_table);

#endif /* image_hpp */
