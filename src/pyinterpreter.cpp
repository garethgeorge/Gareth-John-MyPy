#include "pyinterpreter.hpp"
#include "../lib/debug.hpp"
namespace py {

/* 
    INTERPRETER STATE
*/
InterpreterState::InterpreterState(ValueCode code) {
    //code->print_bytecode();

    this->ns_builtins = alloc.heap_namespace.make();

    this->push_frame(
        alloc.heap_frame.make(code)
    );

    // make ns_globals refer to the bottom's locals
    this->ns_globals = this->cur_frame->ns_local;

    // Save a reference to the code
    this->main_code = code;
}

// InterpreterState::eval can be found in pyframe.cpp to allow inlining

}