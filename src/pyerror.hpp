#ifndef PYERROR_H
#define PYERROR_H

#include <stdexcept>

struct pyerror : public std::runtime_error { 
    pyerror(const std::string& message) : std::runtime_error(message) {};
};

#endif