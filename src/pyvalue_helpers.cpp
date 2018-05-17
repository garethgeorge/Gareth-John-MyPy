#include "pyvalue_helpers.hpp"
#include "builtins/builtins.hpp"
#include <iostream>
#include <sstream>
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
    attribute visitor implementations for specific types i.e. lists
*/

// Namespace list_attributes;
// struct init_list_attributes {
//     init_list_attributes() {
//         list_attributes
//     }
// };

void load_attr_visitor::operator()(ValueList& list) {
    if (builtins::builtin_list_attributes.find(attr) != builtins::builtin_list_attributes.end()) {
        auto& method = builtins::builtin_list_attributes[attr];
        frame.value_stack.push_back(method->bindThisArg(list));
    } else {
        std::stringstream ss;
        ss << "AttributeError: attribute '" << attr << "' could not be found";
        throw pyerror(ss.str());
    }
}


/*
    visitor_debug_repr
*/
struct visitor_debug_repr {
    std::ostream& stream;
    visitor_debug_repr(std::ostream& stream) : stream(stream) { };

    void operator()(bool v) {
        stream << v ? "true" : "false";
    }

    void operator()(double d) {
        stream << d;
    }

    void operator()(int64_t d) {
        stream << d;
    }

    void operator()(ValueString d) {
        stream << *d;
    }

    void operator()(value::NoneType) {
        stream << "None";
    }

    void operator()(ValuePyFunction func) {
        if (func != nullptr) {
            stream << "PyFunc_" << *(func->name);
        } else 
            stream << "PyFunc_<anonymous>";
    }

    void operator()(ValuePyClass arg) {
        stream << "ValuePyClass ("
               << *(std::get<ValueString>((*(arg->attrs))["__qualname__"])) << ")";
    }

    void operator()(ValuePyObject arg) {
        stream << "ValuePyObject of class ("
               << *(std::get<ValueString>((*(arg->static_attrs->attrs))["__qualname__"])) << ")";
    }

    void operator()(ValueList list) {
        stream << "[";
        for (auto& value : list->values) {
            std::visit(value_helper::visitor_debug_repr(stream), value);
            stream << ", ";
        }
        stream << "]";
    }

    template<typename T> 
    void operator()(T) const {
        // TODO: use typetraits to generate this.
        throw pyerror("unimplemented repr for type");
    }
};

}

std::ostream& operator << (std::ostream& stream, const Value value) {
    std::visit(value_helper::visitor_debug_repr(stream), value);
    return stream;
}

}
