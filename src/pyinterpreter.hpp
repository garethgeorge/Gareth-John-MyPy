#pragma once
#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <stack>
#include <unordered_map>
#include <stdio.h>
#include <sys/time.h>
#include "pyerror.hpp"
#include "pyvalue.hpp"
#include "optflags.hpp"

#include "pycode.hpp"
#include "pyframe.hpp"
#include "pyvalue.hpp"
#include "pyallocator.hpp"

namespace py {

struct Code;

using Namespace = gc_ptr<std::unordered_map<std::string, Value>>;

struct InterpreterState {
    gc_ptr<FrameState> cur_frame = nullptr;
    Namespace ns_globals; // ns_globals is just ns_local of the very bottom FrameState
    Namespace ns_builtins;
    ValueCode main_code;
    
    Allocator alloc;

    InterpreterState(ValueCode code);

    void eval();

    inline void push_frame(gc_ptr<FrameState> frame) {
        // TBD: does frame->parent_name need to be changed to a std::shared_ptr?
        frame->parent_frame = this->cur_frame;
        frame->interpreter_state = this;
        this->cur_frame = frame;
    }

    inline void pop_frame() {
        this->cur_frame = this->cur_frame->parent_frame;
    }


#ifdef PROFILING_ON
    // For use in any profiling thign that needs a timespec to find out what right now is
    timespec tmp_timespec;
    // An array of uint64_t that are timestamped events
    // Each is list of what happened (usually an OPCODE), some extra info, and when it happened
    // Interpreted in dump_and_clear_time_events
    std::vector<uint64_t> time_events;
    #ifdef PER_OPCODE_PROFILING
        FILE* opcode_data_file;
        //clock_t per_opcode_clk;
        struct timespec per_opcode_timespec;
        void emit_opcode_data(  const Code::Instruction& instruction,
                                const Code::ByteCode& bytecode, 
                                const uint64_t& arg
        );
    #endif

    void dump_and_clear_time_events();
#endif
};

}

#endif 