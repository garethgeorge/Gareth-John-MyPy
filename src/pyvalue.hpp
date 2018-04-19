#ifndef VALUE_H
#define VALUE_H

#include <memory>
#include <stdint.h>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <variant>
#include <functional>
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
    struct List;
    // struct Map;
    // struct Set;
}

// larger value types should be wrapped by a shared_ptr,
// this is because we want to keep the size of our std::variant class small,
// it also allows sharing string objects between multiple values whenever possible
using ValueString = shared_ptr<std::string>;
using ValueCode = shared_ptr<const Code>;
using ValueList = shared_ptr<value::List>;
using ValueCFunction = shared_ptr<const value::CFunction>;


using Value = std::variant<
    bool,
    int64_t,
    double,
    ValueString,
    ValueCode,
    ValueCFunction,
    ValueList,
    value::NoneType
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