#ifndef VALUE_H
#define VALUE_H

#include <memory>
#include <stdexcept>
#include <stdint.h>
#include <boost/variant/variant.hpp>

namespace py {

struct Value {
    using Type = size_t;

    virtual Type getType() {
        throw std::runtime_error("getType not implemented on py::Value");
    }
    
    virtual std::shared_ptr<Value> add(std::shared_ptr<Value>& other) {
        throw std::runtime_error("add not implemented on py::Value");
    }
};

}

#endif