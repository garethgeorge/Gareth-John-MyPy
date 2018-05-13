#include "pyinterpreter.hpp"
#include "../lib/debug.hpp"
namespace py {

/* 
    INTERPRETER STATE
*/
InterpreterState::InterpreterState(
    std::shared_ptr<Code>& code) {
    
    //code->print_bytecode();

    this->ns_builtins = std::make_shared<std::unordered_map<std::string, Value>>();

    this->callstack.push(
        std::move(FrameState(this, nullptr, code))
    );

    // make ns_globals refer to the bottom's locals
    this->ns_globals = this->callstack.top().ns_local;

    // Save a reference to the code
    this->main_code = code;
}

// InterpreterState::eval can be found in pyframe.cpp to allow inlining

}