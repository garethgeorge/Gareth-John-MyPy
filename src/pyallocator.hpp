#ifndef PYALLOCATOR_H
#define PYALLOCATOR_H

#include <pygc.hpp>

#include "pyvalue.hpp"

namespace py {

    using namespace gc;

    struct InterpreterState;

    struct Allocator {
        gc_heap<value::List> heap_list;
        gc_heap<std::string> heap_string;
    };

    extern Allocator alloc;
}

#endif