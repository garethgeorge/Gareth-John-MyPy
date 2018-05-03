#ifndef PYALLOCATOR_H
#define PYALLOCATOR_H

#include <pygc.hpp>

namespace py {

    using namespace gc;

    struct Allocator {
        gc_heap<value::List> heap_lists;
        
        void collect_garbage(InterpreterState& state);
    };

}


#endif