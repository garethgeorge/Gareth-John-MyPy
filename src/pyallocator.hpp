#ifndef PYALLOCATOR_H
#define PYALLOCATOR_H

#include <pygc.hpp>

#include "pyvalue.hpp"
#include "pycode.hpp" // TODO: i am unhappy about having to have this included

namespace py {

    using namespace gc;

    struct InterpreterState;

    struct Allocator {
        gc_heap<value::List> heap_list;
        gc_heap<std::string> heap_string;
        gc_heap<Code> heap_code;
    };

    extern Allocator alloc;
}

#endif