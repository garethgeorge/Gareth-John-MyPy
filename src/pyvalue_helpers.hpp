#pragma once

#ifndef VALUE_HELPERS_H
#define VALUE_HELPERS_H

#include <string>
#include <sstream>
#include "pyvalue.hpp"

using std::string;

namespace py {

struct Code;
extern std::ostream& operator << (std::ostream& stream, const Value value);

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
        ss << value;
        return ss.str();
    }
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
    Value operator()(T1 a, T2 b) const {
        std::stringstream ss;
        ss << "TypeError: can't add values " << Value(a) << " and " << Value(b);
        throw pyerror(ss.str());
    }
};

}
}

#endif
