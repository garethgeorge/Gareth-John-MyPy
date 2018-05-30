#include "pyvalue.hpp"
#include "pyallocator.hpp"

namespace py {

namespace value {

PyObject::PyObject(ValuePyClass cls) : static_attrs(cls) {
    this->attrs = alloc.heap_namespace.make();
};

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
        if (arg == nullptr) {
            stream << "Null ValuePyObject";
        } else if (arg->static_attrs == nullptr) {
            stream << "ValuePyObject with null static_attrs";
        } else {
            stream << "ValuePyObject of class ("
                << *(std::get<ValueString>((*(arg->static_attrs->attrs))["__qualname__"])) << ")";
        }
    }

    void operator()(ValuePyGenerator arg) {
        stream << "ValuePyGenerator";
    }

    void operator()(ValueList list) {
        stream << "[";
        size_t i = 0;
        for (auto& value : list->values) {
            std::visit(visitor_debug_repr(stream), value);
            stream << ", ";
            if (i++ > 50) {
                stream << "...";
                break;
            }
        }
        stream << "]";
    }

    void operator()(ValueTuple list) {
        stream << "(";
        size_t i = 0;
        for (auto& value : list->values) {
            std::visit(visitor_debug_repr(stream), value);
            stream << ", ";
            if (i++ > 50) {
                stream << "...";
                break;
            }
        }
        stream << ")";
    }

    void operator()(ValueCGenerator generator) {
        stream << "(Unknown C Generator)";
    }

    template<typename T> 
    void operator()(T) {
        // TODO: use typetraits to generate this.
        throw pyerror(string("unimplemented repr for type: ") + typeid(T).name());
    }
};

std::ostream& operator << (std::ostream& stream, const Value value) {
    std::visit(visitor_debug_repr(stream), value);
    return stream;
}

}