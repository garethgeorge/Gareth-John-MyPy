#include "pyinterpreter.hpp"
#include "pycode.hpp"
#include "../lib/debug.hpp"
#include "../lib/oplist.hpp"
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

#ifdef PROFILING_ON
    #ifdef PER_OPCODE_PROFILING
        opcode_data_file = fopen("per_opcode_data.txt","a");
        per_opcode_curr_time = time(NULL);
    #endif
#endif

}

#ifdef PROFILING_ON
    #ifdef PER_OPCODE_PROFILING
        void InterpreterState::emit_opcode_data(const Code::Instruction& instruction,
                                                const Code::ByteCode& bytecode, 
                                                const uint64_t& arg
        ) {
            // Print the time it took since last opcode to get here
            fprintf(opcode_data_file,"TIME: %lf\nOPCODE: %s,  ", 
                        difftime(time(NULL),per_opcode_curr_time),
                        op::name[bytecode]
            );
            per_opcode_curr_time = time(NULL);
        }
    #endif
#endif

// InterpreterState::eval can be found in pyframe.cpp to allow inlining

}