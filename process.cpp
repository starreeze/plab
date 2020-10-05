#include "process.h"
#include "config.h"
#include "progbar.h"
#include <iostream>
#include <array>
#include <QDir>
#include <cmath>
using namespace std;
typedef unsigned char byte;
constexpr int fragsize = 128;

inline double norDist(double x) {
    //a function similar to normal distribution
    return exp(-x*x/3072);
}

inline byte A(unsigned argb) {
    return argb >> 24;
}

inline byte R(unsigned argb) {
    return (argb >> 16) & 0xff;
}

inline byte G(unsigned argb) {
    return (argb >> 8) & 0xff;
}

inline byte B(unsigned argb) {
    return argb & 0xff;
}

inline QRgb bias(QRgb col) {
    byte m = min({R(col), G(col), B(col)});
    return qRgb(R(col)-m, G(col)-m, B(col)-m);
}

inline bool rgble(QRgb x, QRgb y, double scale = 1) {
    //x less or equal scale*y in all r,g,b values
    return R(x) <= scale*R(y) && G(x) <= scale*G(y) && B(x) <= scale*B(y);
}

inline byte byteCut(double val) {
    return val < 0 ? 0 : (val > 0xff ? 0xff : (byte)val);
}

QRgb rgbPlus(QRgb x, QRgb y, double scale = 1) {
    //return x + scale*y, minus can be specified with a negative scale
    return qRgb(byteCut(R(x)+scale*R(y)), byteCut(G(x)+scale*G(y)), byteCut(B(x)+scale*B(y)));
}

QRgb rgbMerge(QRgb x, QRgb y, double factor) {
    //return x + factor*y - factor*averge(y)
    QRgb t = rgbPlus(x, y, factor);
    byte ave = (R(y)+G(y)+B(y))/3;
    return qRgb(byteCut(R(t) - ave*factor), byteCut(G(t) - ave*factor), byteCut(B(t) - ave*factor));
}

QRgb enrichColor(QRgb x, double factor) {
    //return x + factor*bias(x) - factor*averge(bias(x))
    return rgbMerge(x, bias(x), factor);
}

QRgb adjustContrast(QRgb x, QRgb ave, double factor) {
    //set difference from ave according to factor
    double t = max({factor*norDist(R(x)-R(ave)), factor*norDist(G(x)-G(ave)), factor*norDist(B(x)-B(ave))});
    return qRgb(byteCut(R(x)+t), byteCut(G(x)+t), byteCut(B(x)+t));
}

QRgb text2Gray(QRgb x) {
    byte m = min({R(x),G(x),B(x)});
    return qRgb(m,m,m);
}

QRgb pic2Gray(QRgb x) {
    byte m = (R(x) + G(x) + B(x)) / 3;
    return qRgb(m,m,m);
}

QRgb changeBrightness(QRgb x, double factor) {
    return qRgb(R(x)*factor, G(x)*factor, B(x)*factor);
}

void replace(QImage& img, QRgb lower, QRgb upper, QRgb target) {
    for(int i=0; i<img.height(); ++i)
        for(int j=0; j<img.width(); ++j) {
            QRgb t(img.pixel(j, i));
            if(rgble(lower,t) && rgble(t,upper))
                img.setPixel(j, i, target);
        }
}

QRgb rgbAve(const QImage& img, int fraga, int fragb, int aend, int bend) {
    double r=0, g=0, b=0;
    for(int i=fragsize*fraga; i<aend; ++i)
        for(int j=fragsize*fragb; j<bend; ++j) {
            unsigned rgb = img.pixel(j,i);
            r += R(rgb); g += G(rgb); b += B(rgb);
        }
    int pixels = (aend - fragsize*fraga) * (bend - fragsize*fragb);
    r /= pixels; g /= pixels; b /= pixels;
    return qRgb(r, g, b);
}

void smooth(array<int, 256>& distr) {
    for(unsigned i=1; i<distr.size()-1; ++i)
        distr[i] = (distr[i-1]+distr[i]+distr[i+1]) / 3;
}

bool isSmooth(const array<int, 256>& distr) {
    int count = 0;
    for(unsigned i=1; i<distr.size()-1; ++i)
        if(distr[i] > distr[i-1] && distr[i] > distr[i+1])  ++count;
    return count < 3;
}

void deNoise(QImage& img, int intens) {

}

