#include "pycfile.hpp"

PYCFile::PYCFile(unique_ptr<istream> pycfile) : pycfile(std::move(pycfile)) {
    
}