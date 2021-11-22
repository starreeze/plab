#ifndef RGB_PROC_H
#define RGB_PROC_H
#include <QRgb>
#include <cmath>
#include <algorithm>
#include "progbar.h"
typedef unsigned char Byte;
constexpr int fragsize = 128;
#define PIX img.pixel(j, i)

/************************ helper func **************************/

inline double norDist(double x) {
    //a function similar to normal distribution
    return exp(-x * x / 3072);
}

inline Byte A(unsigned argb) {
    return argb >> 24;
}

inline Byte R(unsigned argb) {
    return (argb >> 16) & 0xff;
}

inline Byte G(unsigned argb) {
    return (argb >> 8) & 0xff;
}

inline Byte B(unsigned argb) {
    return argb & 0xff;
}

inline QRgb bias(QRgb col) {
    Byte m = std::min({ R(col), G(col), B(col) });
    return qRgb(R(col) - m, G(col) - m, B(col) - m);
}

inline bool rgble(QRgb x, QRgb y, double scale = 1) {
    //x less or equal scale*y in all r,g,b values
    return R(x) <= scale * R(y) && G(x) <= scale * G(y) && B(x) <= scale * B(y);
}

inline bool biasle(QRgb x, int val) {
    auto b = bias(x);
    return max({ R(b), G(b), B(b) }) <= val;
}

inline Byte ByteCut(double val) {
    return val < 0 ? 0 : (val > 0xff ? 0xff : (Byte)val);
}

QRgb rgbPlus(QRgb x, QRgb y, double scale = 1) {
    //return x + scale*y, minus can be specified with a negative scale
    return qRgb(ByteCut(R(x) + scale * R(y)), ByteCut(G(x) + scale * G(y)), ByteCut(B(x) + scale * B(y)));
}

QRgb rgbMerge(QRgb x, QRgb y, double factor) {
    //return x + factor*y - factor*averge(y)
    QRgb t = rgbPlus(x, y, factor);
    Byte ave = (R(y) + G(y) + B(y)) / 3;
    return qRgb(ByteCut(R(t) - ave * factor), ByteCut(G(t) - ave * factor), ByteCut(B(t) - ave * factor));
}

QRgb enrichColor(QRgb x, double factor) {
    //return x + factor*bias(x) - factor*averge(bias(x))
    return rgbMerge(x, bias(x), factor);
}

QRgb adjustContrast(QRgb x, QRgb ave, double factor) {
    //set difference from ave according to factor
    double t = max({ factor * norDist(R(x) - R(ave)), factor * norDist(G(x) - G(ave)), factor * norDist(B(x) - B(ave)) });
    return qRgb(ByteCut(R(x) + t), ByteCut(G(x) + t), ByteCut(B(x) + t));
}

QRgb text2Gray(QRgb x) {
    // minimum r/g/b value - e.g. words wirtten in red ink should be regarded as a full mark
    Byte m = min({ R(x), G(x), B(x) });
    return qRgb(m, m, m);
}

QRgb pic2Gray(QRgb x) {
    Byte m = (R(x) + G(x) + B(x)) / 3;
    return qRgb(m, m, m);
}

QRgb changeBrightness(QRgb x, double factor) {
    return qRgb(R(x) * factor, G(x) * factor, B(x) * factor);
}

QRgb rgbAve(const QImage& img, int fraga, int fragb, int aend, int bend) {
    //average rgb value in a fragment
    double r = 0, g = 0, b = 0;
    for (int i = fragsize * fraga; i < aend; ++i)
        for (int j = fragsize * fragb; j < bend; ++j) {
            unsigned rgb = PIX;
            r += R(rgb);
            g += G(rgb);
            b += B(rgb);
        }
    int pixels = (aend - fragsize * fraga) * (bend - fragsize * fragb);
    r /= pixels;
    g /= pixels;
    b /= pixels;
    return qRgb(r, g, b);
}

//************************ algorithm for t&f sharpening **************************

void tfsharpen(QImage& img, int intensity) {
    for (int i = 0; i < img.height(); ++i)
        for (int j = 0; j < img.width(); ++j)
            img.setPixel(j, i, text2Gray(PIX));
    int width_upper_limit = 12 + 4 * intensity, width_lower_limit = 5 - intensity, color_upper_limit = 255, color_lower_limit = 0;
    // horizontal scan
    // -----(white)-------*****(black)******------(white)--------
    // j              blackBegin        blackEnd                k
    for (int i = 0; i < img.height(); ++i)
        for (int j = 0; j < img.width(); ++j) {
            int k = j, blackSum = 0;
            if (B(img.pixel(k, i)) < color_upper_limit && B(img.pixel(k, i)) > color_lower_limit)
                for (; k < img.width() && B(img.pixel(k, i)) < color_upper_limit; ++k)
                    blackSum += B(img.pixel(k, i));
            int length = k - j;
            if (length < width_upper_limit && length > width_lower_limit) {
                int blackLen = max(2, length - blackSum / 255), blackBegin = j + (length - blackLen) / 2, blackEnd = blackBegin + blackLen;
                for (; j < blackBegin; ++j)    img.setPixel(j, i, qRgb(255, 255, 255));
                for (; j < blackEnd; ++j)    img.setPixel(j, i, qRgb(0, 0, 0));
                for (; j < k; ++j)    img.setPixel(j, i, qRgb(255, 255, 255));
            }
            else    j = k;
        }
    // vertical scan
    for (int j = 0; j < img.width(); ++j)
        for (int i = 0; i < img.height(); ++i) {
            int k = i, blackSum = 0;
            if (B(img.pixel(j, k)) < color_upper_limit && B(img.pixel(j, k)) > color_lower_limit)
                for (; k < img.height() && B(img.pixel(j, k)) < color_upper_limit; ++k)
                    blackSum += B(img.pixel(j, k));
            int length = k - i;
            if (length < width_upper_limit && length > width_lower_limit) {
                int blackLen = max(2, length - blackSum / 255), blackBegin = i + (length - blackLen) / 2, blackEnd = blackBegin + blackLen;
                for (; i < blackBegin; ++i)    img.setPixel(j, i, qRgb(255, 255, 255));
                for (; i < blackEnd; ++i)    img.setPixel(j, i, qRgb(0, 0, 0));
                for (; i < k; ++i)    img.setPixel(j, i, qRgb(255, 255, 255));
            }
            else    i = k;
        }
}

