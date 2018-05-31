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

using Namespace = gc_ptr<std::unordered_map<std::string, Value>>;

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
    constexpr const static uint8_t FLAG_CLASS_INIT_FRAME = 1;
    constexpr const static uint8_t FLAG_OBJECT_INIT_FRAME = 2;
    constexpr const static uint8_t FLAG_IS_GENERATOR_FUNCTION = 4;
    constexpr const static uint8_t FLAG_RETURNED = 8;
    constexpr const static uint8_t FLAG_DONT_RETURN = 16;

    uint64_t r_pc = 0; // program counter
    gc_ptr<FrameState> parent_frame = nullptr;
    InterpreterState *interpreter_state = nullptr; 

    ValueCode code;
    std::vector<Value> value_stack;
    std::vector<Block> block_stack; // a stack containing blocks: this should be changed to a standard vector
    Namespace ns_local; // the local value namespace
    uint8_t flags = 0;
    
    // The class we are initializing
    // It has become abundantly clear that the frame state which initializes the static fields
    // of a class must be able to access the class it is initializing
    // for this reason, I am adding a field that unfortunately is rarely used
    // Hopefully this can be factored out later, and as such it will only be accessed via helper functions
    ValuePyClass init_class;

    // Needed in LOAD_CLOSURE, LOAD_DEREF, and STORE_DEREF
    ValuePyFunction curr_func; // The function of this current frame stat
    std::vector<ValuePyObject> cells; // Cells that happen to be in my local namespace

    FrameState(const ValueCode code);
    FrameState(const ValueCode code, ValuePyClass& init_class);

    inline void initialize_fields() {
        DEBUG_ADV("initializing fields");
        this->r_pc = 0;
        this->flags = 0;
        this->parent_frame = nullptr;
        this->interpreter_state = nullptr;
        this->code = nullptr;
        this->value_stack.clear();
        this->block_stack.clear();
        this->ns_local = nullptr;
        this->init_class = nullptr;
        this->curr_func = nullptr;
        this->cells.clear();
    }
    void recycle(const ValueCode code);
    void recycle(const ValueCode code, ValuePyClass& init_class);

    void eval_next();

    static void print_value(Value& val);
    void print_stack() const;

    // Add a value to local namespace (for use when creating the frame state)
    void add_to_ns_local(const std::string& name,Value&& v);

    void initialize_from_pyfunc(const ValuePyFunction func, ArgList& args);
    
    // flag getters and setters
    inline bool get_flag(const uint8_t flag) {
        return this->flags & flag;
    }
    inline void set_flag(const uint8_t flag) {
        this->flags |= flag;
    }
    inline void clear_flag(const uint8_t flag) {
        this->flags &= (~flag);
    }
    
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