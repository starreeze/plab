#include "progbar.h" //text progress bar
#include <iostream>
#include <sstream>
#include <iomanip>
using namespace std;

ProgBar::ProgBar(string name, int max, int cur, int length)
    : title(name), len(length), maxVal(max), curVal(cur) {}

ProgBar::~ProgBar() {
    finish();
}

void ProgBar::setVal(int cur) {
    if (curVal < maxVal)
        curVal = cur;
}

void ProgBar::finish() {
    curVal = maxVal;
    cout << "\r" << title << left << setw(40) << ": Done.";
    cout << '\n';
}

void ProgBar::print() const {
    int pos = curVal * len / maxVal, i = 0;
    cout << "\r" << title << ": [";
    for (; i < pos; ++i)
        cout << '#';
    for (; i < len; ++i)
        cout << '.';
    cout << left << setw(10) << ']';
}