QImage toBitmap(const QImage& img, int intens) {
    array<int, 256> distr; // distribution of grayscale
    distr.fill(0);
    for(int i=0; i<img.height(); ++i)    for(int j=0; j<img.width(); ++j) {
        QRgb x = img.pixel(j,i);
        ++distr[min({R(x),G(x),B(x)})];
    }
    for(int i=0; i<20 && !isSmooth(distr); ++i)
        smooth(distr);
    if(!isSmooth(distr)) {
        cout << "Warning: fail to convert picture to bitmap.\n";
        return img;
    }
    unsigned top[2], idx = 0;
    for(unsigned i=1; i<distr.size()-1; ++i)
        if(distr[i] > distr[i-1] && distr[i] > distr[i+1])
            top[idx++] = i;
    QImage im(img);
    bool t = distr[top[0]] < distr[top[1]];
    for(int i=0; i<img.height(); ++i)    for(int j=0; j<img.width(); ++j) {
        QRgb x = img.pixel(j,i);
        if((min({R(x),G(x),B(x)}) < (top[0] + top[1]) / 2) ^ t)
            im.setPixel(j,i,qRgb(255,255,255));
        else    im.setPixel(j,i,qRgb(0,0,0));
    }
    //deNoise(im, intens);
    return im;
}

#ifndef NDEBUG
void print_rgb(QRgb x) {
    cout << R(x) << ' ' <<G(x) << ' '<<B(x)<<'\n';
}
#endif

