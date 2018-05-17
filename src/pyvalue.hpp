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
#include <tuple>

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

    // Constants for flags
    const uint8_t INSTANCE_METHOD = 1;
    const uint8_t CLASS_METHOD = 2;
    const uint8_t STATIC_METHOD = 4;

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
using ValuePyFunction = std::shared_ptr<value::PyFunc>;
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
using Namespace = std::shared_ptr<std::unordered_map<std::string, Value>>;

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

        // CHANGE TO TO A TUPLE!
        ValueList __closure__ = nullptr;

    };

    // The static value of a class
    struct PyClass {
        // Static Attributes
        Namespace attrs;

        // Things I inherit from
        std::vector<ValuePyClass> parents;

        // Check if we are done computing method resolution order
        static bool more_linearization_work_to_do(std::vector<std::vector<ValuePyClass>>& ls);

        // Which list has a head which is not in the tail of any other list
        static int get_next_good_head_ind(std::vector<std::vector<ValuePyClass>>& ls);

        // Remove a class from being considered in the linearizations
        static void remove_from_linearizations(
            std::vector<std::vector<ValuePyClass>>& ls,
            ValuePyClass& cls    
        );

        PyClass();

        PyClass(std::string&& qualname);

        PyClass(std::vector<ValuePyClass>& ps);

        // Defined in FrameState
        static std::tuple<Value,bool> find_attr_in_parents(
                                    const ValuePyClass& cls,
                                    const std::string& attr
                                );

        // Store an attribute into attrs
        void store_attr(const std::string& str,Value& val){
            (*attrs)[str] = val;
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
        PyObject(const ValuePyClass& cls) : static_attrs(cls) {
            this->attrs = std::make_shared<std::unordered_map<std::string, Value>>();
        };

        // Store an attribute into attrs
        void store_attr(const std::string& str,Value& val){
            (*attrs)[str] = val;
        }

        // Defined in FrameState
        // This should REALLY be changed to a not static method but that will happen soon
        static std::tuple<Value,bool> find_attr_in_obj(
                            const ValuePyObject& obj,
                            const std::string& attr
                        );
    };
}

}

#endif