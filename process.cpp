#include "process.h"
#include "config.h"
#include "progbar.h"
#include "rgb_proc.h"
#include <iostream>
#include <array>
#include <QDir>
using namespace std;

#ifndef NDEBUG
void print_rgb(QRgb x) {
    cout << R(x) << ' ' << G(x) << ' ' << B(x) << '\n';
}
#endif
#define PARAM(i) opt.second[i].toInt()
#define REPORT_ILLEAGAL_PARAM(option)                    \
 {                                                    \
        cout << "Illegal parameter for option -" #option \
                ".\n";                                   \
        exit(-1);                                        \
    }
#define CHECK_PARAM(n, option)  \
    if (opt.second.size() != n) \
    REPORT_ILLEAGAL_PARAM(option)

/*
bool smooth(array<int, 256>& distr) {
    // smooth the distrbution and return whether it is smooth
    int count = 0;
    array<int, 256> origin = distr;
    for(unsigned i=1; i<distr.size()-1; ++i) {
        distr[i] = (origin[i-1]+origin[i]+origin[i+1]) / 3;
        if(distr[i] > distr[i-1] && distr[i] > distr[i+1])
            ++count;
    }
    return count < 3;
}

//void deNoise(QImage& img, int intens) {

//}

QImage toBitmap(const QImage& img, int intens) {
    //intense: 0, 1, 2, 3
    array<int, 256> distr; // distribution of grayscale
    distr.fill(0);
    for(int i=0; i<img.height(); ++i)    for(int j=0; j<img.width(); ++j) {
        QRgb x = img.pixel(j,i);
        ++distr[min( {R(x),G(x),B(x)})];
    }
    int i = 0;
    for(; i<20 && !smooth(distr); ++i);
    if(i == 20) {
        cout << "\nWarning: fail to smooth picture; unidentified behavior may occur.\n";
        return img;
    }
    unsigned first_idx = 0, second_idx = 0;
    for(unsigned i=1; i<distr.size()-1; ++i)
        if(distr[i] > distr[first_idx])
            first_idx = i;
    for(unsigned i=1; i<distr.size()-1; ++i)
        if(i != first_idx && distr[i] > distr[second_idx])
            second_idx = i;
    assert(distr[first_idx] > distr[second_idx]);
    QImage im(img);
    bool t = first_idx > second_idx;
    for(int i=0; i<img.height(); ++i)    for(int j=0; j<img.width(); ++j) {
        QRgb x = img.pixel(j,i);
        if((min( {R(x),G(x),B(x)}) < (first_idx + 8 * second_idx) / 9) ^ t)
            im.setPixel(j,i,qRgb(255,255,255));
        else    im.setPixel(j,i,qRgb(0,0,0));
    }
    //deNoise(im, intens);
    return im;
}
*/

void replace(QImage& img, QRgb lower, QRgb upper, QRgb target) {
    for (int i = 0; i < img.height(); ++i)
        for (int j = 0; j < img.width(); ++j) {
            QRgb t(img.pixel(j, i));
            if (rgble(lower, t) && rgble(t, upper))
                img.setPixel(j, i, target);
        }
}

