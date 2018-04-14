#include "pyvalue_helpers.hpp"
#include <stdio.h>

namespace py {
namespace value_helper {

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
