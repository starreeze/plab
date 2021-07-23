#ifndef CONFIG_H
#define CONFIG_H
#include <QString>
#include <vector>
using namespace std;
class Config
{
    QString src, dst;
    vector<pair<char, vector<QString>>> opts;
    void setDst(const vector<QString> &args);

public:
    Config(int argc, char *argv[]);
    const vector<pair<char, vector<QString>>> &getOpt() const;
    QString getSrc() const;
    QString getDst() const;
    void setSrc(const QString &src);
    void setDst(const QString &dst);
};

#endif // CONFIG_H
