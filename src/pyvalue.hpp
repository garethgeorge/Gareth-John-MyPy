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
#include <tuple>

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

    // Constants for flags
    const uint8_t INSTANCE_METHOD = 1;
    const uint8_t CLASS_METHOD = 2;
    const uint8_t STATIC_METHOD = 4;

    struct NoneType { };

    struct CFunction;
    struct CMethod;
    struct List;
    struct Tuple;
    // struct Map;
    // struct Set;

    // A function of python code
    struct PyFunc;
    struct PyGenerator {
        gc_ptr<FrameState> frame;
        
        inline bool operator == (const PyGenerator& other) const {
            return frame == other.frame;
        }
    };

    // The internal representation of a class
    // Created by the builtin __build_class__
    struct PyClass;
    
    // A particular instance of a class
    struct PyObject;
}

// larger value types should be wrapped by a shared_ptr,
// this is because we want to keep the size of our std::variant class small,
// it also allows sharing string objects between multiple values whenever possible
using ValueString = gc_ptr<std::string>;
using ValueCode = gc_ptr<Code>;
using ValueCFunction = std::shared_ptr<value::CFunction>;
using ValueCMethod = std::shared_ptr<value::CMethod>;
using ValuePyFunction = gc_ptr<value::PyFunc>;
using ValuePyObject = gc_ptr<value::PyObject>;
using ValuePyGenerator = value::PyGenerator;

// I think PyClasses (the thing that holds the statics of a class) can be deallocated.
// Need to confirm this thoa
// THIS NEEDS TO CHANGE TO A GC_PTR (or do i?)
using ValuePyClass = gc_ptr<value::PyClass>;
using ValueList = gc_ptr<value::List>;

using Value = std::variant<
    bool,
    int64_t,
    double,
    ValueString,
    ValueCode,
    ValueCFunction,
    ValueCMethod,
    value::NoneType,
    ValuePyFunction,
    ValuePyClass,
    ValuePyObject,
    ValueList,
    ValuePyGenerator
>;

// Bad copy/paste from pyinterpreter.hpp
using Namespace = std::shared_ptr<std::unordered_map<std::string, Value>>;

// Arg List definition
struct ArgList {
    /*
        if index is set to 0 initially, this indicates that there is a value for self
        otherwise index starts out holding the value 1 
    */
    size_t offset;
    std::vector<Value> _args;

    ArgList() {
        offset = 1;
        _args.resize(1);
    }

    ArgList(std::vector<Value>::iterator&& begin, std::vector<Value>::iterator&& end) : ArgList() {
        // insert all of those arguments
        _args.reserve(_args.size() + distance(begin, end));
        _args.insert(_args.end(), begin, end);
    }

    ArgList(const Value& value) : ArgList() {
        _args.insert(_args.end(), value);
    }

    void append_arg(const Value& value) {
        _args.insert(_args.end(), value);
    }

    void bind(Value& thisArg) {
        offset = 0;
        _args[0] = thisArg;
    }

    void bind(Value&& thisArg) {
        offset = 0;
        _args[0] = thisArg;
    }

    inline const Value& operator[](size_t index) const {
        return _args[offset + index];
    }

    inline Value& operator[](size_t index) {
        return _args[offset + index];
    }
    
    inline size_t size() const {
        return _args.size() - offset;
    }
};

namespace value {
    struct CFunction {
        std::function<void(FrameState&, ArgList&)> action;
        CFunction(const decltype(action)& _action) : action(_action) { };
    };

    struct CMethod {
        Value thisArg = value::NoneType();
        std::function<void(FrameState&, ArgList&)> action;
        
        CMethod(const decltype(action)& action) : thisArg(thisArg), action(action) {
        }

        CMethod(Value& thisArg, const decltype(action)& action) : thisArg(thisArg), action(action) {
        }

        ValueCMethod bindThisArg(Value&& thisArg) {
            return std::make_shared<CMethod>(thisArg, action);
        }
    };
    
    struct List { 
        // we must wrap the vector in a forward declared struct, because otherwise the type information
        // and more importantly the size of py::Value, is not available at the time of its creation
        // (before it needs to be included in the std::variant)
        std::vector<Value> values;

        template < typename... Args> 
        List(Args&&... args) : values(std::forward<Args>(args)...) {
        };

        auto begin() {
            return values.begin();
        }

        auto begin() const {
            return values.begin();
        }

        auto end() {
            return values.end();
        }

        auto end() const {
            return values.end();
        }

        auto& operator[](size_t index) {
            return values[index];
        }

        const auto& operator[](size_t index) const {
            return values[index];
        }

        size_t size() const {
            return values.size();
        }
    };
    
    // struct Set {
    //     unordered_set<Value> values;
    // };

    // struct Map {
    //     unordered_map<Value, Value> values;
    // }
    
    struct PyFunc {
        // Its name
        const ValueString name;
        
        // Its code
        const ValueCode code;

        // Its default argument
        const ValueList def_args;

        // A pointer to self for if this is an instance function or class function
        Value self = value::NoneType();

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
        void store_attr(const std::string& str, Value val){
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
        void store_attr(const std::string& str, Value val){
            (*attrs)[str] = val;
        }

        // Defined in FrameState
        // This should REALLY be changed to a not static method but that will happen soon
        static std::tuple<Value,bool> find_attr_in_obj(
                            ValuePyObject& obj,
                            const std::string& attr
                        );
    };
}

}

#endif