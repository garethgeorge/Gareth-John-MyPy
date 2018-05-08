#pragma once

#ifndef VALUE_HELPERS_H
#define VALUE_HELPERS_H

#include <string>
#include "pyvalue.hpp"
#include <tuple>
#include "pyframe.hpp"
#include <variant>

using std::string;

namespace py {
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

// this visitor evaluates the python repr(obj) method
struct visitor_repr {
    string operator()(bool) const;
    
    string operator()(double) const;

    string operator()(int64_t) const;

    string operator()(const ValueString&) const;

    string operator()(value::NoneType) const;

    string operator()(ValuePyFunction) const;

    string operator()(ValuePyClass) const;

    string operator()(ValuePyObject) const;
    
    template<typename T> 
    string operator()(T) const {
        // TODO: use typetraits to generate this.
        throw pyerror("unimplemented repr for type");
    }
};

// this visitor evaluates the python str(obj) method, only differs from
// repr in the case of true objects so as you can see it just wraps repr
struct visitor_str : public visitor_repr {
    using visitor_repr::operator();
    // TODO: add specialization for handling additional types
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
    
    void operator()(ValuePyObject& v1, int64_t v2) const {
        // Get the attribute for it
        std::tuple<Value,bool> res = value::PyObject::find_attr_in_obj(v1,std::string(T::l_attr));
        if(std::get<1>(res)){
            throw pyerror(string("huh"));
        } else {
            throw pyerror(
                string("TypeError: unsupported operand type(s) for ") + T::op_name + string(": '")
                + *(std::get<ValueString>((v1->static_attrs->attrs->at("__qualname__")))) + string("' and '") + string("INT'")
            );
        }
    }
    
    template<typename T1, typename T2>
    void operator()(T1, T2) const {
        throw pyerror(string("type error in numeric_visitor, can not work on values of types ") + typeid(T1).name() + " and " + typeid(T2).name());
    }
};

}
}

#endif
