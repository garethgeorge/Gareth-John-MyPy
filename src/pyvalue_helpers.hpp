#pragma once

#ifndef VALUE_HELPERS_H
#define VALUE_HELPERS_H

#include <string>
#include <sstream>
#include "pyvalue.hpp"
#include <tuple>
#include "pyframe.hpp"
#include "pyinterpreter.hpp"
#include <variant>

using std::string;

namespace py {

struct Code;
extern std::ostream& operator << (std::ostream& stream, const Value value);
template<typename T>
extern std::ostream& operator << (std::ostream& stream, const gc_ptr<T> value) {
    return stream << Value(value);
}

namespace value_helper {

// helper function for creating a class with overloads
// see http://en.cppreference.com/w/cpp/utility/variant/visit 
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

// a visitor that, given a type, returns its truthyness 
struct visitor_is_truthy {
    bool operator()(double) const;
    bool operator()(int64_t) const;
    bool operator()(const ValueString&) const;
    bool operator()(const value::NoneType) const;
    inline bool operator()(bool val) const {
        // inline this to make the common case fast
        return val;
    }
    // TODO: empty sequence, empty mapping
    // https://docs.python.org/2.4/lib/truth.html
    template<typename T>
    bool operator()(T v) const {
        return true;
    }
};

// this visitor evaluates the python str(obj) method, only differs from
// repr in the case of true objects so as you can see it just wraps repr
struct visitor_str {
    // using visitor_repr::operator();
    // TODO: add specialization for handling additional types
    template<typename T>
    string operator()(T& value) {
        std::stringstream ss;
        ss << Value(value);
        return ss.str();
    }
};

struct call_visitor {
    /*
        the call visitor is a helpful visitor class that actually includes 
        some amount of state, it takes the argument list as well as the current
        frame. 

        It may be possible to refactor this into a visitor using the lambda
        style syntax ideally.
    */

    FrameState& frame;
    ArgList& args;
    call_visitor(FrameState& frame, ArgList& args) : frame(frame), args(args) {}

    void operator()(const ValueCFunction& func) const {
        DEBUG("call_visitor dispatching CFunction->action");
        func->action(frame, args);
    }

    void operator()(const ValueCMethod& func) const {
        DEBUG("call_visitor dispatching CMethod->action");
        // args.insert(args.begin(), func->thisArg);
        args.bind(func->thisArg);
        func->action(frame, args);
    }

    // A PyClass was called like a function, therein creating a PyObject
    void operator()(ValuePyClass& cls) const;

    void operator()(const ValuePyFunction& func) const;

    // An object was called like a function
    // Look for it'a __call__ overload and run that
    void operator()(ValuePyObject& obj) const;
    
    template<typename T>
    void operator()(T) const {
        throw pyerror(string("can not call object of type ") + typeid(T).name());
    }
};

template<class T>
struct numeric_visitor {
    FrameState& frame;

    numeric_visitor(FrameState& frame) : frame(frame) {}

    void operator()(double v1, double v2) const {
        frame.value_stack.push_back(T::action(v1, v2));
    }
    void operator()(double v1, int64_t v2) const {
        frame.value_stack.push_back(T::action(v1, v2));
    }
    void operator()(int64_t v1, double v2) const {
        frame.value_stack.push_back(T::action(v1, v2));
    } 
    void operator()(int64_t v1, int64_t v2) const {
        frame.value_stack.push_back(T::action(v1, v2));
    }
    
    void operator()(ValuePyObject& v1, ValuePyObject& v2) const {
        /// Get the attribute for it
        std::tuple<Value,bool> res = value::PyObject::find_attr_in_obj(v1,std::string(T::l_attr));
        if(std::get<1>(res)){
            // Call it like a function
            ArgList arglist(v2);
            arglist.bind(v1);
            std::visit(
                call_visitor(frame, arglist),
                std::get<0>(res)
            );
        } else {
            throw pyerror(
                string("TypeError: unsupported operand type(s) for ") + T::op_name + string(": '")
                + *(std::get<ValueString>((v1->static_attrs->attrs->at("__qualname__"))))
                + string("' and '")
                + *(std::get<ValueString>((v1->static_attrs->attrs->at("__qualname__")))) + "' "
            );
        }
    }

    template<typename OT>
    void operator()(ValuePyObject& v1, OT& v2) const {
        // Get the attribute for it
        std::tuple<Value,bool> res = value::PyObject::find_attr_in_obj(v1,std::string(T::l_attr));
        if(std::get<1>(res)){
            // Call it like a function
            ArgList arglist(v2);
            arglist.bind(v1);
            std::visit(
                call_visitor(frame, arglist),
                std::get<0>(res)
            );
        } else {
            throw pyerror(
                string("TypeError: unsupported operand type(s) for ") + T::op_name + string(": '")
                + *(std::get<ValueString>((v1->static_attrs->attrs->at("__qualname__"))))
                + string("' and '") + typeid(OT).name() + "' "
            );
        }
    }

    template<typename OT2>
    void operator()(OT2& v1, ValuePyObject& v2) const {
        // Get the attribute for it
        std::tuple<Value,bool> res = value::PyObject::find_attr_in_obj(v2,std::string(T::r_attr));
        if(std::get<1>(res)){
            // Call it like a function
            ArgList arglist(v1);
            arglist.bind(v2);
            std::visit(
                call_visitor(frame, arglist),
                std::get<0>(res)
            );
        } else {
            throw pyerror(string("TypeError: unsupported operand type(s) for ") + T::op_name + string(": '")
                + typeid(OT2).name() + string("' and '") 
                + *(std::get<ValueString>((v2->static_attrs->attrs->at("__qualname__")))) + "' "
            );
        }
    }
    
    template<typename T1, typename T2>
    void operator()(T1 a, T2 b) const {
        std::stringstream ss;
        ss << "TypeError: can't add values " << Value(a) << " and " << Value(b);
        throw pyerror(ss.str());
    }
};


// Visitor for accessing class attributes
struct load_attr_visitor {
    FrameState& frame;
    const std::string& attr;

    load_attr_visitor(FrameState& frame, const std::string& attr) : frame(frame), attr(attr) {}
    
    void operator()(ValuePyClass& cls){
        try {
            frame.value_stack.push_back(cls->attrs->at(attr));
        } catch (const std::out_of_range& oor) {
            auto& attrs = *(cls->attrs);
            throw pyerror(std::string(
                *(std::get<ValueString>( (attrs)["__qualname__"]))
                + " has no attribute " + attr
            ));
        }
    }

    // For pyobject, first look in their own namespace, then look in their static namespace
    // ValuePyObject cannot be const as it might be modified
    void operator()(ValuePyObject& obj){
        // First look in my own namespace
        std::tuple<Value,bool> res = value::PyObject::find_attr_in_obj(obj, attr);
        if(std::get<1>(res)){
            frame.value_stack.push_back(std::get<0>(res));
        } else {
            // Nothing found, throw error!
            auto& attrs = (*(obj->static_attrs->attrs));
            throw pyerror(std::string(
                // Should this be __name__??
                *(std::get<ValueString>(attrs["__qualname__"]))
                + " has no attribute " + attr
            ));
        }
    }

    // Load attribute for a List 
    void operator()(ValueList& list);

    template<typename T>
    void operator()(T) const {
        throw pyerror(string("can not get attributed from an object of type ") + typeid(T).name());
    }

};
// I do not believe this can be a reference
// Return a cell
ValuePyObject create_cell(Value contents);

}
}

#endif
