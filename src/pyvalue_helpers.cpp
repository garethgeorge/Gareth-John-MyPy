#include "pyvalue_helpers.hpp"
#include <stdio.h>

namespace py {
namespace value_helper {
/*
    visitor_is_truthy
*/
bool visitor_is_truthy::operator()(double d) const {
    return d != (double)0;
}

bool visitor_is_truthy::operator()(int64_t d) const {
    return d != (int64_t)0;
}

bool visitor_is_truthy::operator()(const ValueString& s) const {
    return s->size() != 0;
}

bool visitor_is_truthy::operator()(const value::NoneType) const {
    return false;
}


/*
    visitor_repr
*/
string visitor_repr::operator()(bool v) const {
    if (v) return "true";
    return "false";
}

string visitor_repr::operator()(double d) const {
    char buff[128];
    sprintf(buff, "%f", d);
    return buff;
}

string visitor_repr::operator()(int64_t d) const {
    char buff[128];
    sprintf(buff, "%lld", d);
    return buff;
}

string visitor_repr::operator()(const ValueString& d) const {
    return *d;
}

string visitor_repr::operator()(value::NoneType) const {
    return "None";
}

}
}
