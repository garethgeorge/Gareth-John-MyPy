#ifndef VALUE_H
#define VALUE_H

#include <memory>
#include <stdint.h>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <variant>
#include <functional>
#include <ostream>
#include <pygc.hpp>
#include "pyerror.hpp"

namespace py {

using namespace gc;

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
    struct Tuple;
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
using ValuePyObject = std::shared_ptr<value::PyObject>;

// I think PyClasses (the thing that holds the statics of a class) can be deallocated.
// Need to confirm this tho
// THIS NEEDS TO CHANGE TO A GC_PTR (or do i?)
using ValuePyClass = std::shared_ptr<value::PyClass>;
using ValueList = gc_ptr<value::List>;


using Value = std::variant<
    bool,
    int64_t,
    double,
    ValueString,
    ValueCode,
    ValueCFunction,
    value::NoneType,
    ValuePyFunction,
    ValuePyClass,
    ValuePyObject,
    ValueList
>;

// Bad copy/paste from pyinterpreter.hpp
using Namespace = std::unordered_map<std::string, Value>;

namespace value {
    struct CFunction {
        std::function<void(FrameState&, std::vector<Value>&)> action;
        CFunction(const std::function<void(FrameState&, std::vector<Value>&)>& _action) : action(_action) { };
    };
    
    struct List { 
        // we must wrap the vector in a forward declared struct, because otherwise the type information
        // and more importantly the size of py::Value, is not available at the time of its creation
        // (before it needs to be included in the std::variant)
        std::vector<Value> values;
    };
    
    // struct Set {
    //     unordered_set<Value> values;
    // };

    // struct Map {
    //     unordered_map<Value, Value> values;
    // }
}

// A struct that holds a python function
namespace value {
    
    struct PyFunc {
        // It's name
        const ValueString name;
        
        // It's code
        const ValueCode code;

        // It's default argument
        const std::shared_ptr<std::vector<Value>> def_args;

        // A pointer to self for if this is an instance function or class function
        const Value self;

        // Flags as needed
        const uint8_t flags;

        bool get_am_class_method() const {
            return flags & 1;
        }

        bool get_am_static_method() const {
            return flags & 2;
        }

        bool get_am_instance_method() const {
            return flags & 4;
        }

        bool get_know_which_class() const {
            return flags & 8;
        }
    };

    // The static value of a class
    struct PyClass {
        // Static Attributes
        Namespace attrs;

        // Things I inherit from
        std::vector<ValuePyClass> parents;

        PyClass(Namespace ns){
            attrs = ns;
        }

        PyClass(std::vector<ValuePyClass>& ps){
            // Copying... meh
            parents = ps;
        }

        // Store an attribute into attrs
        void store_attr(const std::string& str,Value& val){
            attrs[str] = val;
        }
    };

    // A pyobject holds a namespace of it's own as well as a way reference
    // The static namespace for it's class
    // LOAD_ATTR and STORE_ATTR will first search attrs, then static_class
    // Should I make PyObject just a derived class of PyClass?
    struct PyObject {
        // A pointer back to static stuff
        const ValuePyClass static_attrs;
        
        // My attributes
        Namespace attrs;

        // When first creating this, all it needs do is reference it's static class
        // __init__ will be called later if necessary
        PyObject(const ValuePyClass& cls) : static_attrs(cls) {};

        // Store an attribute into attrs
        void store_attr(const std::string& str,Value& val){
            attrs[str] = val;
        }
    };
}

}

#endif