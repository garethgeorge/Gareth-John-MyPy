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
    tmp_time = std::chrono::high_resolution_clock::now();
    profiling_file = fopen("profiling_data.txt","a");
    fprintf(profiling_file,"Program Output");
        #ifdef PER_OPCODE_PROFILING
            per_opcode_time = std::chrono::high_resolution_clock::now();
        #endif
    #endif

}

#ifdef PROFILING_ON
    #ifdef PER_OPCODE_PROFILING
        void InterpreterState::emit_opcode_data(const Code::Instruction& instruction,
                                                const Code::ByteCode& bytecode, 
                                                const uint64_t& arg
        ) {
            // Just put a right now timestamp on it and continue
            per_opcode_time = std::chrono::high_resolution_clock::now();
            time_events.push_back(bytecode);
            time_events.push_back(arg);
            time_events.push_back(
                std::chrono::duration_cast<std::chrono::nanoseconds>(per_opcode_time.time_since_epoch()).count()
            );
        }
    #endif

    #ifdef GARBAGE_COLLECTION_PROFILING
        void InterpreterState::emit_gc_event(bool start){
            tmp_time = std::chrono::high_resolution_clock::now();
            if(start){
                time_events.push_back(GC_START);
            } else {
                time_events.push_back(GC_END);
            }
            time_events.push_back(0);
            time_events.push_back(
                std::chrono::duration_cast<std::chrono::nanoseconds>(tmp_time.time_since_epoch()).count()
            );
        }
    #endif

    // Every time event is an opcode, it's arg, the seconds, and the nanoseconds
    // For time events that are not opcodes, I use values that do not correspond to opcodes
    void InterpreterState::dump_and_clear_time_events(){
        tmp_time = std::chrono::high_resolution_clock::now();
        fprintf(profiling_file,"Dump Start Time: %llu\n",
            std::chrono::duration_cast<std::chrono::nanoseconds>(tmp_time.time_since_epoch()).count()
        );
        for(int i = 0;i < time_events.size();i+=3){
            uint64_t c_op = time_events[i];
            if(c_op == GC_START || c_op == GC_END){
                fprintf(profiling_file,"\tGC %s: %llu\n",
                    (c_op == GC_START ? "START" : "END"),
                    time_events[i + 2]
                );
            } else {
                fprintf(profiling_file,"\tOPCODE, ARG, TIME: %s, %llu, %llu\n", 
                            op::name[time_events[i]],
                            time_events[i + 1],
                            time_events[i + 2]
                );
            }
        }
        tmp_time = std::chrono::high_resolution_clock::now();
        fprintf(profiling_file,"Dump End Time: %llu\n",
            std::chrono::duration_cast<std::chrono::nanoseconds>(tmp_time.time_since_epoch()).count()
        );
                        //tmp_timespec.tv_sec,tmp_timespec.tv_nsec);
        time_events.clear();
    }
#endif

// InterpreterState::eval can be found in pyframe.cpp to allow inlining

}