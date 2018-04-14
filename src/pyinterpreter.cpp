#include "pyinterpreter.hpp"
#include "../lib/debug.hpp"
namespace py {

/* 
    INTERPRETER STATE
*/
InterpreterState::InterpreterState(
    std::shared_ptr<Code>& code) {
    
    this->callstack.push(
        FrameState(this, nullptr, code)
    );
}

void InterpreterState::eval() {
    while (!this->callstack.empty()) {
        // TODO: try caching the top of the stack
        #ifdef PRINT_OPS
        this->callstack.top().eval_print();
        #else
        this->callstack.top().eval_next();
        #endif
    }
}

}