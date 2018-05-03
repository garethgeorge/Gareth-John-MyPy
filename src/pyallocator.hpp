#ifndef PYALLOCATOR_H
#define PYALLOCATOR_H

#include <pygc.hpp>

#include "pyvalue.hpp"

namespace py {

    using namespace gc;

    struct InterpreterState;

    struct Allocator {
        gc_heap<value::List> heap_lists;
        
        void collect_garbage(InterpreterState& state);
    };

}


#endif