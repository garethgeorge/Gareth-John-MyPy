#pragma once
#ifndef PYFRAME_H
#define PYFRAME_H

#include <stack>
#include <vector>
#include <memory>
#include <unordered_map>
#include <cassert>

#include "pyvalue.hpp"
#include "optflags.hpp"
#include "pyerror.hpp"
#include "debug.hpp"

namespace py {

using Namespace = std::shared_ptr<std::unordered_map<std::string, Value>>;

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

const uint8_t CLASS_INIT_FRAME = 1;
const uint8_t OBJECT_INIT_FRAME = 2;
const uint8_t DONT_RETURN_FRAME = 4;

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
    
    // The class we are initializing
    // It has become abundantly clear that the frame state which initializes the static fields
    // of a class must be able to access the class it is initializing
    // for this reason, I am adding a field that unfortunately is rarely used
    // Hopefully this can be factored out later, and as such it will only be accessed via helper functions
    ValuePyClass init_class;

    // The function of this current frame state
    // Needed in LOAD_CLOSURE, LOAD_DEREF, and STORE_DEREF
    ValuePyFunction curr_func;

    FrameState(
        InterpreterState *interpreter_state, 
        FrameState *parent_frame, // null for the top frame on the stack
        const ValueCode& code);

    // Initialize for as class static initializer
    FrameState(
        InterpreterState *interpreter_state, 
        FrameState *parent_frame,
        const ValueCode& code,
        ValuePyClass& init_class);

    void eval_next();
    void print_next();

    static void print_value(Value& val);
    void print_stack() const;

    // Add a value to local namespace (for use when creating the frame state)
    void add_to_ns_local(const std::string& name,Value&& v);
    void initialize_from_pyfunc(const ValuePyFunction func,std::vector<Value>& args);

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