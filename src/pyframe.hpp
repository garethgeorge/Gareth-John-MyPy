#pragma once
#ifndef PYFRAME_H
#define PYFRAME_H

#include <stack>
#include <vector>
#include <memory>
#include <unordered_map>

#include "pyvalue.hpp"
#include "optflags.hpp"
#include "pyerror.hpp"

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
    std::vector<Value> value_stack;
    std::stack<Block> block_stack; // a stack containing blocks: this should be changed to a standard vector
    Namespace ns_local; // the local value namespace

#ifdef OPT_FRAME_NS_LOCAL_SHORTCUT
    // with this #define we are feature flagging the NS_LOCAL_SHORTCUT feature
    // this is an optimization that bypasses the ns_local namespace string lookup
    // by caching the pointer to the value for a given key in an 8 slot lookup table
    // since keys for variables are expected to be shared string constants,
    // we can simply check the key by memory address
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

    // helper method for popping a value from this->value_stack
    // throws pyerror("value stack is empty") when an error is encountered
    inline Value pop_value() {
        if (this->value_stack.size() == 0) {
            pyerror("value stack is empty, no more values to pop.");
        }
        Value tmp = std::move(this->value_stack.back());
        this->value_stack.pop_back();
        return std::move(tmp);
    }
    
    // helper method for pushing a value onto the value stack
    inline void push_value(const Value&& value) {
        this->value_stack.push_back(std::move(value));
    }
};

}

#endif