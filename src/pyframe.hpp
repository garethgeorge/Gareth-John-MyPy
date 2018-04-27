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
#include "debug.hpp"

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
    public:
    uint64_t r_pc = 0; // program counter
    FrameState *parent_frame = nullptr;
    InterpreterState *interpreter_state = nullptr;
    ValueCode code;
    std::vector<Value> value_stack;
    std::stack<Block> block_stack; // a stack containing blocks: this should be changed to a standard vector
    Namespace ns_local; // the local value namespace
    uint8_t flags = 0;

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
        FrameState *parent_frame, // null for the top frame on the stack
        const ValueCode& code);

    void eval_next();
    void print_next();

    void print_value(Value& val) const;
    void print_stack() const;

    // Add a value to local namespace (for use when creating the frame state)
    void add_to_ns_local(const std::string& name,Value&& v);
    void initialize_from_pyfunc(const ValuePyFunction& func,std::vector<Value>& args);

    // Set the flag that says that this framestate is initializing the static values of a class
    void set_class_static_init_flag();
    bool get_class_static_init_flag();

    // Im very sorry but it appears we do need two different flags
    void set_class_dynamic_init_flag();
    bool get_class_dynamic_init_flag();

    // helper method for checking the stack has enough values for the current
    // operation!
    inline const void check_stack_size(size_t expected) {
        if (this->value_stack.size() < expected) {
            throw pyerror("INTERNAL ERROR: not enough values on stack to complete operation");
        }
    }
};

}

#endif