#ifndef VALUE_H
#define VALUE_H

#include <memory>
#include <stdint.h>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <variant>
#include <functional>
#include <pygc.hpp>
#include "pyerror.hpp"

namespace py {

using std::vector;
using std::unordered_set;
using std::unordered_map;
using std::string;
using std::shared_ptr;

// forward declarations
struct Code;
struct FrameState;

// the value namespace for C value types
namespace value {
    struct NoneType { };

    struct CFunction;
    // struct List;
    // struct Map;
    // struct Set;

    // A function of python code
    struct PyFunc;

    // The internal representation of a class
    // Created by the builtin __build_class__
    struct PyClass;
    
    // A particular instance of a class
    struct PyObject;
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
    
    // struct List { 
    //     vector<Value> list;
    // };
    
    // struct Set {
    //     unordered_set<Value> values;
    // }

    // struct Map {
    //     unordered_map<Value, Value> values;
    // }
}

// A struct that holds a python function
namespace value {
    struct PyFunc{
        // It's name
        const ValueString name;
        
        // It's code
        const ValueCode code;

        // It's default argument
        const std::shared_ptr<std::vector<Value>> def_args;
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