void process(const Config& conf) {
    // some basic checkings
    QString format;
    bool alpha = false;
    QImage img = QImage(conf.getSrc()).convertToFormat(QImage::Format_ARGB32);
#ifndef NDEBUG
    cout << "Src: " << conf.getSrc().toUtf8().data() << '\n'
        << "Dst: "
        << conf.getDst().toUtf8().data() << '\n'
        << "format: " << img.format() << '\n';
    cout << "Size: (" << img.width() << ", " << img.height() << ")\n";
#endif
    if (img.format() == QImage::Format_Invalid) {
        cout << "Fail to open \'" << conf.getSrc().toUtf8().data() << "\' as image.\n";
        return;
    }

    // check whether alpha channel is enabled
    for (int i = 0; !alpha && i < img.height(); ++i)
        for (int j = 0; j < img.width(); ++j)
            if ((A(img.pixel(j, i))) != 0xff) {
                alpha = true;
                break;
            }

    // handle options sequencially
    for (const auto& opt : conf.getOpt()) {
        switch (opt.first) {
        case 'c': //crop
            CHECK_PARAM(4, c)
                img = img.copy(PARAM(0), PARAM(1),
                PARAM(2), PARAM(3));
            break;
        case 's': //scale
            CHECK_PARAM(2, s)
                img = img.scaled(PARAM(0), PARAM(1));
            break;
        case 'm': //mirror
            CHECK_PARAM(1, m)
                if (opt.second[0] == "horizontal")
                    img = img.mirrored();
                else if (opt.second[0] == "vertical")
                    img = img.mirrored(true);
                else
                    REPORT_ILLEAGAL_PARAM(m)
                    break;
        case 'l': //clear: lefttop, rightbottom, rgb value
            CHECK_PARAM(7, l)
                for (int i = PARAM(1); i < PARAM(3); ++i)
                    for (int j = PARAM(0); j < PARAM(2); ++j)
                        img.setPixel(j, i, qRgb(PARAM(4), PARAM(5), PARAM(6)));
            break;
        case 'p': //replace
            if (opt.second.size() == 9)
                replace(img,
                qRgb(PARAM(0), PARAM(1), PARAM(2)),
                qRgb(PARAM(3), PARAM(4), PARAM(5)),
                qRgb(PARAM(6), PARAM(7), PARAM(8)));
            else if (opt.second.size() == 6)
                replace(img,
                qRgb(PARAM(0), PARAM(1), PARAM(2)),
                qRgb(PARAM(0), PARAM(1), PARAM(2)),
                qRgb(PARAM(3), PARAM(4), PARAM(5)));
            else
                REPORT_ILLEAGAL_PARAM(p)
                break;
        case 'g': //grayscale
            CHECK_PARAM(1, g)
                if (opt.second[0] == "average")
                    for (auto i = 0; i < img.height(); ++i)
                        for (auto j = 0; j < img.width(); ++j)
                            img.setPixel(j, i, pic2Gray(img.pixel(j, i)));
                else if (opt.second[0] == "min")
                    for (auto i = 0; i < img.height(); ++i)
                        for (auto j = 0; j < img.width(); ++j)
                            img.setPixel(j, i, text2Gray(img.pixel(j, i)));
                else
                    REPORT_ILLEAGAL_PARAM(g)
                    break;
        case 'a':
        { //set alpha
            CHECK_PARAM(1, a)
                unsigned value = opt.second[0].toUInt();
            if (value < 255)
                alpha = true;
            else
                alpha = false;
            for (int i = 0; i < img.height(); ++i)
                for (int j = 0; j < img.width(); ++j)
                    img.setPixel(j, i, (value << 24) | (img.pixel(j, i) & 0xffffff));
        }
        break;
        case 'r':
        { //rotate
            if (opt.second.size() != 1 || opt.second[0] != "left" && opt.second[0] != "right" && opt.second[0] != "horizontal" && opt.second[0] != "vertical")
                REPORT_ILLEAGAL_PARAM(r)
                QMatrix romat;
            romat.rotate(90);
            if (opt.second[0] == "left" || opt.second[0] == "horizontal" && img.height() > img.width() || opt.second[0] == "vertical" && img.height() < img.width())
                img = img.transformed(QTransform(romat));
            else if (opt.second[0] == "right") {
                romat.rotate(-180);
                img = img.transformed(QTransform(romat));
            }
        }
        break;
        case 'b':
        { //brightness, parameter: factor
            CHECK_PARAM(1, b)
                double factor = opt.second[0].toDouble();
            for (auto i = 0; i < img.height(); ++i)
                for (auto j = 0; j < img.width(); ++j)
                    img.setPixel(j, i, changeBrightness(img.pixel(j, i), factor));
        }
        break;
        case 't':
        { // optimization for text, parameter: intensity(1,2,3), bw/gray/colorful, pure/mixed
            if (opt.second.size() != 3 || (opt.second[1] != "gray" && opt.second[1] != "colorful") || (opt.second[2] != "pure" && opt.second[2] != "mixed"))
                REPORT_ILLEAGAL_PARAM(t)
                int intens = PARAM(0);
            bool gray = (opt.second[1] == "gray");
            if (opt.second[2] == "pure")
                text_enhance_pure(img, intens, gray, fragsize, conf.getSrc().toStdString());
            else
                text_enhance_mixed(img, intens, gray);
            // if (opt.second[1] == "bw")
            //     img = toBitmap(img, intens);
            if (!gray)
                cle(img, 1.2 + 0.2 * intens);
        }
        break;
        case 'e': // enrich colors
            CHECK_PARAM(1, e)
                cle(img, opt.second[0].toDouble());
            break;
        case 'f': // assign output format
            CHECK_PARAM(1, f)
                format = opt.second[0];
            if (alpha && format != "png" && format != "tif")
                cout << (QString("Warning: alpha value cannot be written in format ") + format + ".\n").toUtf8().data();
            break;
        case 'h': // text & figure sharpen
            CHECK_PARAM(1, h)
                tfsharpen(img, PARAM(0));
            break;
#ifndef NDEBUG
        case 'd': //debug, output an argb value at given point
            unsigned x = img.pixel(PARAM(0), PARAM(1));
            print_rgb(x);
            break;
#endif
        }
    }
    QString dest = conf.getDst();
    if (!format.isEmpty()) {
        int i = dest.size() - 1;
        while (i >= 0 && dest[i] != '.')
            --i;
        if (i >= 0)
            dest = dest.mid(0, i + 1);
        else    dest += '.';
        dest += format;
    }
    img.save(dest.toUtf8().data(), format.toUtf8().data());
}

void uniproc(const Config& conf) {
    //a universal handle to the process
    QFileInfo srcInfo(conf.getSrc());
    if (srcInfo.isFile())
        process(conf);
    else if (srcInfo.isDir()) {
        QDir srcDir(conf.getSrc());
        QFileInfo dstInfo(conf.getDst());
        dstInfo.dir().mkdir(dstInfo.baseName());
        int i = 0;
        for (const auto& entry : srcDir.entryInfoList()) {
            if (i < 2) {
                ++i;
                continue;
            }
            Config sub(conf);
            sub.setSrc(entry.filePath());
            sub.setDst(dstInfo.dir().path() + "/" + dstInfo.baseName() + "/" + entry.fileName());
            uniproc(sub);
        }
    }
    else
        cout << "Fail to access file or directory \'" << conf.getSrc().toUtf8().data() << "\'.\n";
}
