#pragma once
#ifndef VALUE_HELPERS_H
#define VALUE_HELPERS_H

#include <string>
#include "pyvalue.hpp"

using std::string;

namespace py {
namespace value_helper {

// auto visitor_is_truthy = [](auto&& arg) -> bool {
//             using T = std::decay_t<decltype(arg)>;
//             if constexpr (std::is_integral<T>::value || std::is_floating_point<T>::value) {
//                 return arg == 0;
//             } else if constexpr (std::is_same_v<T, bool>) {
//                 return arg;
//             } else if constexpr (std::is_same_v<T, std::string>)
//                 return arg.length() != 0;
//             return true;
//         };

struct visitor_is_truthy {
    bool operator()(double) const;
    bool operator()(int64_t) const;
    bool operator()(const ValueString&) const;
    bool operator()(const value::NoneType) const;
    inline bool operator()(bool val) const {
        return val;
    }
    // TODO: empty sequence, empty mapping
    // https://docs.python.org/2.4/lib/truth.html
    template<typename T>
    bool operator()(T v) const {
        return true;
    }
};

struct visitor_repr {
    string operator()(double) const;

    string operator()(int64_t) const;

    string operator()(const ValueString&) const;

    string operator()(value::NoneType) const;
    
    template<typename T> 
    string operator()(T) const {
        // TODO: use typetraits to generate this.
        return "<repr>";
    }
};

struct visitor_str {
    template<typename T>
    string operator()(const T& v) const {
        return visitor_repr()(v);
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
    Value operator()(T1, T2) const {
        throw pyerror(string("type error in numeric_visitor, can not work on values of types ") + typeid(T1).name() + " and " + typeid(T2).name());
    }
};


}
}

#endif
