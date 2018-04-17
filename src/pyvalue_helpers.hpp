#pragma once
#ifndef VALUE_HELPERS_H
#define VALUE_HELPERS_H

#include <string>
#include "pyvalue.hpp"

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
    Value operator()(double v1, double v2) const {
        return T::action(v1, v2);
    }
    Value operator()(double v1, int64_t v2) const {
        return T::action(v1, v2);
    }
    Value operator()(int64_t v1, double v2) const {
        return T::action(v1, v2);
    }
    Value operator()(int64_t v1, int64_t v2) const {
        return T::action(v1, v2);
    }
    
    template<typename T1, typename T2>
    Value operator()(T1, T2) const {
        throw pyerror(string("type error in numeric_visitor, can not work on values of types ") + typeid(T1).name() + " and " + typeid(T2).name());
    }
};

}
}

#endif
