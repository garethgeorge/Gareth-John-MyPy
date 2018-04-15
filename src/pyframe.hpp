#ifndef PYFRAME_H
#define PYFRAME_H

#include <stack>
#include <memory>
#include <unordered_map>

#include "pyvalue.hpp"
#include "optflags.hpp"

namespace py {

using Namespace = std::unordered_map<std::string, Value>;

struct Code;
struct InterpreterState;

struct Block {
    enum Type {
        NONE,
        LOOP,
        EXCEPT,
        FINALLY
    };
    Type type = NONE;
    size_t pc_start = 0;
    size_t pc_delta = 0;
    size_t level = 0; // value level to pop up to
};

struct FrameState {
    uint64_t r_pc = 0; // program counter
    FrameState *parent_frame = nullptr;
    InterpreterState *interpreter_state = nullptr;
    std::shared_ptr<Code> code;
    std::stack<Value> value_stack;
    std::stack<Block> block_stack;
    Namespace ns_local;

#ifdef OPT_FRAME_NS_LOCAL_SHORTCUT
    struct NameCacheEntry {
        const std::string *key = nullptr;
        Value *value = nullptr;
    };
    NameCacheEntry ns_local_shortcut[8] = {};
#endif
    
    FrameState(
        InterpreterState *interpreter_state, 
        FrameState *parent_frame, 
        std::shared_ptr<Code>& code);

    void eval_next();
    void print_next();
};

}

#endif