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
        //fprintf(opcode_data_file,"\n\n---\nCLOCK_TICKS_PER_SECOND: %d\n",CLOCKS_PER_SEC);
        //per_opcode_clk = clock();
        clock_gettime(CLOCK_REALTIME, &per_opcode_timespec);
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
            /*fprintf(opcode_data_file,"TIME: %d\nOPCODE: %s, ARG: %llu, ", 
                        //difftime(time(NULL),per_opcode_curr_time),
                        clock() - per_opcode_clk,
                        op::name[bytecode],
                        arg
            );
            per_opcode_clk = clock();*/
            //clock_gettime(CLOCK_REALTIME, &tmp_timespec);
            clock_gettime(CLOCK_REALTIME, &per_opcode_timespec);
            /*fprintf(opcode_data_file,"OPCODE: %s, ARG: %llu, TIME: %llu sec %llu ns\n", 
                        op::name[bytecode],
                        arg,
                        per_opcode_timespec.tv_sec,
                        per_opcode_timespec.tv_nsec
            );*/
            time_events.resize(time_events.size() + 4);
            time_events.push_back(bytecode);
            time_events.push_back(arg);
            time_events.push_back(per_opcode_timespec.tv_sec);
            time_events.push_back(per_opcode_timespec.tv_nsec);
        }
    #endif

    // Every time event is an opcode, it's arg, the seconds, and the nanoseconds
    // For time events that are not opcodes, I use values that do not correspond to opcodes
    void InterpreterState::dump_and_clear_time_events(){
        clock_gettime(CLOCK_REALTIME, &tmp_timespec);
        fprintf(opcode_data_file,"\n\n------Time Data Dump:\n");
        fprintf(opcode_data_file,"\n------Dump Start Time: %llu sec %llu ns\n",
                                tmp_timespec.tv_sec,tmp_timespec.tv_nsec);
        for(int i = 0;i < time_events.size();i+=4){
            fprintf(opcode_data_file,"OPCODE: %s, ARG: %llu, TIME: %llu sec %llu ns\n", 
                        op::name[time_events[i]],
                        time_events[i + 1],
                        time_events[i + 2],
                        time_events[i + 3]
            );
        }
        fprintf(opcode_data_file,"\n------Dump End Time: %llu sec %llu ns\n",
                        tmp_timespec.tv_sec,tmp_timespec.tv_nsec);
        time_events.clear();
    }
#endif

// InterpreterState::eval can be found in pyframe.cpp to allow inlining

}