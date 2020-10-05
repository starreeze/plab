#include "config.h"
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <algorithm>
Config::Config(int argc, char *argv[]): src(argv[1])
{
    if(src[0] == '-') {
        cout << "Error: no input specified.\n"; exit(-1);
    }
    bool out = false;
    for (int i=2; i<argc;) {
        if(argv[i][0]!='-' || strlen(argv[i])!=2) {
            cout << "Error: unrecognized command: " << argv[i][0] <<".\n";
            exit(-1);
        }
        char c = argv[i][1]; vector<QString> t;
        for(++i; i<argc && argv[i][0]!='-'; ++i)
            t.push_back(argv[i]);
        if(c == 'o'){
            if(out) {
                cout << "Error: multiple outputs specified.\n"; exit(-1);
            }
            setDst(t); out = true;
        }
        else    opts.push_back({c,t});
    }
    if(!out)    setDst(vector<QString>());
}

const vector<pair<char, vector<QString>>>& Config::getOpt() const
{
    return opts;
}

QString Config::getSrc() const
{
    return src;
}

QString Config::getDst() const
{
    return dst;
}

void Config::setSrc(const QString& s)
{
    src = s;
}

void Config::setDst(const QString& d)
{
    dst = d;
}

void Config::setDst(const vector<QString>& args)
{
    if(args.size() > 1) {
        cout << "Illegal parameter for option -o.\n"; exit(-1);
    }
    else if(args.size())
        dst = args[0];
    else {
        int i=src.size()-1;
        for(; i>=0 && src[i]!='/'; --i);
        dst = src;
        dst.resize(++i);
        dst += "output.png";
    }
}
