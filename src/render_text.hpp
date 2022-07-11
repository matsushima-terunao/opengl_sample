﻿/*
 * テキスト描画
 *
 * @author matsushima
 * @since 2021/04/27
 */

#ifndef render_text_h
#define render_text_h

#include <string>
#include <vector>

/**
 * テキスト描画初期化。
 */
int init_render_text();

/**
 * テキスト描画。
 */
float render_text(const char* text, float x, float y, float scale, float color[3], bool proportional);

/**
 * テキスト一覧描画。
 */
float render_text_list(std::vector<std::pair<std::string, float*>>& render_text_listv, float x, float y, float scale, bool proportional);

/**
 * テキスト追加。
 */
void append_render_text(std::vector<std::pair<std::string, float*>>& render_text_list, const char* text, float* color);

#endif /* render_text_h */
