#include "config.h"
#include "process.h"

int main(int argc, char *argv[])
{
    Config conf(argc, argv);
    uniproc(conf);
    return 0;
}
