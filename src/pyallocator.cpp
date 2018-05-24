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

void Allocator::mark_live_objects(InterpreterState& interp) {
    DEBUG_ADV("MARKING LIVE OBJECTS");

    if (interp.cur_frame != nullptr) {
        interp.cur_frame.mark();
    }
    interp.ns_globals.mark();
    interp.main_code.mark();
}

void Allocator::print_debug_info() {
    DEBUG_ADV("\tSIZE OF HEAP_LIST: " << heap_list.memory_footprint() << " - " << heap_list.size());
    DEBUG_ADV("\tSIZE OF HEAP_STRING: " << heap_string.memory_footprint() << " - " << heap_string.size());
    DEBUG_ADV("\tSIZE OF HEAP_CODE: " << heap_code.memory_footprint() << " - " << heap_code.size());
    DEBUG_ADV("\tSIZE OF HEAP_FRAME: " << heap_frame.memory_footprint() << " - " << heap_frame.size());
    DEBUG_ADV("\tSIZE OF HEAP_PYFUNC: " << heap_pyfunc.memory_footprint() << " - " << heap_pyfunc.size());
    DEBUG_ADV("\tSIZE OF HEAP_PYOBJECT " << heap_pyobject.memory_footprint() << " - " << heap_pyobject.size());
    DEBUG_ADV("\tSIZE OF HEAP_PYCLASS: " << heap_pyclass.memory_footprint() << " - " << heap_pyclass.size());
    DEBUG_ADV("\tSIZE OF HEAP_NAMESPACE: " << heap_namespace.memory_footprint() << " - " << heap_namespace.size());
}

void Allocator::collect_garbage(InterpreterState& interp) {
    this->mark_live_objects(interp);

    size_t size_before = this->memory_footprint();
    DEBUG_ADV("SWEEPING THE HEAP, CURRENT SIZE: " << size_before);
    print_debug_info();
    
    DEBUG_ADV("\tCLEANING LISTS");
    heap_list.sweep();
    DEBUG_ADV("\tCLEANING STRINGS");
    heap_string.sweep();
    DEBUG_ADV("\tCLEANING CODE");
    heap_code.sweep();
    DEBUG_ADV("\tCLEANING FRAME");
    heap_frame.sweep();
    DEBUG_ADV("\tCLEANING PYFUNCS");
    heap_pyfunc.sweep();
    DEBUG_ADV("\tCLEANING PYOBJECTS");
    heap_pyobject.sweep();
    DEBUG_ADV("\tCLEANING PYCLASSES");
    heap_pyclass.sweep();
    DEBUG_ADV("\tCLEANING NAMESPACES");
    heap_namespace.sweep();

    size_t new_size = this->memory_footprint();
    DEBUG_ADV("CLEANED UP " << size_before - new_size << " BYTES, NEW SIZE: " << new_size);
    print_debug_info();

    this->size_at_last_gc = new_size;
}

}