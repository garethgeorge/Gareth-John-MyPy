#pragma once
#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <stack>
#include <unordered_map>
#include <stdio.h>
#include <sys/time.h>
#include <chrono>

#include <oplist.hpp>
#include "pyerror.hpp"
#include "pyvalue.hpp"
#include "optflags.hpp"

#include "pycode.hpp"
#include "pyframe.hpp"
#include "pyvalue.hpp"
#include "pyallocator.hpp"

#ifdef PROFILING_SIMPLE
#include <iostream>
#endif

namespace py {

struct Code;

#ifdef PROFILING_ON
    const uint64_t GC_START = 254;
    const uint64_t GC_END = 253;
#endif

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

#ifdef PROFILING_SIMPLE
    uint64_t op_times[255] = {0};
    uint64_t op_counts[255] = {0};
    
    inline int64_t curtime() {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    }

    int8_t last_op = 0;
    uint64_t last_op_start = curtime();
    
    inline void profiler_begin_op(Code::Instruction instruction, Code::ByteCode bytecode, uint64_t arg) {
        uint64_t now = curtime();
        op_times[last_op] += now - last_op_start;
        op_counts[last_op]++;
        last_op = bytecode;
        last_op_start = now;
    }

    void dump_op_durations() {
        for (size_t i = 0; i < sizeof(op_times) / sizeof(uint64_t); ++i) {
            if (op_counts[i] != 0) {
                std::cerr << i << "," << op::name[i] << "," << op_counts[i] << "," << op_times[i] << "," << (((double)op_times[i])/((double)op_counts[i])) << std::endl;
            }
        }
    }
#endif

#ifdef PROFILING_ON
    // For use in any profiling thign that needs a timespec to find out what right now is
    //timespec tmp_timespec;
    // An array of uint64_t that are timestamped events
    // Each is list of what happened (usually an OPCODE), some extra info, and when it happened
    // Interpreted in dump_and_clear_time_events
    std::vector<uint64_t> time_events;
    FILE* profiling_file;
    #ifdef PER_OPCODE_PROFILING
        void emit_opcode_data(  const Code::Instruction& instruction,
                                const Code::ByteCode& bytecode, 
                                const uint64_t& arg
        );
    #endif

    #ifdef GARBAGE_COLLECTION_PROFILING
        void emit_gc_event(bool start);
    #endif

    void dump_and_clear_time_events();

    ~InterpreterState() {
        fflush(this->profiling_file);
    }
#endif
};

}

#endif 