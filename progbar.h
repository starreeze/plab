#ifndef PROGBAR_H
#define PROGBAR_H
#include <string>
using namespace std;

class ProgBar
{
    int len, maxVal, curVal;
    string title;
    void finish();

public:
    ProgBar(string name, int max, int cur = 0, int length = 30);
    ~ProgBar();
    void setVal(int cur);
    void print() const;
};

#endif // PROGBAR_H
