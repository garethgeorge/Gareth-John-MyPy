#ifndef PYALLOCATOR_H
#define PYALLOCATOR_H

#include <pygc.hpp>
#include <unordered_map>
#include <string>

#include "pyvalue.hpp"
#include "pycode.hpp" // TODO: i am unhappy about having to have this included
#include "pyframe.hpp"
namespace py {

    using namespace gc;

    struct InterpreterState;

    struct Allocator {
        size_t size_at_last_gc = 32; // 32 bytes or something like that.

        gc_heap<value::List> heap_list;
        gc_heap<value::Tuple> heap_tuple;
        gc_heap<const std::string> heap_string;
        gc_heap<Code> heap_code;
        gc_heap<value::PyFunc> heap_pyfunc;
        gc_heap<value::PyObject> heap_pyobject;
        gc_heap<value::PyClass> heap_pyclass;
        gc_heap<std::unordered_map<std::string, Value>> heap_namespace;
    
        // the recyclable heap types are defined here
        #ifdef RECYCLING_ON
        gc_heap_recycler<FrameState> heap_frame;
        #else 
        gc_heap<FrameState> heap_frame;
        #endif
        
        inline size_t memory_footprint() {
            return heap_list.memory_footprint() + 
                heap_tuple.memory_footprint() + 
                heap_string.memory_footprint() + 
                heap_code.memory_footprint() + 
                heap_frame.memory_footprint() + 
                heap_pyfunc.memory_footprint() + 
                heap_pyobject.memory_footprint() + 
                heap_pyclass.memory_footprint();
        }

        inline bool check_if_gc_needed() {
            return this->memory_footprint() >= size_at_last_gc * 2;
        }

        void print_debug_info();
        void mark_live_objects(InterpreterState& interp);
        void collect_garbage(InterpreterState& interp);

        void retain_all();
    };

    extern Allocator alloc;
}

#endif