#include "pyallocator.hpp"
#include "pyinterpreter.hpp"

namespace py {


namespace gc {

    void mark_children(FrameState* framestate) {
        
    }

}


Allocator alloc;


void Allocator::collect_garbage(InterpreterState* interp) {
    // interp->cur_frame.mark();

    // for (auto& pair : interp->ns_globals) {

    // }

    // heap_list.sweep();
    // heap_string.sweep();
    // heap_code.sweep();
    // heap_frame.sweep();
    // heap_pyfunc.sweep();
    // heap_pyobject.sweep();
    // heap_pyclass.sweep();
}

}