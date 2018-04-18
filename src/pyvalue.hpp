#pragma once
#ifndef VALUE_H
#define VALUE_H

#include <memory>
#include <stdint.h>
#include <vector>
#include <variant>
#include <functional>
#include "pyerror.hpp"

namespace py {

// forward declarations
struct Code;
struct FrameState;

// the value namespace for C value types
namespace value {
    struct NoneType { };

    struct CFunction;

    struct PyFunc;
}

// larger value types should be wrapped by a shared_ptr,
// this is because we want to keep the size of our std::variant class small,
// it also allows sharing string objects between multiple values whenever possible
using ValueString = std::shared_ptr<std::string>;
using ValueCode = std::shared_ptr<const Code>;
using ValueCFunction = std::shared_ptr<const value::CFunction>;
using ValuePyFunction = std::shared_ptr<const value::PyFunc>;

using Value = std::variant<
    bool,
    int64_t,
    double,
    ValueString,
    ValueCode,
    ValueCFunction,
    value::NoneType,
    ValuePyFunction
>;

namespace value {
    struct CFunction {
        std::function<void(FrameState&, std::vector<Value>&)> action;
        CFunction(const std::function<void(FrameState&, std::vector<Value>&)>& _action) : action(_action) { };
    };
}

// A struct that holds a python function
namespace value {
    struct PyFunc{
        // It's code
        ValueCode code;

        // It's name
        ValueString name;

        // It's default argument
        std::vector<Value> def_args;

        // Constructor
        PyFunc(std::shared_ptr<std::string> nm, std::shared_ptr<const Code> c, std::vector<Value> defaults) { };
    };
}

// the beginning of a prototype for object definitions
// struct PyObjectType;

// struct PyObject {
//     using PyObjShared = std::shared_ptr<PyObject>;
//     PyObjectType *type;
// };

// struct PyObjectType {
//     using PyObjMethod = std::function<PyObjShared(PyObjShared&,PyObjShared&)>*;

//     PyObjMethod add;
//     PyObjMethod sub;
//     PyObjMethod mul;
//     PyObjMethod div;
//     PyObjMethod str;

//     Namespace properties;
// };


}

#endif