void process(const Config &conf)
{
    QString format; bool alpha = false;
    QImage img = QImage(conf.getSrc()).convertToFormat(QImage::Format_ARGB32);
#ifndef NDEBUG
    cout<<"Src: "<<conf.getSrc().toUtf8().data()<<'\n'<<"Dst: "
       <<conf.getDst().toUtf8().data()<<'\n'<<"format: "<<img.format()<<'\n';
#endif
    if(img.format() == QImage::Format_Invalid) {
        cout << "Fail to open \'" << conf.getSrc().toUtf8().data() << "\' as image.\n"; return;
    }
    for(int i=0; !alpha && i<img.height(); ++i)
        for(int j=0; j<img.width(); ++j)
            if((A(img.pixel(j,i))) != 0xff) {
                alpha = true; break;
            }
    for(const auto& opt: conf.getOpt()) {
        switch(opt.first) {
        case 'c': //crop
            if(opt.second.size() != 4) {
                cout << "Illegal parameter for option -c.\n"; exit(-1);
            }
            img = img.copy(opt.second[0].toInt(), opt.second[1].toInt(),
                    opt.second[2].toInt(), opt.second[3].toInt());
            break;
        case 's': //scale
            if(opt.second.size() != 2){
                cout << "Illegal parameter for option -s.\n"; exit(-1);
            }
            img = img.scaled(opt.second[0].toInt(), opt.second[1].toInt());
            break;
        case 'm': //mirror
            if(opt.second.size() != 1){
                cout << "Illegal parameter for option -m.\n"; exit(-1);
            }
            if(opt.second[0] == "horizontal")
                img = img.mirrored();
            else if(opt.second[0] == "vertical")
                img = img.mirrored(true);
            else {
                cout << "Illegal parameter for option -m.\n"; exit(-1);
            }
            break;
        case 'l': //clear: lefttop, rightbottom, rgb value
            if(opt.second.size() == 7)
                for(int i = opt.second[1].toInt(); i < opt.second[3].toInt(); ++i)
                    for(int j = opt.second[0].toInt(); j < opt.second[2].toInt(); ++j)
                        img.setPixel(j, i, qRgb(opt.second[4].toInt(), opt.second[5].toInt(),
                            opt.second[6].toInt()));
            else {
                cout << "Illegal parameter for option -r.\n"; exit(-1);
            }
            break;
        case 'p': //replace
            if(opt.second.size() == 9)
                replace(img,
                  qRgb(opt.second[0].toInt(),opt.second[1].toInt(),opt.second[2].toInt()),
                  qRgb(opt.second[3].toInt(),opt.second[4].toInt(),opt.second[5].toInt()),
                  qRgb(opt.second[6].toInt(),opt.second[7].toInt(),opt.second[8].toInt()));
            else if(opt.second.size() == 6)
                replace(img,
                        qRgb(opt.second[0].toInt(),opt.second[1].toInt(),opt.second[2].toInt()),
                        qRgb(opt.second[0].toInt(),opt.second[1].toInt(),opt.second[2].toInt()),
                        qRgb(opt.second[3].toInt(),opt.second[4].toInt(),opt.second[5].toInt()));
            else {
                cout << "Illegal parameter for option -r.\n"; exit(-1);
            }
            break;
        case 'g': //grayscale
            if(opt.second.size() != 1){
                cout << "Illegal parameter for option -g.\n"; exit(-1);
            }
            if(opt.second[0] == "average")
                for(auto i = 0; i < img.height(); ++i)
                    for(auto j = 0; j < img.width(); ++j)
                        img.setPixel(j,i,pic2Gray(img.pixel(j,i)));
            else if(opt.second[0] == "min")
                for(auto i = 0; i < img.height(); ++i)
                    for(auto j = 0; j < img.width(); ++j)
                        img.setPixel(j,i,text2Gray(img.pixel(j,i)));
            else {
                cout << "Illegal parameter for option -g.\n"; exit(-1);
            }
            break;
        case 'a': {//set alpha
            if(opt.second.size() != 1) {
                cout << "Illegal parameter for option -a.\n"; exit(-1);
            }
            unsigned value = opt.second[0].toUInt();
            if(value < 255)   alpha = true;
            else    alpha = false;
            for(int i=0; i<img.height(); ++i)
                for(int j=0; j<img.width(); ++j)
                    img.setPixel(j,i,(value<<24) | (img.pixel(j,i)&0xffffff));
        }   break;
        case 'r': {//rotate
            if(opt.second.size() != 1 || opt.second[0] != "left" && opt.second[0] != "right"
                    && opt.second[0] != "horizontal" && opt.second[0] != "vertical") {
                cout << "Illegal parameter for option -r.\n"; exit(-1);
            }
            QMatrix romat;
            romat.rotate(90);
            if(opt.second[0] == "left" || opt.second[0] == "horizontal" && img.height() > img.width()
                    || opt.second[0] == "vertical" && img.height() < img.width())
                img = img.transformed(romat);
            else if (opt.second[0] == "right") {
                romat.rotate(-180); img = img.transformed(romat);
            }
        }   break;
        case 'b': {//brightness, parameter: factor
            if(opt.second.size() != 1) {
                cout << "Illegal parameter for option -b.\n"; exit(-1);
            }
            double factor = opt.second[0].toDouble();
            for(auto i = 0; i < img.height(); ++i)
                for(auto j = 0; j < img.width(); ++j)
                    img.setPixel(j,i,changeBrightness(img.pixel(j,i), factor));
        }   break;
        case 't':{ //optimization for text, parameter: intensity(1,2,3), bw/gray/colorful, pure/mixed
            if(opt.second.size() != 3 || opt.second[1] != "gray" && opt.second[1] != "colorful"
                    && opt.second[1] != "bw" || opt.second[2] != "pure" && opt.second[2] != "mixed") {
                cout << "Illegal parameter for option -t.\n"; exit(-1);
            }
            int intens = opt.second[0].toInt();
            bool gray = (opt.second[1] == "gray");
            ProgBar pbar(conf.getSrc().toStdString(), img.height() * img.width());
            //algorithm:
            //(1) regional color correction: split the picture to multiple fragment of 128 and minus
            //  average rgb for each of them to slove the problem of partial illumination;
            //(2) contrast enhance: add rgb if it is higher than average, the more close to average
            //  the more it should be added; same to the contrary. Here the Normal Distrbution
            //  function is used;
            //(3) black and white recognition: gray - rgb higher than c1 * average is set to white
            //  and rgb lower than c2 * average is set to black; bw - foreground and background analysis;
            //(4) color enhence: enrich colors if set to colorful.
            if(opt.second[2] == "pure")    for(int a=0; fragsize*a < img.height(); ++a)
                        for(int b=0; fragsize*b < img.width(); ++b) {
                int aend = fragsize*(a+1)>img.height() ? img.height() : fragsize*(a+1),
                    bend = fragsize*(b+1)>img.width() ? img.width() : fragsize*(b+1);
                pbar.setVal((a*img.width() + b*fragsize) * fragsize); pbar.print();
                // (1)
                QRgb ave = rgbAve(img, a, b, aend, bend);
                for(int i=fragsize*a; i<aend; ++i)    for(int j=fragsize*b; j<bend; ++j) {
                    QRgb t = rgbMerge(img.pixel(j,i), bias(ave), -1);
                    if(gray)    t = text2Gray(t);
                    img.setPixel(j, i, t);
                }
                // (2)
                ave = rgbAve(img, a, b, aend, bend);
                for(int i=fragsize*a; i<aend; ++i)    for(int j=fragsize*b; j<bend; ++j)
                    img.setPixel(j,i, adjustContrast(img.pixel(j,i),ave,64+32*intens));
                // (3)
                if(opt.second[1] != "bw") {
                    ave = rgbAve(img, a, b, aend, bend);
                    for(int i=fragsize*a; i<aend; ++i)    for(int j=fragsize*b; j<bend; ++j) {
                        if(rgble(ave, img.pixel(j,i), 0.8 + 0.1*intens + 0.2*gray)
                          && [&](){auto b=bias(img.pixel(j,i)); return max({R(b),G(b),B(b)})<=10*intens;}())
                            img.setPixel(j, i, img.pixel(j,i) | 0x00ffffff);
                        else if(rgble(img.pixel(j,i), ave, 0.5 + 0.2*intens + 0.6*gray) && rgble(img.pixel(j,i),
                          qRgb(30 + 30*intens + 100*gray,30 + 30*intens + 100*gray,30 + 30*intens + 100*gray))
                          && [&](){auto b=bias(img.pixel(j,i)); return max({R(b),G(b),B(b)})<=5*intens;}())
                            img.setPixel(j, i, img.pixel(j,i) & 0xff000000);
                    }
                }
            } else {
                // (2)
                auto ave = rgbAve(img, 0, 0, img.height(), img.width());
                for(int i=0; i<img.height(); ++i)    for(int j=0; j<img.width(); ++j) {
                    auto t = img.pixel(j,i);
                    if(gray)    t = pic2Gray(t);
                    img.setPixel(j,i, adjustContrast(t,ave,32+32*intens));
                }
                // (3)
                ave = rgbAve(img, 0, 0, img.height(), img.width());
                for(int i=0; i<img.height(); ++i)    for(int j=0; j<img.width(); ++j) {
                    if(rgble(ave, img.pixel(j,i), 0.2 + 0.1*intens)
                        && [&](){auto b=bias(img.pixel(j,i)); return max({R(b),G(b),B(b)})<=3*intens;}())
                        img.setPixel(j, i, img.pixel(j,i) | 0x00ffffff);
                    else if(rgble(img.pixel(j,i), ave, 0.2 + 0.1*intens) && rgble(img.pixel(j,i),
                      qRgb(15 + 15*intens,15 + 15*intens,15 + 15*intens))
                      && [&](){auto b=bias(img.pixel(j,i)); return max({R(b),G(b),B(b)})<=5*intens;}())
                        img.setPixel(j, i, img.pixel(j,i) & 0xff000000);
                }
            }
            if(opt.second[1] == "bw")
                img = toBitmap(img, intens);
            // (4)
            if(!gray)  for(int i=0; i<img.height(); ++i)
                for(int j=0; j<img.width(); ++j)
                    img.setPixel(j,i,enrichColor(img.pixel(j,i), 1.2 + 0.2*intens));
        }   break;
        case 'e': {//enrich colors
            if(opt.second.size() != 1) {
                cout << "Illegal parameter for option -e.\n"; exit(-1);
            }
            double scale = opt.second[0].toDouble();
            for(int i=0; i<img.height(); ++i)
                for(int j=0; j<img.width(); ++j)
                    img.setPixel(j,i,enrichColor(img.pixel(j,i), scale));
        }   break;
        case 'f': //assign output format
            if(opt.second.size() != 1) {
                cout << "Illegal parameter for option -f.\n"; exit(-1);
            }
            format = opt.second[0];
            if(alpha && format != "png" && format != "tif")
                cout << (QString("Warning: alpha value cannot be written in format ")
                         + format + ".\n").toUtf8().data();
#ifndef NDEBUG
            break;
        case 'd': //debug, output an argb value at given point
            unsigned x = img.pixel(opt.second[0].toInt(), opt.second[1].toInt());
            print_rgb(x);
#endif
        }
    }
    QString f = format.isEmpty() ? QString() : format
        , dest = conf.getDst();
    if(!f.isEmpty()) {
        for(int i=dest.size()-1; i>=0 && dest[i]!='.'; --i)
            remove_if(dest.end()-1, dest.end(),[](QChar){return true;});
        dest += format;
    }
    img.save(dest.toUtf8().data(), f.toUtf8().data());
}

void uniproc(Config& conf) {
    //a universal handle to the process
    QFileInfo srcInfo(conf.getSrc());
    if(srcInfo.isFile())    process(conf);
    else if(srcInfo.isDir()) {
        QDir srcDir(conf.getSrc());
        QFileInfo dstInfo(conf.getDst());
        dstInfo.dir().mkdir(dstInfo.baseName());
        for(const auto& entry: srcDir.entryInfoList(QDir::Files)) {
            conf.setSrc(entry.filePath());
            conf.setDst(dstInfo.dir().path() + "/" + dstInfo.baseName()
                        + "/" + entry.fileName());
            process(conf);
        }
    }
    else    cout << "Fail to access file or directory \'" << conf.getSrc().toUtf8().data() << "\'.\n";
}
