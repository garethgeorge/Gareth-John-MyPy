#pragma once
#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <stack>
#include <unordered_map>
#include "pyerror.hpp"
#include "pyvalue.hpp"
#include "optflags.hpp"

#include "pycode.hpp"
#include "pyframe.hpp"
#include "pyvalue.hpp"
#include "pyallocator.hpp"

namespace py {

struct Code;

using Namespace = std::shared_ptr<std::unordered_map<std::string, Value>>;

struct InterpreterState {
    std::shared_ptr<FrameState> cur_frame;
    std::stack<FrameState> callstack;
    Namespace ns_globals; // ns_globals is just ns_local of the very bottom FrameState
    Namespace ns_builtins;
    ValueCode main_code;
    
    Allocator alloc;

    InterpreterState(std::shared_ptr<Code>& code);

    void eval();
};

}

#endif 