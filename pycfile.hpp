#ifndef PYCFILE_H
#define PYCFILE_H

#include <memory.h>
#include <istream>

using std::unique_ptr;
using std::istream;

class PYCFile {
private:
    unique_ptr<istream> pycfile;
public:
    PYCFile(unique_ptr<istream> pycfile);
};

#endif