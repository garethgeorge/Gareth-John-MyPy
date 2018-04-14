#ifndef VALUE_H
#define VALUE_H

#include <memory>
#include <stdexcept>
#include <stdint.h>
#include <boost/variant/variant.hpp>
#include "pyerror.hpp"

namespace py {

struct Value {
    using Type = size_t;

    virtual Type getType() {
        throw pyerror("getType not implemented on py::Value");
    }
    
    virtual std::shared_ptr<Value> add(std::shared_ptr<Value>&) {
        throw pyerror("add not implemented on py::Value");
    }
};

}

#endif