/************************* algorithm for text enhance ****************************
(1) regional color correction: split the picture to multiple fragment of 128 and minus
 average rgb for each of them to slove the problem of partial illumination;
(2) contrast enhance: add rgb if it is higher than average, the more close to average
 the more it should be added; same to the contrary. Here the Normal Distrbution
 function is used;
(3) black and white recognition: gray - rgb higher than c1 * average is set to white
 and rgb lower than c2 * average is set to black;
(4) color enhence: enrich colors if set to colorful. */

#define FOR_EACH_PIXEL                                      \
    QRgb                                                    \
        ave = rgbAve(img, 0, 0, img.height(), img.width()); \
    for (int i = 0; i < img.height(); ++i)                  \
        for (int j = 0; j < img.width(); ++j)

#define FOR_EACH_PIXEL_IN_FRAG                \
    QRgb ave = rgbAve(img, a, b, aend, bend); \
    for (int i = fragsize * a; i < aend; ++i) \
        for (int j = fragsize * b; j < bend; ++j)

void rcc(QImage& img, int fragsize, int a, int b, int aend, int bend) {
    FOR_EACH_PIXEL_IN_FRAG{
        QRgb t = rgbMerge(PIX, bias(ave), -1);
        img.setPixel(j, i, t);
    }
}

void cte_full(QImage& img, int intense) {
    FOR_EACH_PIXEL
        img.setPixel(j, i, adjustContrast(PIX, ave, 32 + 32 * intense));
}

void cte_frag(QImage& img, int intense, int fragsize, int a, int b, int aend, int bend) {
    FOR_EACH_PIXEL_IN_FRAG
        img.setPixel(j, i, adjustContrast(PIX, ave, 64 + 32 * intense));
}

void bwr_full(QImage& img, int intense) {
    FOR_EACH_PIXEL{
        // set to white
        if (rgble(ave, PIX, 0.2 + 0.1 * intense) && biasle(PIX, 3 * intense))
            img.setPixel(j, i, PIX | 0x00ffffff);
        // set to black
        else if (
            rgble(PIX, ave, 0.2 + 0.1 * intense) &&
            rgble(PIX, qRgb(15 + 15 * intense, 15 + 15 * intense, 15 + 15 * intense)) &&
            biasle(PIX, 5 * intense))
            img.setPixel(j, i, PIX & 0xff000000);
    }
}

void bwr_frag(QImage& img, int intense, bool gray, int fragsize, int a, int b, int aend, int bend) {
    FOR_EACH_PIXEL_IN_FRAG{
        // set to white
        if (rgble(ave, PIX, 0.8 + 0.1 * intense + 0.2 * gray) && biasle(PIX, 10 * intense))
            img.setPixel(j, i, PIX | 0x00ffffff);
        // set to black
        else if (
            rgble(PIX, ave, 0.5 + 0.2 * intense + 0.6 * gray) &&
            rgble(PIX, qRgb(30 + 30 * intense + 100 * gray, 30 + 30 * intense + 100 * gray, 30 + 30 * intense + 100 * gray)) &&
            biasle(PIX, 5 * intense))
            img.setPixel(j, i, PIX & 0xff000000);
    }
}

void text_enhance_pure(QImage& img, int intense, bool gray, int fragsize, string name) {
    ProgBar pbar(name, img.height() * img.width());
    for (int a = 0; fragsize * a < img.height(); ++a)
        for (int b = 0; fragsize * b < img.width(); ++b) {
            int aend = fragsize * (a + 1) > img.height() ? img.height() : fragsize * (a + 1),
                bend = fragsize * (b + 1) > img.width() ? img.width() : fragsize * (b + 1);
            pbar.setVal((a * img.width() + b * fragsize) * fragsize);
            pbar.print();
            for (int i = fragsize * a; i < aend; ++i)
                for (int j = fragsize * b; j < bend; ++j)
                    if (gray)
                        img.setPixel(j, i, text2Gray(PIX));
            rcc(img, fragsize, a, b, aend, bend);
            cte_frag(img, intense, fragsize, a, b, aend, bend);
            bwr_frag(img, intense, gray, fragsize, a, b, aend, bend);
        }
}

void text_enhance_mixed(QImage& img, int intense, bool gray) {
    for (int i = 0; i < img.height(); ++i)
        for (int j = 0; j < img.width(); ++j)
            if (gray)
                img.setPixel(j, i, text2Gray(PIX));
    cte_full(img, intense);
    bwr_full(img, intense);
}

void cle(QImage& img, double scale) {
    for (int i = 0; i < img.height(); ++i)
        for (int j = 0; j < img.width(); ++j)
            img.setPixel(j, i, enrichColor(img.pixel(j, i), scale));
}

#endif // RGB_PROC_H
