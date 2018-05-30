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
        fprintf(opcode_data_file,"\n\n---\nCLOCK_TICKS_PER_SECOND: %d\n",CLOCKS_PER_SEC);
        //per_opcode_curr_time = time(NULL);
        //time(&per_opcode_curr_time);
        per_opcode_clk = clock();
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
            fprintf(opcode_data_file,"TIME: %d\nOPCODE: %s, ARG: %llu, ", 
                        //difftime(time(NULL),per_opcode_curr_time),
                        clock() - per_opcode_clk,
                        op::name[bytecode],
                        arg
            );
            per_opcode_clk = clock();
            //per_opcode_curr_time = time(NULL);
            //time(&per_opcode_curr_time);
        }
    #endif
#endif

// InterpreterState::eval can be found in pyframe.cpp to allow inlining

}