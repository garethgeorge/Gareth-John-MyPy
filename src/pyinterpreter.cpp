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

// InterpreterState::eval can be found in pyframe.cpp to allow inlining


}