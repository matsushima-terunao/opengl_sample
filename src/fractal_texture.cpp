/*
 * フラクタルで地形テクスチャー生成
 *
 * @author matsushima
 * @since 2021/04/23
 */

#include <iostream>
#define _USE_MATH_DEFINES
#include <math.h>
#include <time.h>

static double gauss(int n_rand, double gauss_add, double gauss_fac) {
    double sum = 0;
    int i;
    for (i = 0; i < n_rand; ++i) {
        sum += (double)rand() / RAND_MAX;
    }
    return gauss_fac * sum - gauss_add;
}

/**
 * 1次元高速フーリエ変換。
 */
static void fft1(double* buf1_real, double* buf1_imag, double* tmp1_real, double* tmp1_imag, int width_bits) {
    int width = 1 << width_bits;
    int i, j, x, y;
    for (i = 1, j = width / 2; i <= width / 2; i *= 2, j /= 2) {
        for (x = 0; x < j; ++x) {
            double c = cos(2 * M_PI * i * x / width);
            double s = sin(2 * M_PI * i * x / width);
            for (y = 0; y < i; ++y) {
                double a_real = buf1_real[x + j * 2 * y];
                double a_imag = buf1_imag[x + j * 2 * y];
                double b_real = buf1_real[x + j * 2 * y + j];
                double b_imag = buf1_imag[x + j * 2 * y + j];
                buf1_real[x + j * 2 * y] = a_real + b_real;
                buf1_imag[x + j * 2 * y] = a_imag + b_imag;
                buf1_real[x + j * 2 * y + j] = (a_real - b_real) * c - (a_imag - b_imag) * s;
                buf1_imag[x + j * 2 * y + j] = (a_imag - b_imag) * c + (a_real - b_real) * s;
            }
        }
    }
    for (i = 0; i < width; ++i) {
        int k = 0;
        for (j = 0; j < width_bits; j++) {
            k = (k << 1) | ((i >> j) & 1);
        }
        tmp1_real[i] = buf1_real[k];
        tmp1_imag[i] = buf1_imag[k];
    }
    for (i = 0; i < width; ++i) {
        buf1_real[i] = tmp1_real[i] / width;
        buf1_imag[i] = tmp1_imag[i] / width;
    }
}

/**
 * 2次元高速フーリエ変換。
 */
static void fft2(double* buf_real, double* buf_imag, double* tmp_real, double* tmp_imag, int width_bits) {
    int width = 1 << width_bits;
    double* buf1_real = new double[width];
    double* buf1_imag = new double[width];
    double* tmp1_real = new double[width];
    double* tmp1_imag = new double[width];
    int x, y;
    for (x = 0; x < width; ++x) {
        for (y = 0; y < width; ++y) {
            buf1_real[y] = buf_real[x + y * width];
            buf1_imag[y] = buf_imag[x + y * width];
        }
        fft1(buf1_real, buf1_imag, tmp1_real, tmp1_imag, width_bits);
        for (y = 0; y < width; ++y) {
            tmp_real[x + y * width] = buf1_real[y];
            tmp_imag[x + y * width] = buf1_imag[y];
        }
    }
    for (y = 0; y < width; ++y) {
        for (x = 0; x < width; ++x) {
            buf1_real[x] = tmp_real[x + y * width];
            buf1_imag[x] = tmp_imag[x + y * width];
        }
        fft1(buf1_real, buf1_imag, tmp1_real, tmp1_imag, width_bits);
        for (x = 0; x < width; ++x) {
            buf_real[x + y * width] = buf1_real[x];
            buf_imag[x + y * width] = buf1_imag[x];
        }
    }
    delete[] buf1_real;
    delete[] buf1_imag;
    delete[] tmp1_real;
    delete[] tmp1_imag;
}

/**
 * フラクタルのビットマップ作成。
 */
float* create_fractal(int width_bits, double h) {
    std::cout << "< create_fractal:width_bits=" << width_bits << "(" << (1 << width_bits) << ")" << std::endl;
    size_t width = 1ull << width_bits;
    int x, y;
    double* buf_real = new double[width * width];
    double* buf_imag = new double[width * width];
    double* tmp_real = new double[width * width];
    double* tmp_imag = new double[width * width];
    double phase, elem;
    int YY = 64;
    // generate gauss
    int n_rand = 4;
    double gauss_add = sqrt(3 * n_rand);
    double gauss_fac = 2 * gauss_add / n_rand;
    for (x = 0; x < width * width; ++x) {
        buf_real[x] = 0;
        buf_imag[x] = 0;
    }
    for (x = 0; x <= width / 2; ++x) {
        for (y = 0; y <= width / 2; ++y) {
            phase = 2.0 * M_PI * rand() / RAND_MAX;
            elem = (0 == x || 0 == y) ? 0 : pow((x * x) + (y * y), -(h + 1) / 2) * gauss(n_rand, gauss_add, gauss_fac);
            double c = elem * cos(phase);
            double s = elem * sin(phase);
            if ((x <= YY) && (y <= YY)) {
                buf_real[x + y * width] = c;
                buf_imag[x + y * width] = s;
                size_t x2 = (0 == x) ? 0 : width - x;
                size_t y2 = (0 == y) ? 0 : width - y;
                buf_real[x2 + y2 * width] = c;
                buf_imag[x2 + y2 * width] = -s;
            }
        }
    }
    for (x = 1; x < width / 2; ++x) {
        for (y = 1; y < width / 2; ++y) {
            phase = 2.0 * M_PI * rand() / RAND_MAX;
            elem = pow((x * x) + (y * y), -(h + 1) / 2) * gauss(n_rand, gauss_add, gauss_fac);
            double c = elem * cos(phase);
            double s = elem * sin(phase);
            if ((x < YY) && (y < YY)) {
                buf_real[x + (width - y) * width] = c;
                buf_imag[x + (width - y) * width] = s;
                buf_real[(width - x) + y * width] = c;
                buf_imag[(width - x) + y * width] = -s;
            }
        }
    }
    // FFT
    fft2(buf_real, buf_imag, tmp_real, tmp_imag, width_bits);
    // output
    double elem_max = 0, elem_min = 0;
    for (x = 0; x < width; ++x) {
        for (y = 0; y < width; ++y) {
            double elem_real = buf_real[x + y * width];
            //double elem_imag = buf_imag[x + y * n];
            //double elem = Math.sqrt(elem_real * elem_real + elem_imag * elem_imag);
            //double elem = Math.pow(elem_real, 1.1);
            elem = elem_real;
            if (0 == x && 0 == y) {
                elem_max = elem_min = 0;
            } else {
                elem_max = std::max(elem_max, elem);
                elem_min = std::min(elem_min, elem);
            }
            buf_real[x + y * width] = elem;
        }
    }
    float* buf = new float[width * width];
    for (x = 0; x < width; ++x) {
        for (y = 0; y < width; ++y) {
            elem = buf_real[x + y * width];
            elem = (elem - elem_min) / (elem_max - elem_min);
            buf[x + y * width] = (float)elem;
        }
    }
    delete[] buf_real;
    delete[] buf_imag;
    delete[] tmp_real;
    delete[] tmp_imag;
    std::cout << "> create_fractal:elem_max=" << elem_max << ",elem_min=" << elem_min << std::endl;
    return buf;
}
