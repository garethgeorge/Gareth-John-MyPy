#include "pyinterpreter.hpp"

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
        this->callstack.top().eval_next();
    }
}

}