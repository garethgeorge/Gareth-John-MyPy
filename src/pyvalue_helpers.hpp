#ifndef VALUE_HELPERS_H
#define VALUE_HELPERS_H

#include <string>
#include "pyvalue.hpp"

using std::string;

namespace py {
namespace value_helper {

struct visitor_repr: public boost::static_visitor<std::string> {
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

struct visitor_str: public boost::static_visitor<std::string> {
    template<typename T>
    string operator()(const T& v) const {
        return visitor_repr()(v);
    }
};


}
}

#endif