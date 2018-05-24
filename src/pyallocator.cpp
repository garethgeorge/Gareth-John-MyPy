#define DEBUG_ON

#include <debug.hpp>
#include <variant>
#include <iostream>

#include "pyallocator.hpp"
#include "pyinterpreter.hpp"
#include "pyvalue_helpers.hpp"

namespace py {


Allocator alloc;

struct gc_visitor {
    void operator() (value::PyGenerator& gen) {
        gen.frame.mark();
    }

    template<typename T>
    void operator() (gc_ptr<T> value) {
        value.mark();
    }

    template<typename T>
    void operator() (T& value) {
        // we don't need to do anything here
    }
};

}

namespace gc {

    using namespace py;

    void mark_children(std::vector<Value>& values) {
        DEBUG_ADV("\tMarking a vector");
        for (auto& value : values) {
            std::visit(gc_visitor(), value);
        }
    }

    void mark_children(gc_ptr<FrameState> framestate) {
        DEBUG_ADV("\tMarking frame state " << Value(framestate));
        framestate->code.mark();
        mark_children(framestate->value_stack);
        framestate->ns_local.mark();

        if (framestate->init_class) {
            framestate->init_class.mark();
        }

        if (framestate->curr_func) {
            framestate->curr_func.mark();
        }

        for (ValuePyObject cell : framestate->cells) {
            cell.mark();
        }

        if (framestate->parent_frame != nullptr) {
            framestate->parent_frame.mark();
        }
    }

    void mark_children(ValueString string) {
        DEBUG_ADV("\tMarking string: " << Value(string));
    }

    void mark_children(ValueCode code) {
        DEBUG_ADV("\tMarking a code object");
        for (auto& value : code->co_consts) {
            std::visit(gc_visitor(), value);
        }
    }

    void mark_children(Namespace ns) {
        DEBUG_ADV("\tMarking a namespace");
        for (const auto& [key, value] : *ns) {
            std::visit(gc_visitor(), value);
        }
    }

    void mark_children(ValueList list) {
        DEBUG_ADV("\tMarking ValueList " << Value(list));
        mark_children(list->values);
    }

    void mark_children(ValuePyObject pyobject) {
        DEBUG_ADV("\tMarking ValuePyObject " << Value(pyobject));
        if (pyobject->static_attrs) {
            pyobject->static_attrs.mark();
        }

        pyobject->attrs.mark();
    }

    void mark_children(ValuePyClass pyclass) {
        DEBUG_ADV("\tMarking ValuePyClass " << Value(pyclass));
        pyclass->attrs.mark();

        for (ValuePyClass parent : pyclass->parents) {
            parent.mark();
        }
    }

    void mark_children(ValuePyFunction func) {
        DEBUG_ADV("\tMarking ValuePyFunction " << Value(func));
        func->name.mark();

        func->code.mark();

        if (func->def_args)
            func->def_args.mark();

        std::visit(gc_visitor(), func->self);
        
        if (func->__closure__) {
            func->__closure__.mark();
        }
        
    }
}

namespace py {

void Allocator::collect_garbage(InterpreterState& interp) {
    DEBUG_ADV("COLLECTING GARBAGE");

    interp.cur_frame.mark();
    
    mark_children(interp.ns_globals);

    // heap_list.sweep();
    // heap_string.sweep();
    // heap_code.sweep();
    // heap_frame.sweep();
    // heap_pyfunc.sweep();
    // heap_pyobject.sweep();
    // heap_pyclass.sweep();
}

}