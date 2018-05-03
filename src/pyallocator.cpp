#include "pyallocator.hpp"

namespace py {

void mark_children(FrameState& frame) {
    
}


Allocator::collect_garbage(InterpreterState& state) {
    FrameState *frame = state.frame;

    while (frame != nullptr) {
        // TODO: actually collect garbage
        mark_children(frame);

        frame = frame.parent_frame;
    }
}

}