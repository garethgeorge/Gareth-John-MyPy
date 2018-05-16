#include "pyvalue_helpers.hpp"
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
            stream << *(func->name);
        } else 
            stream << "anonymous function";
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

ValuePyObject create_cell(Value contents){
    DEBUG_ADV("Creating cell for " << contents << "\n");
    ValuePyObject nobj = std::make_shared<value::PyObject>(value::PyObject(cell_class));
    nobj->store_attr("contents",contents);
    //ValueString vs = std::make_shared<std::string>(std::string("CELL_OBJECT"));
    //nobj->store_attr("__qualname__",vs);
    (*(nobj->static_attrs->attrs))["__qualname__"] = std::make_shared<std::string>(std::string("CELL_OBJECT"));
    return nobj;
}

}

std::ostream& operator << (std::ostream& stream, const Value value) {
    std::visit(value_helper::visitor_debug_repr(stream), value);
    return stream;
}

// This is needed to allow the create_cell function
ValuePyClass cell_class = std::make_shared<value::PyClass>(value::PyClass());

}
